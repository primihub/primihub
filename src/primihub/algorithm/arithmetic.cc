#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>

#include "src/primihub/algorithm/arithmetic.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/factory.h"

using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::Table;

namespace primihub {
void spiltStr(string str, const string &split, std::vector<string> &strlist) {
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

template <Decimal Dbit>
ArithmeticExecutor<Dbit>::ArithmeticExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "arithmetic";
  this->set_party_name(config.party_name());
  this->set_party_id(config.party_id());
#ifdef MPC_SOCKET_CHANNEL
  auto &node_map = config.node_map;
  // LOG(INFO) << node_map.size();
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

  party_id_ = iter->second.vm(0).party_id();
  LOG(INFO) << "Note party id of this node is " << party_id_ << ".";

  if (party_id_ == 0) {
    rpc::Node &node = party_id_node_map[0];

    next_ip_ = node.ip();
    next_port_ = node.vm(0).next().port();

    prev_ip_ = node.ip();
    prev_port_ = node.vm(0).prev().port();

  } else if (party_id_ == 1) {
    rpc::Node &node = party_id_node_map[1];

    // A local server addr.
    uint16_t port = node.vm(0).next().port();
    // next_addr_ = std::make_pair(node.ip(), port);

    // // A remote server addr.
    // prev_addr_ =
    //     std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());

    next_ip_ = node.ip();
    next_port_ = port;

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  } else {
    rpc::Node &node = party_id_node_map[2];

    // Two remote server addr.
    // next_addr_ =
    //     std::make_pair(node.vm(0).next().ip(), node.vm(0).next().port());
    // prev_addr_ =
    //     std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());

    next_ip_ = node.vm(0).next().ip();
    next_port_ = node.vm(0).next().port();

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  }
#else
  party_config_.Init(config);
#endif
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::loadParams(primihub::rpc::Task &task) {
  auto param_map = task.params().param_map();
  try {
    const auto& task_info = task.task_info();
    task_id_ = task_info.task_id();
    job_id_ = task_info.job_id();

    data_file_path_ = param_map["Data_File"].value_string();
    LOG(INFO) << "Data file path is " << data_file_path_ << ".";

    // col_and_owner
    std::string col_and_owner = param_map["Col_And_Owner"].value_string();
    std::vector<string> tmp1, tmp2, tmp3;
    spiltStr(col_and_owner, ";", tmp1);
    for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int owner = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_owner_.insert(make_pair(col, owner));
      // LOG(INFO) << col << ":" << owner;
    }
    // LOG(INFO) << col_and_owner;

    std::string col_and_dtype = param_map["Col_And_Dtype"].value_string();
    spiltStr(col_and_dtype, ";", tmp2);
    for (auto itr = tmp2.begin(); itr != tmp2.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int dtype = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_dtype_.insert(make_pair(col, dtype));
      // LOG(INFO) << col << ":" << dtype;
    }
    // LOG(INFO) << col_and_dtype;

    expr_ = param_map["Expr"].value_string();
    is_cmp = false;
    if (expr_.substr(0, 3) == "CMP")
      is_cmp = true;
    if (is_cmp) {
      std::string next_name;
      std::string prev_name;
      if (party_id_ == 0) {
        next_name = "01";
        prev_name = "02";
      } else if (party_id_ == 1) {
        next_name = "12";
        prev_name = "01";
      } else if (party_id_ == 2) {
        next_name = "02";
        prev_name = "12";
      }

      mpc_op_exec_ =
          std::make_unique<MPCOperator>(party_id_, next_name, prev_name);
    } else {
      mpc_exec_ = std::make_unique<MPCExpressExecutor<Dbit>>();
    }

    std::string parties = param_map["RevealToParties"].value_string();
    spiltStr(parties, ";", tmp3);
    for (auto itr = tmp3.begin(); itr != tmp3.end(); itr++) {
      uint32_t party = std::atoi((*itr).c_str());
      parties_.push_back(party);
      // LOG(INFO) << party;
    }
    // LOG(INFO) << parties;

    res_name_ = param_map["ResFileName"].value_string();
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::loadDataset() {
  int ret = _LoadDatasetFromCSV(data_file_path_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }

  if (is_cmp)
    return 0;

  mpc_exec_->initColumnConfig(party_id_);
  for (auto &pair : col_and_owner_)
    mpc_exec_->importColumnOwner(pair.first, pair.second);

  for (auto &pair : col_and_dtype_)
    mpc_exec_->importColumnDtype(pair.first, pair.second);

  mpc_exec_->importExpress(expr_);
  mpc_exec_->resolveRunMode();
  mpc_exec_->InitFeedDict();

  if (col_and_val_double.size() != 0) {
    for (auto &pair : col_and_val_double)
      mpc_exec_->importColumnValues(pair.first, pair.second);
  } else {
    for (auto &pair : col_and_val_int)
      mpc_exec_->importColumnValues(pair.first, pair.second);
  }

  return 0;
}

#ifdef MPC_SOCKET_CHANNEL
template <Decimal Dbit> int ArithmeticExecutor<Dbit>::initPartyComm(void) {
  if (is_cmp) {
    mpc_op_exec_->setup(next_ip_, prev_ip_, next_port_, prev_port_);
    return 0;
  }

  mpc_exec_->initMPCRuntime(party_id_, next_ip_, prev_ip_, next_port_,
                            prev_port_);
  return 0;
}
#else
template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::initPartyComm(void) {
  auto link_ctx = this->GetLinkContext();
  if (link_ctx == nullptr) {
    LOG(ERROR) << "link context is unavailable";
    return -1;
  }
  // construct channel for next party
  std::string next_party_name = this->party_config_.NextPartyName();
  Node next_party_info = this->party_config_.NextPartyInfo();
  // construct channel for prev party
  std::string prev_party_name = this->party_config_.PrevPartyName();
  Node prev_party_info = this->party_config_.PrevPartyInfo();

  base_channel_next_ = link_ctx->getChannel(next_party_info);

  base_channel_prev_ = link_ctx->getChannel(prev_party_info);

  mpc_channel_next_ = std::make_shared<MpcChannel>(
      party_config_.SelfPartyName(), link_ctx);
  mpc_channel_prev_ = std::make_shared<MpcChannel>(
      party_config_.SelfPartyName(), link_ctx);

  mpc_channel_next_->SetupBaseChannel(next_party_name, base_channel_next_);

  mpc_channel_prev_->SetupBaseChannel(prev_party_name, base_channel_prev_);
  LOG(INFO) << "local_id_local_id_: " << party_config_.SelfPartyId();
  LOG(INFO) << "next_party: " << next_party_name
      << " detail: " << next_party_info.to_string();
  LOG(INFO) << "prev_party: " << prev_party_name
      << " detail: " << prev_party_info.to_string();
  if (is_cmp) {
    mpc_op_exec_->setup(*mpc_channel_prev_, *mpc_channel_next_);
    return 0;
  }
  mpc_exec_->initMPCRuntime(party_id_, *mpc_channel_prev_, *mpc_channel_next_);
  return 0;
}
#endif

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::execute() {
  if (is_cmp) {
    try {
      sbMatrix sh_res;
      f64Matrix<Dbit> m;
      if (col_and_owner_[expr_.substr(4, 1)] == party_id_) {
        m.resize(1, col_and_val_double[expr_.substr(4, 1)].size());
        for (size_t i = 0; i < col_and_val_double[expr_.substr(4, 1)].size();
             i++)
          m(i) = col_and_val_double[expr_.substr(4, 1)][i];
        mpc_op_exec_->MPC_Compare(m, sh_res);
      } else if (col_and_owner_[expr_.substr(6, 1)] == party_id_) {
        m.resize(1, col_and_val_double[expr_.substr(6, 1)].size());
        for (size_t i = 0; i < col_and_val_double[expr_.substr(6, 1)].size();
             i++)
          m(i) = col_and_val_double[expr_.substr(6, 1)][i];
        mpc_op_exec_->MPC_Compare(m, sh_res);
      } else {
        mpc_op_exec_->MPC_Compare(sh_res);
      }

      // reveal
      for (const auto &party : parties_) {
        if (party_id_ == party) {
          i64Matrix tmp = mpc_op_exec_->reveal(sh_res);
          for (size_t i = 0; i < tmp.rows(); i++)
            cmp_res_.emplace_back(static_cast<bool>(tmp(i, 0)));
        } else {
          mpc_op_exec_->reveal(sh_res, party);
        }
      }
    } catch (std::exception &e) {
      LOG(ERROR) << "In party " << party_id_ << ":\n" << e.what() << ".";
    }
    return 0;
  }

  try {
    std::stringstream ss;
    ss << "Reveal result to";
    for (auto &party : parties_)
      ss << " " << party;
    ss << ".";
    LOG(INFO) << ss.str();

    mpc_exec_->runMPCEvaluate();
    if (mpc_exec_->isFP64RunMode()) {
      mpc_exec_->revealMPCResult(parties_, final_val_double_);
    } else {
      mpc_exec_->revealMPCResult(parties_, final_val_int64_);
    }
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }
  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::finishPartyComm(void) {
  if (is_cmp) {
    mpc_op_exec_->fini();
    mpc_op_exec_.reset();
  }

  mpc_exec_.reset();
  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::saveModel(void) {
  bool is_reveal = false;
  for (auto party : parties_) {
    if (party == party_id_) {
      is_reveal = true;
      break;
    }
  }
  if (!is_reveal) {
    return 0;
  }
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::DoubleBuilder builder(pool);
  if (final_val_double_.size() != 0)
    for (int i = 0; i < final_val_double_.size(); i++)
      builder.Append(final_val_double_[i]);
  else if (final_val_int64_.size() != 0)
    for (int i = 0; i < final_val_int64_.size(); i++)
      builder.Append(final_val_int64_[i]);
  else
    for (int i = 0; i < cmp_res_.size(); i++)
      builder.Append(cmp_res_[i]);
  std::shared_ptr<arrow::Array> array;
  builder.Finish(&array);

  std::vector<std::shared_ptr<arrow::Field>> schema_vector_double = {
      arrow::field(expr_, arrow::float64())};
  std::vector<std::shared_ptr<arrow::Field>> schema_vector_int64 = {
      arrow::field(expr_, arrow::int64())};
  std::vector<std::shared_ptr<arrow::Field>> schema_vector_bool = {
      arrow::field(expr_, arrow::boolean())};

  std::shared_ptr<arrow::Table> table;
  if (final_val_double_.size() != 0)
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_double), {array});
  else if (final_val_int64_.size() != 0)
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_int64), {array});
  else
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_bool), {array});
  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());
  std::shared_ptr<CSVDriver> csv_driver =
      std::dynamic_pointer_cast<CSVDriver>(driver);

  std::string filepath = "data/" + res_name_ + ".csv";
  int ret = 0;
  if (col_and_val_double.size() != 0)
    ret = csv_driver->write(table, filepath);
  else
    ret = csv_driver->write(table, filepath);
  if (ret != 0) {
    LOG(ERROR) << "Save res to file " << filepath << " failed.";
    return -1;
  }
  LOG(INFO) << "Save res to " << filepath << ".";

  return 0;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::_LoadDatasetFromCSV(std::string &dataset_id) {
  auto driver = this->dataset_service_->getDriver(dataset_id);
  auto cursor = driver->read();
  std::shared_ptr<Dataset> ds = cursor->read();
  std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();

  bool errors = false;
  int num_col = table->num_columns();
  // 'array' include values in a column of csv file.
  int chunk_num = table->column(num_col - 1)->chunks().size();
  int64_t array_len = 0;
  for (int k = 0; k < chunk_num; k++) {
    auto array = std::static_pointer_cast<DoubleArray>(
        table->column(num_col - 1)->chunk(k));
    array_len += array->length();
  }

  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";
  // Force the same value count in every column.

  for (int i = 0; i < num_col; i++) {
    int chunk_num = table->column(i)->chunks().size();
    if (col_and_dtype_[col_names[i]] == 0) {
      if (table->schema()->GetFieldByName(col_names[i])->type()->id() != 9) {
        LOG(ERROR) << "Local data type is inconsistent with the demand data "
                      "type!Demand data type is int,but local data type is "
                      "double!Please input consistent data type!";
        return -1;
      }
      std::vector<int64_t> tmp_data;
      int64_t tmp_len = 0;
      for (int k = 0; k < chunk_num; k++) {
        auto array =
            std::static_pointer_cast<Int64Array>(table->column(i)->chunk(k));
        tmp_len += array->length();
        for (int64_t j = 0; j < array->length(); j++) {
          tmp_data.push_back(array->Value(j));
          // LOG(INFO) << array->Value(j);
        }
      }
      if (tmp_len != array_len) {
        LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                   << " value, but other column has " << array_len << " value.";
        errors = true;
        break;
      }
      col_and_val_int.insert(
          pair<string, std::vector<int64_t>>(col_names[i], tmp_data));
      // for (auto itr = col_and_val_int.begin(); itr != col_and_val_int.end();
      //      itr++) {
      //   LOG(INFO) << itr->first;
      //   auto tmp_vec = itr->second;
      //   for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
      //     LOG(INFO) << *iter;
      // }
    } else {
      std::vector<double> tmp_data;
      int64_t tmp_len = 0;
      if (table->schema()->GetFieldByName(col_names[i])->type()->id() == 9) {
        for (int k = 0; k < chunk_num; k++) {
          auto array =
              std::static_pointer_cast<Int64Array>(table->column(i)->chunk(k));
          tmp_len += array->length();
          for (int64_t j = 0; j < array->length(); j++) {
            tmp_data.push_back(array->Value(j));
            // LOG(INFO) << array->Value(j);
          }
        }
        if (tmp_len != array_len) {
          LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                     << " value, but other column has " << array_len
                     << " value.";
          errors = true;
          break;
        }
      } else {
        for (int k = 0; k < chunk_num; k++) {
          auto array =
              std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(k));
          tmp_len += array->length();
          for (int64_t j = 0; j < array->length(); j++) {
            tmp_data.push_back(array->Value(j));
            // LOG(INFO) << array->Value(j);
          }
        }
        if (tmp_len != array_len) {
          LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                     << " value, but other column has " << array_len
                     << " value.";
          errors = true;
          break;
        }
      }
      col_and_val_double.insert(
          pair<string, std::vector<double>>(col_names[i], tmp_data));
      // for (auto itr = col_and_val_double.begin();
      //      itr != col_and_val_double.end(); itr++) {
      //   LOG(INFO) << itr->first;
      //   auto tmp_vec = itr->second;
      //   for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
      //     LOG(INFO) << *iter;
      // }
    }
  }
  if (errors)
    return -1;

  return array_len;
}
template class ArithmeticExecutor<D32>;
template class ArithmeticExecutor<D16>;

