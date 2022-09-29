#include "src/primihub/algorithm/arithmetic.h"
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>
using arrow::Array;
using arrow::DoubleArray;
using arrow::Table;
namespace primihub {
void spiltStr(string str, const string &split, vector<string> &strlist) {
  strlist.clear();
  if (str == "")
    return;
  string strs = str + split;
  size_t pos = strs.find(split);
  int steps = split.size();

  while (pos != strs.npos) {
    string temp = strs.substr(0, pos);
    strlist.push_back(temp);
    strs = strs.substr(pos + steps, strs.size());
    pos = strs.find(split);
  }
}

// template <typename T>
// static MPCExpressExecutor *
// runParty(std::map<std::string, std::vector<T>> &col_and_val,
//          std::map<std::string, bool> &col_and_dtype,
//          std::map<std::string, u32> &col_and_owner, u32 party_id,
//          std::string next_ip, std::string prev_ip, uint16_t next_port,
//          uint16_t prev_port) {
//   MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();

//   mpc_exec->initColumnConfig(party_id);
//   importColumnOwner(mpc_exec, col_and_owner);
//   importColumnDtype(mpc_exec, col_and_dtype);

//   mpc_exec->importExpress(expr);
//   mpc_exec->resolveRunMode();

//   mpc_exec->InitFeedDict();
//   importColumnValues(mpc_exec, col_and_val);

//   // std::vector<double> final_val;

//   std::vector<uint32_t> parties = {0, 1};

//   try {
//     mpc_exec->initMPCRuntime(party_id, next_ip, prev_ip, next_port,
//     prev_port);
//     // ASSERT_EQ(mpc_exec->runMPCEvaluate(), 0);
//     mpc_exec->runMPCEvaluate();
//     mpc_exec->revealMPCResult(parties, final_val);
//     for (auto itr = final_val.begin(); itr != final_val.end(); itr++)
//       LOG(INFO) << *itr;
//   } catch (const std::exception &e) {
//     std::string msg = "In party 0, ";
//     msg = msg + e.what();
//     throw std::runtime_error(msg);
//   }

//   // delete mpc_exec;

//   return mpc_exec;
// }

ArithmeticExecutor::ArithmeticExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "arithmetic";

  mpc_exec_ = new MPCExpressExecutor();
  u32 party_id = 0;
  next_ip_ = "127.0.0.1";
  prev_ip_ = "127.0.0.1";
  next_port_ = 10010;
  prev_port_ = 10020;
  if (config.node_id == "node1") {
    party_id = 1;
    next_port_ = 10030;
    prev_port_ = 10010;
  } else if (config.node_id == "node2") {
    party_id = 2;
    next_port_ = 10020;
    prev_port_ = 10030;
  }
  //   mpc_exec->initColumnConfig(party_id);

  //   mpc_exec->InitFeedDict();

  //   for (auto &pair : col_and_val) //读取csv获取数据
  //     mpc_exec->importColumnValues(pair.first, pair.second);
  //   importColumnValues(mpc_exec, col_and_val);

  // std::vector<double> final_val;
  //   std::vector<T> final_val;

  //   try {
  //     mpc_exec->initMPCRuntime(party_id, next_ip, prev_ip, next_port,
  //     prev_port);
  //     // ASSERT_EQ(mpc_exec->runMPCEvaluate(), 0);
  //     mpc_exec->runMPCEvaluate();
  //     mpc_exec->revealMPCResult(parties, final_val);
  //     for (auto itr = final_val.begin(); itr != final_val.end(); itr++)
  //       LOG(INFO) << *itr;
  // }
  // catch (const std::exception &e) {
  //   std::string msg = "In party 0, ";
  //   msg = msg + e.what();
  //   throw std::runtime_error(msg);
  // }

  // std::map<std::string, Node> &node_map = config.node_map;
  // std::map<uint16_t, rpc::Node> party_id_node_map;
  // for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
  //   rpc::Node &node = iter->second;
  //   uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
  //   party_id_node_map[party_id] = node;
  // }
  // auto iter = node_map.find(config.node_id);
  // if (iter == node_map.end()) {
  //   stringstream ss;
  //   ss << "Can't find " << config.node_id << " in node_map.";
  //   throw std::runtime_error(ss.str());
  // }

