/*
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "src/primihub/algorithm/arithmetic.h"

#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>

#include <string>
#include <memory>
#include <utility>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/network/message_interface.h"
#include "src/primihub/util/util.h"
#include "src/primihub/common/value_check_util.h"

using Array = arrow::Array;
using DoubleArray = arrow::DoubleArray;
using Int64Array = arrow::Int64Array;
using Table = arrow::Table;

namespace primihub {

template <Decimal Dbit>
ArithmeticExecutor<Dbit>::ArithmeticExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(config, dataset_service) {
  this->algorithm_name_ = "arithmetic";
  this->set_party_name(config.party_name());
  this->set_party_id(config.party_id());
  party_id_ = party_config_.SelfPartyId();
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::loadParams(primihub::rpc::Task &task) {
  auto ret = this->ExtractProxyNode(task, &this->proxy_node_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "extract proxy node failed";
    return -1;
  }
  LOG(INFO) << "party_name: " << this->party_name_;
  auto party_datasets = task.party_datasets();
  auto it = party_datasets.find(this->party_name());
  if (it == party_datasets.end()) {
    LOG(ERROR) << "no data set found for party name: " << this->party_name();
    return -1;
  }
  const auto &dataset = it->second.data();
  auto iter = dataset.find("Data_File");
  if (iter == dataset.end()) {
    LOG(ERROR) << "no dataset found for dataset name Data_File";
    return -1;
  }
  // File path.
  if (it->second.dataset_detail()) {
    this->is_dataset_detail_ = true;
    auto &param_map = task.params().param_map();
    auto p_it = param_map.find("Data_File");
    if (p_it != param_map.end()) {
      this->data_file_path_ = p_it->second.value_string();
      this->dataset_id_ = iter->second;
    } else {
      LOG(ERROR) << "no dataset id found";
      return -1;
    }
  } else {
    data_file_path_ = iter->second;
    this->dataset_id_ = iter->second;
  }

  LOG(INFO) << "Data file path is " << data_file_path_ << ".";
  auto param_map = task.params().param_map();
  try {
    const auto &task_info = task.task_info();
    task_id_ = task_info.task_id();
    job_id_ = task_info.job_id();

    // col_and_owner
    std::string col_and_owner = param_map["Col_And_Owner"].value_string();
    TrimAll(col_and_owner);
    std::vector<std::string> tmp1, tmp2, tmp3;
    str_split(col_and_owner, &tmp1, ';');
    for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      std::string party_name = itr->substr(pos + 1, itr->size());
      uint16_t owner;
      auto ret = party_config_.PartyName2PartyId(party_name, &owner);
      if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "convert party name to party id failed for: "
                   << party_name;
        return -1;
      }
      col_and_owner_.insert(make_pair(col, owner));
      // LOG(INFO) << col << ":" << owner;
    }
    // LOG(INFO) << col_and_owner;

    std::string col_and_dtype = param_map["Col_And_Dtype"].value_string();
    TrimAll(col_and_dtype);
    str_split(col_and_dtype, &tmp2, ';');
    for (auto itr = tmp2.begin(); itr != tmp2.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int dtype = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_dtype_.insert(make_pair(col, dtype));
      // LOG(INFO) << col << ":" << dtype;
    }
    // LOG(INFO) << col_and_dtype;

    expr_ = param_map["Expr"].value_string();
    TrimAll(expr_);
    int comma_index = expr_.find(",");
    cmp_col1 = expr_.substr(4, comma_index - 4);
    cmp_col2 = expr_.substr(comma_index + 1, expr_.length() - comma_index - 2);
    is_cmp = false;
    if (expr_.substr(0, 3) == "CMP") is_cmp = true;
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
    str_split(parties, &tmp3, ';');
    for (auto itr = tmp3.begin(); itr != tmp3.end(); itr++) {
      // uint32_t party = std::atoi((*itr).c_str());
      uint16_t party_id;
      auto ret = party_config_.PartyName2PartyId(*itr, &party_id);
      if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "convert party name to party id failed for: " << *itr;
        return -1;
      }
      parties_.push_back(party_id);
      // LOG(INFO) << party;
    }
    // LOG(INFO) << parties;

    res_name_ = param_map["ResFileName"].value_string();
    res_name_ = CompletePath(res_name_);
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  return 0;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::loadDataset() {
  int ret = _LoadDatasetFromCSV(this->dataset_id_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }

  if (is_cmp) {
    // When the data types of two columns are different,
    // conversion is possible.
    auto iter1 = col_and_dtype_.find(cmp_col1);
    if (iter1 == col_and_dtype_.end()) {
      std::stringstream ss;
      ss << "Can't find dtype of column " << cmp_col1 << ".";
      RaiseException(ss.str());
    }

    auto iter2 = col_and_dtype_.find(cmp_col2);
    if (iter2 == col_and_dtype_.end()) {
      std::stringstream ss;
      ss << "Can't find dtype of column " << cmp_col1 << ".";
      RaiseException(ss.str());
    }

    if (iter1->second == iter2->second) {
      if (iter1->second == 1)
        i64_cmp = false;
      else
        i64_cmp = true;

      return 0;
    }

    if (iter1->second == 0)
      LOG(INFO) << "Dtype of the compared column don't match, Convert dtype of "
                   "column "
                << cmp_col1 << " to double.";
    else
      LOG(INFO) << "Dtype of the compared column don't match, Convert dtype of "
                   "column "
                << cmp_col2 << " to double.";

    i64_cmp = false;

    std::string convert_col;
    if (iter1->second == 0)
      convert_col = cmp_col1;
    else
      convert_col = cmp_col2;

    auto iter3 = col_and_owner_.find(convert_col);
    if (iter3 == col_and_owner_.end()) {
      std::stringstream ss;
      ss << "Party: " << this->party_name()
         << ", Can't find the party that column "
         << convert_col << " belong to.";
      RaiseException(ss.str());
    }

    if (party_id_ != iter3->second) {
      LOG(INFO) << "Skip dtype convert because column " << convert_col
                << " don't belong to this party.";
      return 0;
    }

    auto col_iter = col_and_val_int.find(convert_col);
    if (col_iter == col_and_val_int.end()) {
      std::stringstream ss;
      ss << "Party: " << this->party_name() << ", "
         << "Can't find column value with column name " << convert_col << ".";
      RaiseException(ss.str());
    }

    std::vector<int64_t> &col_src = col_iter->second;
    std::vector<double> col_dest;

    for (auto &elem : col_src) col_dest.push_back(elem);

    col_and_val_double.insert(std::make_pair(convert_col, col_dest));
    col_and_val_int.erase(col_iter);

    LOG(INFO) << "Convert value of column " << convert_col
              << " to double finish.";
    return 0;
  }

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

template <Decimal Dbit>
retcode ArithmeticExecutor<Dbit>::InitEngine() {
  if (is_cmp) {
    // mpc_op_exec_->setup(next_ip_, prev_ip_, next_port_, prev_port_);
    mpc_op_exec_->setup(this->CommPkgPtr());
    return retcode::SUCCESS;
  }
  mpc_exec_->initMPCRuntime(this->party_id(), this->CommPkgPtr());
  return retcode::SUCCESS;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::execute() {
  if (is_cmp) {
    try {
      LOG(INFO) << "Run MPC Compare between " << cmp_col1 << " and " << cmp_col2
                << ".";

      sbMatrix sh_res;

      if (i64_cmp) {
        i64Matrix m;
        if (col_and_owner_[cmp_col1] == party_id_) {
          size_t count = col_and_val_int[cmp_col1].size();
          m.resize(1, count);

          std::vector<int64_t> &col = col_and_val_int[cmp_col1];
          for (size_t i = 0; i < count; i++)
            m(i) = col[i];

          mpc_op_exec_->MPC_Compare(m, sh_res);
        } else if (col_and_owner_[cmp_col2] == party_id_) {
          size_t count = col_and_val_int[cmp_col2].size();
          m.resize(1, count);

          std::vector<int64_t> &col = col_and_val_int[cmp_col2];
          for (size_t i = 0; i < count; i++)
            m(i) = col[i];

          mpc_op_exec_->MPC_Compare(m, sh_res);
        } else {
          mpc_op_exec_->MPC_Compare(sh_res);
        }
      } else {
        f64Matrix<Dbit> m;
        if (col_and_owner_[cmp_col1] == party_id_) {
          size_t count = col_and_val_double[cmp_col1].size();
          m.resize(1, count);

          std::vector<double> &col = col_and_val_double[cmp_col1];
          for (size_t i = 0; i < count; i++) m(i) = col[i];

          mpc_op_exec_->MPC_Compare(m, sh_res);
        } else if (col_and_owner_[cmp_col2] == party_id_) {
          size_t count = col_and_val_double[cmp_col2].size();
          m.resize(1, count);

          std::vector<double> &col = col_and_val_double[cmp_col2];
          for (size_t i = 0; i < count; i++) m(i) = col[i];

          mpc_op_exec_->MPC_Compare(m, sh_res);
        } else {
          mpc_op_exec_->MPC_Compare(sh_res);
        }
      }

      // reveal
      for (const auto &party : parties_) {
        if (party_id_ == party) {
          i64Matrix tmp = mpc_op_exec_->reveal(sh_res);
          for (i64 i = 0; i < tmp.rows(); i++)
            cmp_res_.emplace_back(static_cast<bool>(tmp(i, 0)));
        } else {
          mpc_op_exec_->reveal(sh_res, party);
        }
      }
    } catch (std::exception &e) {
      std::stringstream ss;
      ss << "Error occurs during MPC Compare, " << e.what();
      RaiseException(ss.str());
    }

    return 0;
  }

  try {
    std::stringstream ss;
    ss << "Reveal result to";
    for (auto &party : parties_) ss << " " << party;
    ss << ".";
    LOG(INFO) << ss.str();

    mpc_exec_->runMPCEvaluate();
    if (mpc_exec_->isFP64RunMode()) {
      mpc_exec_->revealMPCResult(parties_, final_val_double_);
    } else {
      mpc_exec_->revealMPCResult(parties_, final_val_int64_);
    }
  } catch (const std::exception &e) {
    std::stringstream ss;
    ss << "Error occurs during MPC run, " << e.what() << ".";
    RaiseException(ss.str());
  }

  return 0;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::saveModel(void) {
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
  if (final_val_double_.size() != 0) {
    for (size_t i = 0; i < final_val_double_.size(); i++) {
      builder.Append(final_val_double_[i]);
    }
  } else if (final_val_int64_.size() != 0) {
    for (size_t i = 0; i < final_val_int64_.size(); i++) {
      builder.Append(final_val_int64_[i]);
    }
  } else {
    for (size_t i = 0; i < cmp_res_.size(); i++) {
      builder.Append(cmp_res_[i]);
    }
  }

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
  // std::shared_ptr<CSVDriver> csv_driver =
  //     std::dynamic_pointer_cast<CSVDriver>(driver);
  auto &filepath = res_name_;
  auto data_cursor = driver->initCursor(filepath);
  auto dataset = std::make_shared<primihub::Dataset>(table, driver);
  // int ret = 0;
  // if (col_and_val_double.size() != 0)
  //   ret = csv_driver->write(table, filepath);
  // else
  //   ret = csv_driver->write(table, filepath);
  int ret = data_cursor->write(dataset);
  if (ret != 0) {
    LOG(ERROR) << "Save res to file " << filepath << " failed.";
    return -1;
  }
  LOG(INFO) << "Save res to " << filepath << ".";

  return 0;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::_LoadDatasetFromCSV(std::string &dataset_id) {
  auto driver =
      this->dataset_service_->getDriver(dataset_id, this->is_dataset_detail_);
  if (driver == nullptr) {
    LOG(ERROR) << "load dataset driver failed";
    return -1;
  }
  auto cursor = driver->read();
  if (cursor == nullptr) {
    LOG(ERROR) << "get data cursor failed";
    return -1;
  }
  std::shared_ptr<Dataset> ds = cursor->read();
  if (ds == nullptr) {
    LOG(ERROR) << "load dataset failed";
    return -1;
  }
  auto &table = std::get<std::shared_ptr<Table>>(ds->data);

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();

  bool errors = false;
  int num_col = table->num_columns();
  // 'array' include values in a column of csv file.
  int chunk_num = table->column(num_col - 1)->chunks().size();
  int64_t array_len = 0;
  for (int k = 0; k < chunk_num; k++) {
    // auto array = std::static_pointer_cast<DoubleArray>(
    //     table->column(num_col - 1)->chunk(k));
    auto array = table->column(num_col - 1)->chunk(k);
    array_len += array->length();
  }

  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";
  // Force the same value count in every column.

  for (int i = 0; i < num_col; i++) {
    auto& col_name = col_names[i];
    if (col_and_dtype_.find(col_name) == col_and_dtype_.end()) {
      continue;
    }
    int chunk_num = table->column(i)->chunks().size();
    auto col_data_type =
        table->schema()->GetFieldByName(col_name)->type()->id();
    if (col_and_dtype_[col_name] == 0) {
      if (col_data_type != arrow::Type::INT64) {
        RaiseException("Local data type is inconsistent with the demand data "
                       "type!Demand data type is int,but local data type is "
                       "double!Please input consistent data type!");
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
        std::stringstream ss;
        ss << "Party: " << this->party_name() << ", "
           << "Column " << col_name << " has " << tmp_len
           << " value, but other column has " << array_len << " value.";
        RaiseException(ss.str());
        errors = true;
        break;
      }
      col_and_val_int.insert(std::make_pair(col_name, tmp_data));
      // std::pair<std::string, std::vector<int64_t>>(col_names[i], tmp_data));
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
      if (col_data_type == arrow::Type::INT64) {
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
          std::stringstream ss;
          ss << "Party: " << this->party_name() << ", "
             << "Column " << col_name << " has " << tmp_len
             << " value, but other column has " << array_len << " value.";
          RaiseException(ss.str());
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
          std::stringstream ss;
          ss << "Party: " << this->party_name() << ", "
             << "Column " << col_name << " has " << tmp_len
             << " value, but other column has " << array_len << " value.";
          RaiseException(ss.str());
          errors = true;
          break;
        }
      }
      col_and_val_double.insert(std::make_pair(col_name, tmp_data));
      // pair<string, std::vector<double>>(col_names[i], tmp_data));
      // for (auto itr = col_and_val_double.begin();
      //      itr != col_and_val_double.end(); itr++) {
      //   LOG(INFO) << itr->first;
      //   auto tmp_vec = itr->second;
      //   for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
      //     LOG(INFO) << *iter;
      // }
    }
  }
  if (errors) return -1;

  return array_len;
}
template class ArithmeticExecutor<D32>;
template class ArithmeticExecutor<D16>;

// #ifndef MPC_SOCKET_CHANNEL
// MPCSendRecvExecutor::MPCSendRecvExecutor(
//     PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
//     : AlgorithmBase(dataset_service) {
//   std::ignore = dataset_service;
//   this->algorithm_name_ = "mpc_channel_sendrecv";

//   auto &node_map = config.node_map;

//   for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
//     if (iter->first == SCHEDULER_NODE) {
//       continue;
//     }

//     const rpc::Node &node = iter->second;
//     uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
//     partyid_node_map_[party_id] =
//         primihub::Node(node.ip(), node.port(), node.use_tls());
//     partyid_node_map_[party_id].id_ = node.node_id();

//     LOG(INFO) << "Party id " << party_id << ", node id " << node.node_id()
//               << ".";
//   }

//   auto iter = node_map.find(config.node_id);
//   if (iter == node_map.end()) {
//     std::stringstream ss;
//     ss << "Can't find node config with node id " << config.node_id << ".";
//     throw std::runtime_error(ss.str());
//   }

//   local_party_id_ = iter->second.vm(0).party_id();
//   local_node_.ip_ = iter->second.ip();
//   local_node_.port_ = iter->second.port();
//   local_node_.use_tls_ = iter->second.use_tls();
//   local_node_.id_ = iter->second.node_id();

//   next_party_id_ = (local_party_id_ + 1) % 3;
//   prev_party_id_ = (local_party_id_ + 2) % 3;
//   auto next_node = partyid_node_map_[next_party_id_];
//   auto prev_node = partyid_node_map_[prev_party_id_];

//   LOG(INFO) << "Local party: party id " << local_party_id_ << ", node "
//             << local_node_.to_string() << ", node id " << local_node_.id()
//             << ".";

//   LOG(INFO) << "Next party: party id " << next_party_id_ << ", node "
//             << next_node.to_string() << ", node id " << next_node.id()
//             << ".";

//   LOG(INFO) << "Prev party: party id " << prev_party_id_ << ", node "
//             << prev_node.to_string() << ", node id " << prev_node.id()
//             << ".";
// }

// int MPCSendRecvExecutor::initPartyComm(void) {
//   auto link_ctx = GetLinkContext();
//   if (link_ctx == nullptr) {
//     LOG(ERROR) << "link context is not available";
//     return -1;
//   }
//   // construct channel for next party
//   std::string next_party_name = this->party_config_.NextPartyName();
//   auto next_party_info = this->party_config_.NextPartyInfo();
//   auto base_channel_next = link_ctx->getChannel(next_party_info);

//   // construct channel for prev party
//   auto prev_party_name = this->party_config_.PrevPartyName();
//   auto prev_party_info = this->party_config_.PrevPartyInfo();
//   auto base_channel_prev = link_ctx->getChannel(prev_party_info);

//   // The 'osuCrypto::Channel' will consider it to be a unique_ptr and will
//   // reset the unique_ptr, so the 'osuCrypto::Channel' will delete it.
//   auto msg_interface_prev =
//   std::make_unique<network::TaskMessagePassInterface>(
//       link_ctx->job_id(), link_ctx->task_id(), link_ctx->request_id(),
//       this->party_name(), prev_party_name, link_ctx, base_channel_prev);

//   auto msg_interface_next =
//   std::make_unique<network::TaskMessagePassInterface>(
//       link_ctx->job_id(), link_ctx->task_id(), link_ctx->request_id(),
//       this->party_name(), next_party_name, link_ctx, base_channel_next);

//   osuCrypto::Channel chl_prev(ios_, msg_interface_prev.release());
//   osuCrypto::Channel chl_next(ios_, msg_interface_next.release());
//   auto com_pkg = std::make_shared<aby3::CommPkg>();
//   com_pkg->mPrev = std::move(chl_prev);
//   com_pkg->mNext = std::move(chl_next);
//   // auto link_ctx = this->GetLinkContext();
//   // if (link_ctx == nullptr) {
//   //   LOG(ERROR) << "link context is not available";
//   //   return -1;
//   // }
//   // base_channel_next_ =
//   //     link_ctx->getChannel(partyid_node_map_[next_party_id_]);
//   // base_channel_prev_ =
//   //     link_ctx->getChannel(partyid_node_map_[prev_party_id_]);

//   // mpc_channel_next_ = std::make_shared<MpcChannel>(
//   //     job_id_, task_id_, local_node_.id(), link_ctx);
//   // mpc_channel_prev_ = std::make_shared<MpcChannel>(
//   //     job_id_, task_id_, local_node_.id(), link_ctx);

//   // mpc_channel_next_->SetupBaseChannel(
//   //     partyid_node_map_[next_party_id_].id(), base_channel_next_);

//   // mpc_channel_prev_->SetupBaseChannel(
//   //     partyid_node_map_[prev_party_id_].id(), base_channel_prev_);

//   return 0;
// }

// int MPCSendRecvExecutor::execute() {

//   // Phase 1: simulate the communication in the creation of matrix's
//   arithmetic
//   // share.
//   LOG(INFO) << "Send and recv si64Matrix.";

//   {
//     si64Matrix sh_m(100, 100);

//     srand(100);
//     for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
//       sh_m.mShares[0](i) = rand();
//       sh_m.mShares[1](i) = 0;
//     }

//     mpc_channel_next_->asyncSendCopy(sh_m.mShares[0].data(),
//                                      sh_m.mShares[0].size());
//     auto fut = mpc_channel_prev_->asyncRecv(sh_m.mShares[1].data(),
//                                             sh_m.mShares[1].size());
//     fut.get();

//     for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
//       if (sh_m.mShares[0](i) != sh_m.mShares[1](i)) {
//         std::stringstream ss;
//         ss << "Find value mismatch, index " << i << ".";
//         LOG(ERROR) << ss.str();
//         throw std::runtime_error(ss.str());
//       }
//     }

//     mpc_channel_next_->asyncSendCopy(sh_m.mShares[0]);
//     fut = mpc_channel_prev_->asyncRecv(sh_m.mShares[1]);
//     fut.get();

//     for (uint64_t i = 0; i < sh_m.mShares[0].size(); i++) {
//       if (sh_m.mShares[0](i) != sh_m.mShares[1](i)) {
//         std::stringstream ss;
//         ss << "Find value mismatch, index " << i << ".";
//         LOG(ERROR) << ss.str();
//         throw std::runtime_error(ss.str());
//       }
//     }
//   }

//   LOG(INFO) << "Finish.";

//   // Phase 2: simulate the communication in the creation of matrix's binary
//   // share.
//   LOG(INFO) << "Send and recv sbMatrix.";
//   {
//     sbMatrix sh_bin_m(100, 64);

//     srand(100);
//     for (uint64_t i = 0; i < sh_bin_m.mShares[0].size(); i++) {
//       sh_bin_m.mShares[0](i) = rand();
//       sh_bin_m.mShares[1](i) = 0;
//     }

//     mpc_channel_next_->asyncSendCopy(sh_bin_m.mShares[0].data(),
//                                      sh_bin_m.mShares[0].size());
//     auto fut = mpc_channel_prev_->asyncRecv(sh_bin_m.mShares[1].data(),
//                                             sh_bin_m.mShares[1].size());
//     fut.get();

//     for (uint64_t i = 0; i < sh_bin_m.mShares[0].size(); i++) {
//       if (sh_bin_m.mShares[0](i) != sh_bin_m.mShares[1](i)) {
//         std::stringstream ss;
//         ss << "Find value mismatch, index " << i << ".";
//         LOG(ERROR) << ss.str();
//         throw std::runtime_error(ss.str());
//       }
//     }
//   }

//   LOG(INFO) << "Finish.";

//   // Phase 3: simulate the communicate in the creation of a value's
//   arithmetic
//   // share.
//   LOG(INFO) << "Send and recv si64.";
//   {
//     srand(100);
//     si64 sh_val1;
//     sh_val1.mData[0] = rand();
//     sh_val1.mData[1] = 0;

//     mpc_channel_next_->asyncSendCopy(sh_val1.mData[0]);
//     auto fut = mpc_channel_prev_->asyncRecv(sh_val1.mData[1]);
//     fut.get();

//     if (sh_val1.mData[0] != sh_val1.mData[1]) {
//       std::stringstream ss;
//       ss << "Find value mismatch.";
//       LOG(ERROR) << ss.str();
//       throw std::runtime_error(ss.str());
//     }
//   }

//   LOG(INFO) << "Finish.";

//   // Phase 4: simulate the communicate in the creation of a value's binary
//   // share.
//   LOG(INFO) << "Send and recv sb64.";
//   {
//     sb64 sh_val2;
//     sh_val2.mData[0] = 1;
//     sh_val2.mData[1] = 0;

//     mpc_channel_next_->asyncSendCopy(sh_val2.mData[0]);
//     auto fut = mpc_channel_prev_->asyncRecv(sh_val2.mData[1]);
//     fut.get();

//     if (sh_val2.mData[0] != sh_val2.mData[1]) {
//       std::stringstream ss;
//       ss << "Find value mismatch.";
//       LOG(ERROR) << ss.str();
//       throw std::runtime_error(ss.str());
//     }
//   }

//   LOG(INFO) << "Finish.";

//   std::string next_name = "fake_next";
//   std::string prev_name = "fake_prev";

//   mpc_op_ = std::make_unique<MPCOperator>(local_party_id_, next_name.c_str(),
//                                           prev_name.c_str());
//   mpc_op_->setup(*mpc_channel_prev_, *mpc_channel_next_);

//   LOG(INFO) << "Begin single value's secure share.";
//   {
//     si64 sh_val;
//     int64_t val = 10;

//     for (uint8_t i = 0; i < 3; i++) {
//       if (local_party_id_ == i)
//         mpc_op_->createShares(val, sh_val);
//       else
//         mpc_op_->createShares(sh_val);
//     }
//   }

//   LOG(INFO) << "finish.";

//   LOG(INFO) << "Begin matrix's secure share.";
//   {
//     uint16_t rows = 10;
//     uint16_t cols = 10;
//     eMatrix<double> m(rows, cols);
//     sf64Matrix<D8> sh_m(rows, cols);

//     srand(0);
//     for (uint16_t i = 0; i < rows; i++)
//       for (uint16_t j = 0; j < cols; j++)
//         m(i, j) = i;

//     if (local_party_id_ == 0)
//       mpc_op_->createShares(m, sh_m);
//     else
//       mpc_op_->createShares(sh_m);

//     LOG(INFO) << "Secure share finish.";

//     sf64Matrix<D8> sh_mul(rows, cols);
//     std::vector<sf64Matrix<D8>> sh_vals;
//     sh_vals.emplace_back(sh_m);
//     sh_vals.emplace_back(sh_m);
//     sh_mul = mpc_op_->MPC_Mul(sh_vals);

//     LOG(INFO) << "MPC Mul finish.";

//     eMatrix<double> mpc_result;
//     mpc_result = mpc_op_->revealAll(sh_mul);

//     eMatrix<double> plain_result = m * m;

//     for (uint16_t i = 0; i < rows; i++)
//       for (uint16_t j = 0; j < cols; j++)
//         if (round(mpc_result(i, j)) != plain_result(i, j))
//           LOG(INFO) << mpc_result(i, j) << "(MPC) vs " << plain_result(i, j)
//                     << " (Local).";
//   }

//   LOG(INFO) << "finish.";

//   return 0;
// }

// int MPCSendRecvExecutor::loadParams(rpc::Task &task) {
//   const auto& task_info = task.task_info();
//   task_id_ = task_info.task_id();
//   job_id_ = task_info.job_id();

//   return 0;
// }

// int MPCSendRecvExecutor::loadDataset(void) {
//   // Do nothing.
//   return 0;
// }

// int MPCSendRecvExecutor::finishPartyComm(void) {
//   // Do nothing.
//   return 0;
// }

// int MPCSendRecvExecutor::saveModel(void) {
//   // Do nothing.
//   return 0;
// }
// #endif

}  // namespace primihub