#ifndef MPC_SOCKET_CHANNEL
MPCSendRecvExecutor::MPCSendRecvExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  std::ignore = dataset_service;
  this->algorithm_name_ = "mpc_channel_sendrecv";

  auto &node_map = config.node_map;

  for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
    if (iter->first == SCHEDULER_NODE) {
      continue;
    }

    const rpc::Node &node = iter->second;
    uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
    partyid_node_map_[party_id] =
        primihub::Node(node.ip(), node.port(), node.use_tls());
    partyid_node_map_[party_id].id_ = node.node_id();

    LOG(INFO) << "Party id " << party_id << ", node id " << node.node_id()
              << ".";
  }

  auto iter = node_map.find(config.node_id);
  if (iter == node_map.end()) {
    std::stringstream ss;
    ss << "Can't find node config with node id " << config.node_id << ".";
    throw std::runtime_error(ss.str());
  }

  local_party_id_ = iter->second.vm(0).party_id();
  local_node_.ip_ = iter->second.ip();
  local_node_.port_ = iter->second.port();
  local_node_.use_tls_ = iter->second.use_tls();
  local_node_.id_ = iter->second.node_id();

  next_party_id_ = (local_party_id_ + 1) % 3;
  prev_party_id_ = (local_party_id_ + 2) % 3;
  auto next_node = partyid_node_map_[next_party_id_];
  auto prev_node = partyid_node_map_[prev_party_id_];

  LOG(INFO) << "Local party: party id " << local_party_id_ << ", node "
            << local_node_.to_string() << ", node id " << local_node_.id()
            << ".";

  LOG(INFO) << "Next party: party id " << next_party_id_ << ", node "
            << next_node.to_string() << ", node id " << next_node.id()
            << ".";

  LOG(INFO) << "Prev party: party id " << prev_party_id_ << ", node "
            << prev_node.to_string() << ", node id " << prev_node.id()
            << ".";
}