  // local_id_ = iter->second.vm(0).party_id();
  // LOG(INFO) << "Note party id of this node is " << local_id_ << ".";

  // if (local_id_ == 0) {
  //   rpc::Node &node = party_id_node_map[0];
  //   uint16_t port = 0;

  //   // Two Local server addr.
  //   port = node.vm(0).next().port();
  //   next_addr_ = std::make_pair(node.ip(), port);

  //   port = node.vm(0).prev().port();
  //   prev_addr_ = std::make_pair(node.ip(), port);
  // } else if (local_id_ == 1) {
  //   rpc::Node &node = party_id_node_map[1];

  //   // A local server addr.
  //   uint16_t port = node.vm(0).next().port();
  //   next_addr_ = std::make_pair(node.ip(), port);

  //   // A remote server addr.
  //   prev_addr_ =
  //       std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());
  // } else {
  //   rpc::Node &node = party_id_node_map[2];

  //   // Two remote server addr.
  //   next_addr_ =
  //       std::make_pair(node.vm(0).next().ip(), node.vm(0).next().port());
  //   prev_addr_ =
  //       std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());
  // }

  // // Key when save model.
  // std::stringstream ss;
  // ss << config.job_id << "_" << config.task_id << "_party_" << local_id_;
  // model_name_ = ss.str();
}

int ArithmeticExecutor::loadParams(primihub::rpc::Task &task) {
  auto param_map = task.params().param_map();
  try {
    data_filepath_ = param_map["Data_File"].value_string();
    // col_and_owner
    std::string col_and_owner = param_map["Col_And_Owner"].value_string();
    std::vector<string> tmp1, tmp2, tmp3;
    spiltStr(col_and_owner, ";", tmp1);
    for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int owner = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_owner_.insert(make_pair(col, owner));
      LOG(INFO) << col << ":" << owner;
    }
    LOG(INFO) << col_and_owner;

    std::string col_and_dtype = param_map["Col_And_Dtype"].value_string();
    spiltStr(col_and_dtype, ";", tmp2);
    for (auto itr = tmp2.begin(); itr != tmp2.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int dtype = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_owner_.insert(make_pair(col, dtype));
      LOG(INFO) << col << ":" << dtype;
    }
    LOG(INFO) << col_and_dtype;
    expr_ = param_map["Expr"].value_string();
    LOG(INFO) << expr_;
    std::string parties = param_map["Parties"].value_string();
    spiltStr(parties, ";", tmp3);
    for (auto itr = tmp3.begin(); itr != tmp3.end(); itr++) {
      uint32_t party = std::atoi((*itr).c_str());
      parties_.push_back(party);
      LOG(INFO) << party;
    }
    LOG(INFO) << parties;
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  return 0;
}

int ArithmeticExecutor::loadDataset() {
  int ret = _LoadDatasetFromCSV(data_filepath_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }

  return 0;
}

