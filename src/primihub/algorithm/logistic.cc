/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>
#include <glog/logging.h>
#include <sys/stat.h>

#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/model.h"

using namespace std;
using namespace Eigen;
using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::Table;

namespace primihub {
eMatrix<double> logistic_main(sf64Matrix<D> &train_data_0_1,
                              sf64Matrix<D> &train_label_0_1,
                              sf64Matrix<D> &W2_0_1,
                              sf64Matrix<D> &test_data_0_1,
                              sf64Matrix<D> &test_label_0_1, aby3ML &p, int B,
                              int IT, int pIdx) {
  RegressionParam params;
  params.mBatchSize = B;
  params.mIterations = IT;
  params.mLearningRate = 1.0 / (1 << 7);

  eMatrix<double> val_W2;

  LOG(INFO) << "(Learning rate):" << params.mLearningRate << ".\n";
  LOG(INFO) << "(Epoch):" << params.mIterations << ".\n";
  LOG(INFO) << "(Batchsize) :" << params.mBatchSize << ".\n";
  LOG(INFO) << "(Train_loader size):"
            << (train_data_0_1.rows() / params.mBatchSize) << ".\n";

  SGD_Logistic(params, p, train_data_0_1, train_label_0_1, W2_0_1,
               &test_data_0_1, &test_label_0_1);

  val_W2 = p.reveal(W2_0_1);
  return val_W2;
}

LogisticRegressionExecutor::LogisticRegressionExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "logistic_regression";

  this->set_party_name(config.party_name());
  this->set_party_id(config.party_id());
  local_id_ = config.party_id();
#ifdef MPC_SOCKET_CHANNEL
  auto &node_map = config.node_map;
  std::map<uint16_t, rpc::Node> party_id_node_map;
  for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
    rpc::Node &node = iter->second;
    uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
    party_id_node_map[party_id] = node;
  }

  auto iter = node_map.find(config.node_id); // node_id
  if (iter == node_map.end()) {
    stringstream ss;
    ss << "Can't find " << config.node_id << " in node_map.";
    throw std::runtime_error(ss.str());
  }

  local_id_ = iter->second.vm(0).party_id();
  LOG(INFO) << "Note party id of this node is " << local_id_ << ".";

  if (local_id_ == 0) {
    rpc::Node &node = party_id_node_map[0];
    uint16_t port = 0;

    // Two Local server addr.
    port = node.vm(0).next().port();
    next_addr_ = std::make_pair(node.ip(), port);

    port = node.vm(0).prev().port();
    prev_addr_ = std::make_pair(node.ip(), port);
  } else if (local_id_ == 1) {
    rpc::Node &node = party_id_node_map[1];

    // A local server addr.
    uint16_t port = node.vm(0).next().port();
    next_addr_ = std::make_pair(node.ip(), port);

    // A remote server addr.
    prev_addr_ =
        std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());
  } else {
    rpc::Node &node = party_id_node_map[2];

    // Two remote server addr.
    next_addr_ =
        std::make_pair(node.vm(0).next().ip(), node.vm(0).next().port());
    prev_addr_ =
        std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());
  }
#else
  this->party_config_.CopyFrom(config);
#endif

  // Key when save model.
  std::stringstream ss;
  ss << config.job_id << "_" << config.task_id << "_party_" << local_id_
     << "_lr";
  model_name_ = ss.str();
}

int LogisticRegressionExecutor::loadParams(primihub::rpc::Task &task) {
  try {
    LOG(INFO) << "party_name: " << this->party_name_;
    auto party_datasets = task.party_datasets();
    auto it = party_datasets.find(this->party_name());
    if (it == party_datasets.end()) {
      LOG(ERROR) << "no data set found for party name: " << this->party_name();
      return -1;
    }
    const auto& dataset = it->second.data();
    auto iter = dataset.find("Data_File");
    if (iter == dataset.end()) {
      LOG(ERROR) << "no dataset found for dataset name Data_File";
      return -1;
    }
    train_input_filepath_ = iter->second;
    auto param_map = task.params().param_map();
    // test_input_filepath_ = param_map["TestData"].value_string();
    batch_size_ = param_map["BatchSize"].value_int32();
    num_iter_ = param_map["NumIters"].value_int32();
    model_file_name_ = param_map["modelName"].value_string();

    if (model_file_name_ == "")
      model_file_name_ = "./" + model_name_ + ".csv";

  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }
  LOG(INFO) << "Train data " << train_input_filepath_ << ", test data "
            << test_input_filepath_ << ".";
  return 0;
}