int MPCSendRecvExecutor::initPartyComm(void) {
  auto link_ctx = this->GetLinkContext();
  if (link_ctx == nullptr) {
    LOG(ERROR) << "link context is not available";
    return -1;
  }
  base_channel_next_ =
      link_ctx->getChannel(partyid_node_map_[next_party_id_]);
  base_channel_prev_ =
      link_ctx->getChannel(partyid_node_map_[prev_party_id_]);

  mpc_channel_next_ = std::make_shared<MpcChannel>(
      job_id_, task_id_, local_node_.id(), link_ctx);
  mpc_channel_prev_ = std::make_shared<MpcChannel>(
      job_id_, task_id_, local_node_.id(), link_ctx);

  mpc_channel_next_->SetupBaseChannel(
      partyid_node_map_[next_party_id_].id(), base_channel_next_);

  mpc_channel_prev_->SetupBaseChannel(
      partyid_node_map_[prev_party_id_].id(), base_channel_prev_);

  return 0;
}

int MPCSendRecvExecutor::execute() {

  // Phase 1: simulate the communication in the creation of matrix's arithmetic
  // share.
  LOG(INFO) << "Send and recv si64Matrix.";

  {
    si64Matrix sh_m(100, 100);

    srand(100);
    for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
      sh_m.mShares[0](i) = rand();
      sh_m.mShares[1](i) = 0;
    }

    mpc_channel_next_->asyncSendCopy(sh_m.mShares[0].data(),
                                     sh_m.mShares[0].size());
    auto fut = mpc_channel_prev_->asyncRecv(sh_m.mShares[1].data(),
                                            sh_m.mShares[1].size());
    fut.get();

    for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
      if (sh_m.mShares[0](i) != sh_m.mShares[1](i)) {
        std::stringstream ss;
        ss << "Find value mismatch, index " << i << ".";
        LOG(ERROR) << ss.str();
        throw std::runtime_error(ss.str());
      }
    }

    mpc_channel_next_->asyncSendCopy(sh_m.mShares[0]);
    fut = mpc_channel_prev_->asyncRecv(sh_m.mShares[1]);
    fut.get();

    for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
      if (sh_m.mShares[0](i) != sh_m.mShares[1](i)) {
        std::stringstream ss;
        ss << "Find value mismatch, index " << i << ".";
        LOG(ERROR) << ss.str();
        throw std::runtime_error(ss.str());
      }
    }
  }

  LOG(INFO) << "Finish.";

  // Phase 2: simulate the communication in the creation of matrix's binary
  // share.
  LOG(INFO) << "Send and recv sbMatrix.";
  {
    sbMatrix sh_bin_m(100, 64);

    srand(100);
    for (uint64_t i = 0; i < sh_bin_m.mShares[0].size(); i++) {
      sh_bin_m.mShares[0](i) = rand();
      sh_bin_m.mShares[1](i) = 0;
    }

    mpc_channel_next_->asyncSendCopy(sh_bin_m.mShares[0].data(),
                                     sh_bin_m.mShares[0].size());
    auto fut = mpc_channel_prev_->asyncRecv(sh_bin_m.mShares[1].data(),
                                            sh_bin_m.mShares[1].size());
    fut.get();

    for (uint64_t i = 0; i < sh_bin_m.mShares[0].size(); i++) {
      if (sh_bin_m.mShares[0](i) != sh_bin_m.mShares[1](i)) {
        std::stringstream ss;
        ss << "Find value mismatch, index " << i << ".";
        LOG(ERROR) << ss.str();
        throw std::runtime_error(ss.str());
      }
    }
  }

  LOG(INFO) << "Finish.";

  // Phase 3: simulate the communicate in the creatin of a value's arithmetic
  // share.
  LOG(INFO) << "Send and recv si64.";
  {
    srand(100);
    si64 sh_val1;
    sh_val1.mData[0] = rand();
    sh_val1.mData[1] = 0;

    mpc_channel_next_->asyncSendCopy(sh_val1.mData[0]);
    auto fut = mpc_channel_prev_->asyncRecv(sh_val1.mData[1]);
    fut.get();

    if (sh_val1.mData[0] != sh_val1.mData[1]) {
      std::stringstream ss;
      ss << "Find value mismatch.";
      LOG(ERROR) << ss.str();
      throw std::runtime_error(ss.str());
    }
  }

  LOG(INFO) << "Finish.";

  // Phase 4: simulate the communicate in the creation of a value's binary
  // share.
  LOG(INFO) << "Send and recv sb64.";
  {
    sb64 sh_val2;
    sh_val2.mData[0] = 1;
    sh_val2.mData[1] = 0;

    mpc_channel_next_->asyncSendCopy(sh_val2.mData[0]);
    auto fut = mpc_channel_prev_->asyncRecv(sh_val2.mData[1]);
    fut.get();

    if (sh_val2.mData[0] != sh_val2.mData[1]) {
      std::stringstream ss;
      ss << "Find value mismatch.";
      LOG(ERROR) << ss.str();
      throw std::runtime_error(ss.str());
    }
  }

  LOG(INFO) << "Finish.";

  std::string next_name = "fake_next";
  std::string prev_name = "fake_prev";

  mpc_op_ = std::make_unique<MPCOperator>(local_party_id_, next_name.c_str(),
                                          prev_name.c_str());
  mpc_op_->setup(*mpc_channel_prev_, *mpc_channel_next_);

  LOG(INFO) << "Begin single value's secure share.";
  {
    si64 sh_val;
    int64_t val = 10;

    for (uint8_t i = 0; i < 3; i++) {
      if (local_party_id_ == i)
        mpc_op_->createShares(val, sh_val);
      else
        mpc_op_->createShares(sh_val);
    }
  }

  LOG(INFO) << "finish.";

  LOG(INFO) << "Begin matrix's secure share.";
  {
    uint16_t rows = 10;
    uint16_t cols = 10;
    eMatrix<double> m(rows, cols);
    sf64Matrix<D8> sh_m(rows, cols);

    srand(0);
    for (uint16_t i = 0; i < rows; i++)
      for (uint16_t j = 0; j < cols; j++)
        m(i, j) = i;

    if (local_party_id_ == 0)
      mpc_op_->createShares(m, sh_m);
    else
      mpc_op_->createShares(sh_m);

    LOG(INFO) << "Secure share finish.";

    sf64Matrix<D8> sh_mul(rows, cols);
    std::vector<sf64Matrix<D8>> sh_vals;
    sh_vals.emplace_back(sh_m);
    sh_vals.emplace_back(sh_m);
    sh_mul = mpc_op_->MPC_Mul(sh_vals);

    LOG(INFO) << "MPC Mul finish.";

    eMatrix<double> mpc_result;
    mpc_result = mpc_op_->revealAll(sh_mul);

    eMatrix<double> plain_result = m * m;

    for (uint16_t i = 0; i < rows; i++)
      for (uint16_t j = 0; j < cols; j++)
        if (round(mpc_result(i, j)) != plain_result(i, j))
          LOG(INFO) << mpc_result(i, j) << "(MPC) vs " << plain_result(i, j)
                    << " (Local).";
  }

  LOG(INFO) << "finish.";

  return 0;
}

int MPCSendRecvExecutor::loadParams(rpc::Task &task) {
  const auto& task_info = task.task_info();
  task_id_ = task_info.task_id();
  job_id_ = task_info.job_id();

  return 0;
}

int MPCSendRecvExecutor::loadDataset(void) {
  // Do nothing.
  return 0;
}

int MPCSendRecvExecutor::finishPartyComm(void) {
  // Do nothing.
  return 0;
}

int MPCSendRecvExecutor::saveModel(void) {
  // Do nothing.
  return 0;
}
#endif

} // namespace primihub