int ArithmeticExecutor::initPartyComm(void) {
  // //   VLOG(3) << "Next addr: " << next_addr_.first << ":" <<
  // next_addr_.second
  // //           << ".";
  // //   VLOG(3) << "Prev addr: " << prev_addr_.first << ":" <<
  // prev_addr_.second
  // //           << ".";

  // //   if (local_id_ == 0) {
  // //     std::ostringstream ss;
  // //     ss << "sess_" << local_id_ << "_1";
  // //     std::string sess_name_1 = ss.str();

  // //     ss.str("");
  // //     ss << "sess_" << local_id_ << "_2";
  // //     std::string sess_name_2 = ss.str();

  // //     ep_next_.start(ios_, next_addr_.first, next_addr_.second,
  // //                    SessionMode::Server, sess_name_1);
  // //     LOG(INFO) << "[Next] Init server session, party " << local_id_ << ",
  // "
  // //               << "ip " << next_addr_.first << ", port " <<
  // next_addr_.second
  // //               << ", name " << sess_name_1 << ".";

  // //     ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
  // //                    SessionMode::Server, sess_name_2);
  // //     LOG(INFO) << "[Prev] Init server session, party " << local_id_ << ",
  // "
  // //               << "ip " << prev_addr_.first << ", port " <<
  // prev_addr_.second
  // //               << ", name " << sess_name_2 << ".";
  // //   } else if (local_id_ == 1) {
  // //     std::ostringstream ss;
  // //     ss << "sess_" << local_id_ << "_1";
  // //     std::string sess_name_1 = ss.str();
  // //     ss.str("");

  // //     ss << "sess_" << (local_id_ + 2) % 3 << "_1";
  // //     std::string sess_name_2 = ss.str();

  // //     ep_next_.start(ios_, next_addr_.first, next_addr_.second,
  // //                    SessionMode::Server, sess_name_1);
  // //     LOG(INFO) << "[Next] Init server session, party " << local_id_ << ",
  // "
  // //               << "ip " << next_addr_.first << ", port " <<
  // next_addr_.second
  // //               << ", name " << sess_name_1 << ".";

  // //     ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
  // //                    SessionMode::Client, sess_name_2);
  // //     LOG(INFO) << "[Prev] Init client session, party " << local_id_ << ",
  // "
  // //               << "ip " << prev_addr_.first << ", port " <<
  // prev_addr_.second
  // //               << ", name " << sess_name_2 << ".";
  // //   } else {
  // //     std::ostringstream ss;
  // //     ss.str("");
  // //     ss << "sess_" << (local_id_ + 1) % 3 << "_2";
  // //     std::string sess_name_1 = ss.str();

  // //     ss.str("");
  // //     ss << "sess_" << (local_id_ + 2) % 3 << "_1";
  // //     std::string sess_name_2 = ss.str();

  // //     ep_next_.start(ios_, next_addr_.first, next_addr_.second,
  // //                    SessionMode::Client, sess_name_1);
  // //     LOG(INFO) << "[Next] Init client session, party " << local_id_ << ",
  // "
  // //               << "ip " << next_addr_.first << ", port " <<
  // next_addr_.second
  // //               << ", name " << sess_name_1 << ".";

  // //     ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
  // //                    SessionMode::Client, sess_name_2);
  // //     LOG(INFO) << "[Prev] Init client session, party " << local_id_ << ",
  // "
  // //               << "ip " << prev_addr_.first << ", port " <<
  // prev_addr_.second
  // //               << ", name " << sess_name_2 << ".";
  //   }

  //   auto chann_next = ep_next_.addChannel();
  //   auto chann_prev = ep_prev_.addChannel();

  //   chann_next.waitForConnection();
  //   chann_prev.waitForConnection();

  //   chann_next.send(local_id_);
  //   chann_prev.send(local_id_);

  //   uint16_t prev_party = 0;
  //   uint16_t next_party = 0;
  //   chann_next.recv(next_party);
  //   chann_prev.recv(prev_party);

  //   if (next_party != (local_id_ + 1) % 3) {
  //     LOG(ERROR) << "Party " << local_id_ << ", expect next party id "
  //                << (local_id_ + 1) % 3 << ", but give " << next_party <<
  //                ".";
  //     return -3;
  //   }

  //   if (prev_party != (local_id_ + 2) % 3) {
  //     LOG(ERROR) << "Party " << local_id_ << ", expect prev party id "
  //                << (local_id_ + 2) % 3 << ", but give " << prev_party <<
  //                ".";
  //     return -3;
  //   }

  //   chann_next.close();
  //   chann_prev.close();

  //   engine_.init(local_id_, ep_prev_, ep_next_, toBlock(local_id_));
  //   LOG(INFO) << "Init party communication finish.";

  return 0;
}

int ArithmeticExecutor::execute() {
  //   sf64Matrix<D> w;
  //   sf64Matrix<D> train_data, train_label;
  //   sf64Matrix<D> test_data, test_label;

  //   int ret = _ConstructShares(w, train_data, train_label, test_data,
  //   test_label); if (ret) {
  //     finishPartyComm();
  //     return -1;
  //   }

  //   engine_.mPreproNext.resetStats();
  //   engine_.mPreproPrev.resetStats();

  //   model_ = logistic_main(train_data, train_label, w, test_data, test_label,
  //                          engine_, batch_size_, num_iter_, local_id_, false,
  //                          ep_prev_, ep_next_);

  //   LOG(INFO) << "Party " << local_id_ << " train finish.";
  return 0;
}