int LogisticRegressionExecutor::_LoadDatasetFromCSV(std::string &dataset_id) {
  auto driver = this->dataset_service_->getDriver(dataset_id);
  auto cursor = driver->read();
  std::shared_ptr<Dataset> ds = cursor->read();
  std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();
  bool errors = false;
  int num_col = table->num_columns();

  // 'array' include values in a column of csv file.
  auto array = std::static_pointer_cast<DoubleArray>(
      table->column(num_col - 1)->chunk(0));
  int64_t array_len = array->length();
  VLOG(3) << "Label column '" << col_names[num_col - 1] << "' has " << array_len
          << " values.";

  // Force the same value count in every column.
  for (int i = 0; i < num_col - 1; i++) {
    auto array =
        std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
    if (array->length() != array_len) {
      LOG(ERROR) << "Column " << col_names[i] << " has " << array->length()
                 << " value, but label column has " << array_len << " value.";
      errors = true;
      break;
    }
  }

  if (errors)
    return -1;
  int64_t train_length = floor(array_len * 0.8);
  int64_t test_length = array_len - train_length;
  // LOG(INFO)<<"array_len: "<<array_len;
  // LOG(INFO)<<"train_length: "<<train_length;
  // LOG(INFO)<<"test_length: "<<test_length;
  // train_input_.resize(train_length, num_col);
  // test_input_.resize(test_length, num_col);
  // // m.resize(array_len, num_col);
  // for (int i = 0; i < num_col; i++) {
  //   if (table->schema()->GetFieldByName(col_names[i])->type()->id() == 9) {
  //     auto array =
  //         std::static_pointer_cast<Int64Array>(table->column(i)->chunk(0));
  //     for (int64_t j = 0; j < array->length(); j++) {
  //       if (j < train_length)
  //         train_input_(j, i) = array->Value(j);
  //       else
  //         test_input_(j - train_length, i) = array->Value(j);
  //       // m(j, i) = array->Value(j);
  //     }
  //   } else {
  //     auto array =
  //         std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
  //     for (int64_t j = 0; j < array->length(); j++) {
  //       if (j < train_length)
  //         train_input_(j, i) = array->Value(j);
  //       else
  //         test_input_(j - train_length, i) = array->Value(j);
  //       // m(j, i) = array->Value(j);
  //     }
  //   }
  // }
  train_input_.resize(train_length, num_col + 1);
  test_input_.resize(test_length, num_col + 1);
  // m.resize(array_len, num_col);
  for (int i = 0; i < num_col + 1; i++) {
    if (i == 0) {
      for (int64_t j = 0; j < array_len; j++) {
        if (j < train_length) {
          train_input_(j, i) = 1;
        } else
          test_input_(j - train_length, i) = 1;
        // m(j, i) = array->Value(j);
      }
    } else {
      if (table->schema()->GetFieldByName(col_names[i - 1])->type()->id() ==
          9) {
        auto array = std::static_pointer_cast<Int64Array>(
            table->column(i - 1)->chunk(0));

        for (int64_t j = 0; j < array->length(); j++) {
          if (j < train_length)
            train_input_(j, i) = array->Value(j);
          else
            test_input_(j - train_length, i) = array->Value(j);
          // m(j, i) = array->Value(j);
        }
      } else {
        auto array = std::static_pointer_cast<DoubleArray>(
            table->column(i - 1)->chunk(0));
        for (int64_t j = 0; j < array->length(); j++) {
          if (j < train_length)
            train_input_(j, i) = array->Value(j);
          else
            test_input_(j - train_length, i) = array->Value(j);
          // m(j, i) = array->Value(j);
        }
      }
    }
  }

  return array->length();
}

int LogisticRegressionExecutor::loadDataset() {
  int ret = _LoadDatasetFromCSV(train_input_filepath_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset failed.";
    return -1;
  }

  // ret = _LoadDatasetFromCSV(test_input_filepath_, test_input_);
  // // file reading error or file empty
  // if (ret <= 0) {
  //   LOG(ERROR) << "Load dataset for test failed.";
  //   return -2;
  // }

  if (train_input_.cols() != test_input_.cols()) {
    LOG(ERROR)
        << "Dimension mismatch between local train dataset and test dataset.";
    return -3;
  }

  VLOG(3) << "Train dataset has " << train_input_.rows()
          << " examples, and dimension of each is " << train_input_.cols()
          << ".";

  VLOG(3) << "Test dataset has " << test_input_.rows()
          << " examples, and dimension of each is " << test_input_.cols()
          << ".";

  return 0;
}