int ArithmeticExecutor::finishPartyComm(void) {
  //   ep_next_.stop();
  //   ep_prev_.stop();
  //   engine_.fini();
  delete mpc_exec_;
  return 0;
}

int ArithmeticExecutor::saveModel(void) {
  //   arrow::MemoryPool *pool = arrow::default_memory_pool();
  //   arrow::DoubleBuilder builder(pool);

  //   for (int i = 0; i < model_.rows(); i++)
  //     builder.Append(model_(i, 0));

  //   std::shared_ptr<arrow::Array> array;
  //   builder.Finish(&array);

  //   std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
  //       arrow::field("w", arrow::float64())};
  //   auto schema = std::make_shared<arrow::Schema>(schema_vector);
  //   std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema,
  //   {array});

  //   std::shared_ptr<DataDriver> driver =
  //       DataDirverFactory::getDriver("CSV",
  //       dataset_service_->getNodeletAddr());
  //   std::shared_ptr<CSVDriver> csv_driver =
  //       std::dynamic_pointer_cast<CSVDriver>(driver);

  //   std::string filepath = "data/" + model_name_ + ".csv";
  //   int ret = csv_driver->write(table, filepath);
  //   if (ret != 0) {
  //     LOG(ERROR) << "Save LR model to file " << filepath << " failed.";
  //     return -1;
  //   }
  //   LOG(INFO) << "Save model to " << filepath << ".";

  //   std::shared_ptr<Dataset> dataset = std::make_shared<Dataset>(table,
  //   driver); service::DatasetMeta meta(dataset, model_name_,
  //                             service::DatasetVisbility::PUBLIC);
  //   dataset_service_->regDataset(meta); //新的模型注册到分布式哈希表
  //   LOG(INFO) << "Register new dataset finish.";

  return 0;
}

int ArithmeticExecutor::_LoadDatasetFromCSV1(std::string &filename) {
  ifstream inFile(filename);
  string lineStr;
  vector<vector<float>> strArray;
  while (getline(inFile, lineStr)) {
    // cout << lineStr << endl;
    stringstream ss(lineStr);
    string str;
    vector<float> lineArray;
    while (getline(ss, str, ',')) {
      // string转float
      float str_float;
      istringstream istr(str);
      istr >> str_float;
      lineArray.push_back(str_float);
    }
    strArray.push_back(lineArray);
  }
}

int ArithmeticExecutor::_LoadDatasetFromCSV(std::string &filename) {
  std::string nodeaddr("test address"); // TODO
  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", nodeaddr);
  std::shared_ptr<Cursor> &cursor = driver->read(filename);
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
  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";

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

  m.resize(array_len, num_col);
  for (int i = 0; i < num_col - 1; i++) {
    auto array =
        std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
    for (int64_t j = 0; j < array->length(); j++)
      m(j, i) = array->Value(j);
  }
  auto array_lastCol = std::static_pointer_cast<arrow::Int64Array>(
      table->column(num_col - 1)->chunk(0));
  for (int64_t j = 0; j < array_lastCol->length(); j++) {
    m(j, num_col - 1) = array_lastCol->Value(j);
  }
  return array->length();
}

} // namespace primihub

// std::map<std::string, u32> col_and_owner_;
// col_and_owner.insert(std::make_pair("A", 0));
// col_and_owner.insert(std::make_pair("B", 0));
// col_and_owner.insert(std::make_pair("C", 1));
// col_and_owner.insert(std::make_pair("D", 2));

// std::map<std::string, bool> col_and_dtype_;
// col_and_dtype.insert(std::make_pair("A", false));
// col_and_dtype.insert(std::make_pair("B", true));
// col_and_dtype.insert(std::make_pair("C", false));
// col_and_dtype.insert(std::make_pair("D", true));
// for (auto &pair : col_and_owner)
//   mpc_exec->importColumnOwner(pair.first, pair.second);
// for (auto &pair : col_and_dtype)
//   mpc_exec->importColumnDtype(pair.first, pair.second);
// std::string expr_ = "A+B";
// mpc_exec->importExpress(expr_);
// mpc_exec->resolveRunMode();
// std::vector<uint32_t> parties_ = {0, 1};