#ifdef MPC_SOCKET_CHANNEL
int LogisticRegressionExecutor::initPartyComm(void) {
  VLOG(3) << "Next addr: " << next_addr_.first << ":" << next_addr_.second
          << ".";
  VLOG(3) << "Prev addr: " << prev_addr_.first << ":" << prev_addr_.second
          << ".";

  if (local_id_ == 0) {
    std::ostringstream ss;
    ss << "sess_" << local_id_ << "_1";
    std::string sess_name_1 = ss.str();

    ss.str("");
    ss << "sess_" << local_id_ << "_2";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   SessionMode::Server, sess_name_1);
    LOG(INFO) << "[Next] Init server session, party " << local_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   SessionMode::Server, sess_name_2);
    LOG(INFO) << "[Prev] Init server session, party " << local_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  } else if (local_id_ == 1) {
    std::ostringstream ss;
    ss << "sess_" << local_id_ << "_1";
    std::string sess_name_1 = ss.str();
    ss.str("");

    ss << "sess_" << PrevPartyId() << "_1";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   SessionMode::Server, sess_name_1);
    LOG(INFO) << "[Next] Init server session, party " << local_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   SessionMode::Client, sess_name_2);
    LOG(INFO) << "[Prev] Init client session, party " << local_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  } else {
    std::ostringstream ss;
    ss.str("");
    ss << "sess_" << this->NextPartyId() << "_2";
    std::string sess_name_1 = ss.str();

    ss.str("");
    ss << "sess_" << this->PrevPartyId() << "_1";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   SessionMode::Client, sess_name_1);
    LOG(INFO) << "[Next] Init client session, party " << local_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   SessionMode::Client, sess_name_2);
    LOG(INFO) << "[Prev] Init client session, party " << local_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  }

  auto chann_next = ep_next_.addChannel();
  auto chann_prev = ep_prev_.addChannel();

  chann_next.waitForConnection();
  chann_prev.waitForConnection();

  chann_next.send(local_id_);
  chann_prev.send(local_id_);

  uint16_t prev_party = 0;
  uint16_t next_party = 0;
  chann_next.recv(next_party);
  chann_prev.recv(prev_party);

  if (next_party != this->NextPartyId()) {
    LOG(ERROR) << "Party " << local_id_ << ", expect next party id "
               << this->NextPartyId() << ", but give " << next_party << ".";
    return -3;
  }

  if (prev_party != this->PrevPartyId()) {
    LOG(ERROR) << "Party " << local_id_ << ", expect prev party id "
               << this->PrevPartyId() << ", but give " << prev_party << ".";
    return -3;
  }

  chann_next.close();
  chann_prev.close();

  engine_.init(local_id_, ep_prev_, ep_next_, toBlock(local_id_));
  LOG(INFO) << "Init party: " << local_id_ << " communication finish.";

  return 0;
}
#else
int LogisticRegressionExecutor::initPartyComm(void) {
  uint16_t prev_party_id = this->PrevPartyId();
  uint16_t next_party_id = this->NextPartyId();
  auto link_ctx = this->GetLinkContext();
  if (link_ctx == nullptr) {
    LOG(ERROR) << "link context is not available";
    return -1;
  }
  auto& party_id_map = party_config_.PartyId2PartyNameMap();
  auto& party_info_map = party_config_.PartyName2PartyInfoMap();

  // construct channel for communication to next party
  std::string party_name_next = party_id_map[next_party_id];
  auto pb_party_node_next = party_info_map[party_name_next];
  Node party_node_next;
  pbNode2Node(pb_party_node_next, &party_node_next);
  auto base_channel_next = link_ctx->getChannel(party_node_next);

  // construct channel for communication to prev party
  std::string party_name_prev = party_id_map[prev_party_id];
  auto pb_party_node_prev = party_info_map[party_name_prev];
  Node party_node_prev;
  pbNode2Node(pb_party_node_prev, &party_node_prev);
  auto base_channel_prev = link_ctx->getChannel(party_node_prev);

  LOG(INFO) << "local_id_local_id_: " << local_id_;
  LOG(INFO) << "next_party: " << party_name_next << " detail: " << party_node_next.to_string();
  LOG(INFO) << "prev_party: " << party_name_prev << " detail: " << party_node_prev.to_string();
  MpcChannel channel_next(this->party_name(), link_ctx);
  MpcChannel channel_prev(this->party_name(), link_ctx);

  channel_next.SetupBaseChannel(party_name_next, base_channel_next);
  channel_prev.SetupBaseChannel(party_name_prev, base_channel_prev);

  engine_.init(local_id_, channel_prev, channel_next, toBlock(local_id_));
  return 0;
}
#endif

#ifdef MPC_SOCKET_CHANNEL
int LogisticRegressionExecutor::finishPartyComm(void) {
  ep_next_.stop();
  ep_prev_.stop();
  engine_.fini();
  return 0;
}
#else
int LogisticRegressionExecutor::finishPartyComm(void) { return 0; }

#endif

int LogisticRegressionExecutor::_ConstructShares(sf64Matrix<D> &w,
                                                 sf64Matrix<D> &train_data,
                                                 sf64Matrix<D> &train_label,
                                                 sf64Matrix<D> &test_data,
                                                 sf64Matrix<D> &test_label) {
  // Construct shares of train data and train label.
  LOG(ERROR) << "Construct shares of train data and train label.";
  sf64Matrix<D> train_shares[3];
  for (u64 i = 0; i < 3; i++) {
    if (local_id_ == i)
      train_shares[i] = engine_.localInput<D>(train_input_);
    else
      train_shares[i] = engine_.remoteInput<D>(i);
  }
  LOG(ERROR) << "end Construct shares of train data and train label.";
  if (train_shares[0].cols() != train_shares[1].cols()) {
    LOG(ERROR)
        << "Count of column in train dataset mismatch between party 0 and 1, "
        << "party 0 has " << train_shares[0].cols() << " column, party 1 has "
        << train_shares[1].cols() << " column.";
    return -1;
  }

  if (train_shares[1].cols() != train_shares[2].cols()) {
    LOG(ERROR)
        << "Count of column in train dataset mismatch between party 1 and 2, "
        << "party 1 has " << train_shares[1].cols() << " column, party 2 has "
        << train_shares[2].cols() << " column.";
    return -1;
  }

  int num_cols = train_shares[0].cols() - 1;
  int num_rows =
      train_shares[0].rows() + train_shares[1].rows() + train_shares[2].rows();

  train_data.resize(num_rows, num_cols);
  train_label.resize(num_rows, 1);

  int row_index = 0;
  for (int h = 0; h < 3; h++) {
    for (int i = 0; i < train_shares[h].rows(); i++) {
      for (int j = 0; j < train_shares[h].cols() - 1; j++) {
        train_data[0](row_index, j) = train_shares[h][0](i, j);
        train_data[1](row_index, j) = train_shares[h][1](i, j);
      }
      row_index++;
    }
  }

  row_index = 0;
  for (int h = 0; h < 3; h++) {
    for (int i = 0; i < train_shares[h].rows(); i++) {
      train_label[0](row_index, 0) = train_shares[h][0](i, num_cols);
      train_label[1](row_index, 0) = train_shares[h][1](i, num_cols);
      row_index++;
    }
  }

  // Construct shares of test data and test label.
  sf64Matrix<D> test_shares[3];
  for (u64 i = 0; i < 3; i++) {
    if (local_id_ == i)
      test_shares[i] = engine_.localInput<D>(test_input_);
    else
      test_shares[i] = engine_.remoteInput<D>(i);
  }

  if (test_shares[0].cols() != test_shares[1].cols()) {
    LOG(ERROR)
        << "Count of column in test dataset mismatch between party 0 and 1, "
        << "party 0 has " << test_shares[0].cols() << " column, party 1 has "
        << test_shares[1].cols() << " column.";
    return -2;
  }

  if (test_shares[1].cols() != train_shares[0].cols()) {
    LOG(ERROR)
        << "Count of column mismatch between train dataset and test dataset, "
        << "train dataset has " << train_shares[0].cols()
        << ", test dataset has " << test_shares[1].cols() << " column.";
    return -2;
  }

  if (test_shares[1].cols() != test_shares[2].cols()) {
    LOG(ERROR)
        << "Count of column in test dataset mismatch between party 1 and 2, "
        << "party 1 has " << test_shares[1].cols() << " column, party 2 has "
        << test_shares[2].cols() << " column.";
    return -2;
  }

  num_cols = test_shares[0].cols() - 1;
  num_rows =
      test_shares[0].rows() + test_shares[1].rows() + test_shares[2].rows();

  test_data.resize(num_rows, num_cols);
  test_label.resize(num_rows, 1);

  row_index = 0;
  for (int h = 0; h < 3; h++) {
    for (int i = 0; i < test_shares[h].rows(); i++) {
      for (int j = 0; j < test_shares[h].cols() - 1; j++) {
        test_data[0](row_index, j) = test_shares[h][0](i, j);
        test_data[1](row_index, j) = test_shares[h][1](i, j);
      }
      row_index++;
    }
  }

  row_index = 0;
  for (int h = 0; h < 3; h++) {
    for (int i = 0; i < test_shares[h].rows(); i++) {
      // train_label(row_index++, 1) = train_shares[h](i, num_cols);
      test_label[0](row_index, 0) = test_shares[h][0](i, num_cols);
      test_label[1](row_index, 0) = test_shares[h][1](i, num_cols);
      row_index++;
    }
  }

  // Create share of model.
  eMatrix<double> val_w(train_shares[0].cols() - 1, 1);
  val_w.setZero();
  if (local_id_ == 0) {
    w = engine_.localInput<D>(val_w);
  } else {
    w = engine_.remoteInput<D>(0);
  }

  LOG(INFO) << "Train dataset has " << train_data.rows()
            << " examples, dimension of each is " << train_data.cols() + 1
            << ".";
  LOG(INFO) << "Test dataset has " << test_data.rows()
            << " examples, dimension of each is " << test_data.cols() + 1
            << ".";
  return 0;
}

int LogisticRegressionExecutor::execute() {
  sf64Matrix<D> w;
  sf64Matrix<D> train_data, train_label;
  sf64Matrix<D> test_data, test_label;

  int ret = _ConstructShares(w, train_data, train_label, test_data, test_label);
  if (ret) {
    finishPartyComm();
    return -1;
  }

  model_ = logistic_main(train_data, train_label, w, test_data, test_label,
                         engine_, batch_size_, num_iter_, local_id_);

  LOG(INFO) << "Party " << local_id_ << " train finish.";
  return 0;
}

int LogisticRegressionExecutor::saveModel(void) {
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::DoubleBuilder builder(pool);

  for (int i = 0; i < model_.rows(); i++)
    builder.Append(model_(i, 0));

  std::shared_ptr<arrow::Array> array;
  builder.Finish(&array);

  std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
      arrow::field("w", arrow::float64())};
  auto schema = std::make_shared<arrow::Schema>(schema_vector);
  std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});

  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());

  bool dir_error = false;
  size_t st_pos = 0, end_pos = 0;
  std::string current_dir;
  while ((end_pos = model_file_name_.find("/", st_pos)) != std::string::npos) {
    if (end_pos == st_pos) {
      st_pos = end_pos + 1;
      continue;
    }

    current_dir = model_file_name_.substr(0, end_pos);
    if (mkdir(current_dir.c_str(), 0700)) {
      if (errno != EEXIST) {
        LOG(ERROR) << "Create directory " << current_dir << " failed, "
                   << strerror(errno) << ".";
        dir_error = true;
        break;
      }
    }

    st_pos = end_pos + 1;
    VLOG(3) << "Create directory " << current_dir << " finish.";
  }

  if (dir_error) {
    size_t pos = model_file_name_.rfind("/");
    std::string out_path = model_file_name_.substr(0, pos);
    LOG(ERROR) << "Create subpath " << current_dir << " failed, full path is "
               << out_path << ".";
    return -1;
  }

  auto cursor = driver->initCursor(model_file_name_);
  auto dataset = std::make_shared<primihub::Dataset>(table, driver);
  int ret = cursor->write(dataset);
  if (ret != 0) {
    LOG(ERROR) << "Save LR model to file " << model_file_name_ << " failed.";
    return -1;
  }
  LOG(INFO) << "Save model to " << model_file_name_ << ".";

  service::DatasetMeta meta(dataset, model_name_,
                            service::DatasetVisbility::PUBLIC,
                            model_file_name_);
  dataset_service_->regDataset(meta);
  LOG(INFO) << "Register new dataset finish.";
  return 0;
}

} // namespace primihub
