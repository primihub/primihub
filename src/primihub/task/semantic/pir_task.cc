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

#include "src/primihub/task/semantic/pir_task.h"
#include <glog/logging.h>

#include <set>
#include <utility>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

#include "src/primihub/kernel/pir/operator/base_pir.h"
#include "src/primihub/kernel/pir/operator/factory.h"
#include "src/primihub/common/config/server_config.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub::task {
PirTask::PirTask(const TaskParam* task_param,
                 std::shared_ptr<DatasetService> dataset_service)
                 : TaskBase(task_param, dataset_service) {}

retcode PirTask::BuildOptions(const rpc::Task& task, pir::Options* options) {
  // build Options for operator
  this->ParsePirRole(task, &(options->role));
  options->self_party = this->party_name();
  options->link_ctx_ref = getTaskContext().getLinkContext().get();
  options->code = task.code();
  auto& party_info = options->party_info;
  const auto& pb_party_info = task.party_access_info();
  for (const auto& [_party_name, pb_node] : pb_party_info) {
    Node node_info;
    pbNode2Node(pb_node, &node_info);
    party_info[_party_name] = std::move(node_info);
  }

  if (RoleValidation::IsServer(this->party_name())) {
    // parameter for offline generate db info
    const auto& param_map = task.params().param_map();
    auto iter = param_map.find("DbInfo");
    if (iter != param_map.end()) {
      options->db_path = iter->second.value_string();
      LOG(INFO) << "db_file_cache path: " << options->db_path;
      if (this->dataset_id_.empty()) {
        LOG(ERROR) << "dataset id is empty for party: " << party_name();
        return retcode::FAIL;
      }
      ValidateDir(options->db_path);
      options->generate_db = true;
    } else {
      // parameter for online task
      if (this->dataset_id_.empty()) {
        LOG(ERROR) << "dataset id is empty for party: " << party_name();
        return retcode::FAIL;
      }
      options->db_path = db_cache_dir_ + "/";
      // check db cache exist or not
      if (is_dataset_detail_) {
        auto it = param_map.find(party_name());
        if (it != param_map.end()) {
          options->db_path.append(it->second.value_string());

        } else {
          LOG(ERROR) << "dateset id is not set";
          return retcode::FAIL;
        }
      } else {
        options->db_path.append(this->dataset_id_);
      }
      for (const auto key_index : this->server_key_columns_) {
        options->db_path.append("_").append(std::to_string(key_index));
      }

      if (DbCacheAvailable(options->db_path)) {
        options->use_cache = true;
      }
    }
  }
  // peer node info
  std::string peer_party_name;
  if (RoleValidation::IsServer(this->party_name())) {
    peer_party_name = PARTY_CLIENT;
  } else if (RoleValidation::IsClient(this->party_name())) {
     peer_party_name = PARTY_SERVER;
  } else {
    LOG(ERROR) << "invalid party: " << this->party_name();
  }
  auto it = party_info.find(peer_party_name);
  if (it != party_info.end()) {
    options->peer_node = it->second;
  } else {
    LOG(WARNING) << "find peer node info failed for party: " << party_name();
  }
  // proxy node
  auto ret = this->ExtractProxyNode(task, &options->proxy_node);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "get proxy node failed";
    return retcode::FAIL;
  }
  // end of build Options
  return retcode::SUCCESS;
}

retcode PirTask::ParseQueryConfig(const rpc::Task& task_config) {
  GetServerDataSetSchema(task_config);
  const auto& param_map = task_config.params().param_map();
  // parse key colum and label colums
  auto it = param_map.find("QueryConfig");
  if (it == param_map.end()) {
    LOG(WARNING) << "no QueryConfig found, "
                 << "Server side using column 0 as default keyword";
    // compatible with no queryconfig set
    if (this->server_key_columns_.empty()) {
      this->server_key_columns_.push_back(0);
    }
    if (this->server_label_columns_.empty()) {
      size_t col_count = server_dataset_schema_.size();
      for (size_t i = 0; i < col_count; i++) {
        this->server_label_columns_.push_back(i);
      }
    }
    LOG(ERROR) << "server_key_columns_: " << server_key_columns_.size() << " "
               << "server_label_columns_: " << server_label_columns_.size();
    return retcode::SUCCESS;
  }

  auto& conf_str = it->second.value_string();
  nlohmann::json query_conf = nlohmann::json::parse(conf_str);
  // get server query config
  if (query_conf.contains(PARTY_SERVER)) {
    auto& query_info = query_conf[PARTY_SERVER];
    // get key
    if (query_info.contains("key_columns")) {
      auto& keys = query_info["key_columns"];
      std::set<int> dup;
      for (auto& key_index : keys) {
        if (dup.find(key_index) != dup.end()) {
          continue;
        }
        dup.insert(key_index.get<int>());
        this->server_key_columns_.push_back(key_index);
      }
    }
    // get label, label is all fields
    if (this->server_label_columns_.empty()) {
      size_t total_colums = server_dataset_schema_.size();
      for (size_t i = 0; i < total_colums; i++) {
        this->server_label_columns_.push_back(i);
      }
    }
    if (this->server_label_columns_.empty()) {
      RaiseException("No label columns secififed for Server side");
    }
  }
  // get client query config
  if (query_conf.contains(PARTY_CLIENT)) {
    auto& query_info = query_conf[PARTY_CLIENT];
    // get key
    if (query_info.contains("key_columns")) {
      auto& keys = query_info["key_columns"];
      for (auto& key_index : keys) {
        this->client_key_columns_.push_back(key_index);
      }
    }
  }
  return retcode::SUCCESS;
}

retcode PirTask::ParseDataset(const rpc::Task& task_config) {
  const auto& party_datasets = task_config.party_datasets();
  auto dataset_it = party_datasets.find(party_name());
  if (dataset_it != party_datasets.end()) {
    const auto& datasets_map = dataset_it->second.data();
    auto it = datasets_map.find(party_name());
    if (it == datasets_map.end()) {
      LOG(WARNING) << "no datasets is set for party: " << party_name();
    } else {
      dataset_id_ = it->second;
      VLOG(5) << "data set id: " << dataset_id_;
    }
    if (dataset_it->second.dataset_detail()) {
      is_dataset_detail_ = true;
    }
  }
  return retcode::SUCCESS;
}

retcode PirTask::ParsePirRole(const rpc::Task& task_config, Role* role) {
  if (this->party_name() == PARTY_CLIENT) {
    *role = Role::CLIENT;
  } else if (this->party_name() == PARTY_SERVER) {
    *role = Role::SERVER;
  } else {
    LOG(ERROR) << "no suitable role assign for party: " << this->party_name();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode PirTask::ParsePirType(const rpc::Task& task_config) {
  const auto& param_map = task_config.params().param_map();
  auto iter = param_map.find("pirType");
  if (iter != param_map.end()) {
    pir_type_ = iter->second.value_int32();
  }
  return retcode::SUCCESS;
}

retcode PirTask::ParseResultPathConfig(const rpc::Task& task_config) {
  const auto& param_map = task_config.params().param_map();
  if (RoleValidation::IsClient(this->party_name())) {
    VLOG(7) << "dataset_id: " << dataset_id_;
    auto it = param_map.find("outputFullFilename");
    if (it != param_map.end()) {
      result_file_path_ = CompletePath(it->second.value_string());
      VLOG(5) << "result_file_path_: " << result_file_path_;
    } else  {
      LOG(ERROR) << "no keyword outputFullFilename match";
      return retcode::FAIL;
    }
  }
  return retcode::SUCCESS;
}

retcode PirTask::LoadParams(const rpc::Task& task) {
  ParsePirType(task);
  ParseQueryConfig(task);
  ParseDataset(task);
  auto ret = BuildOptions(task, &this->options_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "build operator options for party: "
               << party_name() << " failed";
    return retcode::FAIL;
  }
  ParseResultPathConfig(task);
  return retcode::SUCCESS;
}

retcode PirTask::GetServerDataSetSchema(const rpc::Task& task) {
  // get server dataset id
  const auto& party_datasets = task.party_datasets();
  auto it = party_datasets.find(PARTY_SERVER);
  if (it == party_datasets.end()) {
    LOG(WARNING) << "no dataset found for party_name: " << PARTY_SERVER;
    return retcode::FAIL;
  }
  const auto& datasets_map = it->second.data();
  auto iter = datasets_map.find(PARTY_SERVER);
  if (iter == datasets_map.end()) {
    LOG(WARNING) << "no dataset found for party_name: " << PARTY_SERVER;
    return retcode::FAIL;
  }

  if (it->second.dataset_detail()) {
    auto& dataset_access = iter->second;
    nlohmann::json acc_info = nlohmann::json::parse(dataset_access);
    auto schema_str = acc_info["schema"].get<std::string>();
    VLOG(0) << "Server Dataset Schema: " << schema_str;
    nlohmann::json filed_list = nlohmann::json::parse(schema_str);
    for (const auto& filed : filed_list) {
      for (const auto& [name, _] : filed.items()) {
        server_dataset_schema_.push_back(name);
      }
    }
  } else {
    auto& server_dataset_id = iter->second;
    auto& dataset_service = this->getDatasetService();
    auto driver = dataset_service->getDriver(server_dataset_id);
    if (driver == nullptr) {
      LOG(WARNING) << "no dataset access info found for id: "
                    << server_dataset_id;
      return retcode::FAIL;
    }
    auto& access_info = driver->dataSetAccessInfo();
    if (access_info == nullptr) {
      LOG(WARNING) << "no dataset access info found for id: "
                    << server_dataset_id;
      return retcode::FAIL;
    }
    auto& schema = access_info->Schema();
    for (const auto& field : schema) {
      VLOG(5) << "field: " << std::get<0>(field);
      server_dataset_schema_.push_back(std::get<0>(field));
    }
  }
  return retcode::SUCCESS;
}

retcode PirTask::LoadDataset() {
  CHECK_TASK_STOPPED(retcode::FAIL);
  if (RoleValidation::IsClient(this->party_name())) {
    return ClientLoadDataset();
  } else if (RoleValidation::IsServer(this->party_name())) {
    return ServerLoadDataset();
  } else {
    LOG(WARNING) << "party: " << this->party_name()
        << " does not load dataset";
    return retcode::SUCCESS;
  }
}

retcode PirTask::ClientLoadDataset() {
  const auto& param_map = getTaskParam()->params().param_map();
  auto client_data_it = param_map.find("clientData");
  if (client_data_it != param_map.end()) {
    auto& client_data = client_data_it->second;
    if (client_data.is_array()) {
      const auto& items = client_data.value_string_array().value_string_array();
      for (const auto& item : items) {
        elements_[item];
      }
      if (elements_.empty()) {
        LOG(ERROR) << "no query data set by client";
        return retcode::FAIL;
      }
    } else {
      auto item = client_data.value_string();
      elements_[item];
    }
    return retcode::SUCCESS;
  }

  // load data from dataset
  if (this->dataset_id_.empty()) {
    LOG(ERROR) << "no dataset found for client: " << party_name();
    return retcode::FAIL;
  }
  VLOG(7) << "dataset_id: " << this->dataset_id_;
  auto data_ptr = LoadDataSetInternal(this->dataset_id_);
  if (data_ptr == nullptr) {
    LOG(ERROR) << "read data for dataset id: "
               << this->dataset_id_ << " failed";
    return retcode::FAIL;
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(data_ptr->data);
  auto& key_col = client_key_columns_;
  auto key_array = GetSelectedContent(table, key_col);
  for (auto& item : key_array) {
    VLOG(7) << "item: " << item;
    elements_[item];
  }
  return retcode::SUCCESS;
}

retcode PirTask::ServerLoadDataset() {
  if (this->options_.use_cache) {
    VLOG(0) << "using cache data for party: " << party_name();
    return retcode::SUCCESS;
  }
  auto data_ptr = LoadDataSetInternal(this->dataset_id_);
  if (data_ptr == nullptr) {
    std::stringstream ss;
    ss << "read data for dataset id: " << this->dataset_id_ << " failed";
    RaiseException(ss.str());
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(data_ptr->data);
  int col_count = table->num_columns();
  size_t row_count = table->num_rows();
  if (col_count < 2) {
    RaiseException("data for server must have label");
  }
  auto& key_col = this->server_key_columns_;
  if (key_col.empty()) {
    RaiseException("no column selected for keyword");
  }
  auto key_array = GetSelectedContent(table, key_col);
  auto& value_col = this->server_label_columns_;
  if (value_col.empty()) {
    RaiseException("no column selected for label");
  }
  auto value_array = GetSelectedContent(table, value_col);
  elements_.reserve(key_array.size());
  for (size_t i = 0; i < key_array.size(); ++i) {
    auto& key = key_array[i];
    auto& value = value_array[i];
    auto it = elements_.find(key);
    if (it != elements_.end()) {
      it->second.push_back(value);
    } else {
      std::vector<std::string> vec;
      vec.push_back(value);
      elements_.insert({key, std::move(vec)});
    }
  }
  return retcode::SUCCESS;
}

std::shared_ptr<Dataset> PirTask::LoadDataSetInternal(
    const std::string& dataset_id) {
  auto driver =
      this->getDatasetService()->getDriver(dataset_id, is_dataset_detail_);
  if (driver == nullptr) {
    LOG(ERROR) << "get driver for dataset: " << dataset_id << " failed";
    return nullptr;
  }
  auto cursor = driver->GetCursor();
  if (cursor == nullptr) {
    LOG(ERROR) << "init cursor failed for dataset id: " << dataset_id;
    return nullptr;
  }
  // maybe pass schema to get expected data type
  // copy dataset schema, and change all filed to string
  auto schema = driver->dataSetAccessInfo()->Schema();
  for (auto& field : schema) {
    auto& type = std::get<1>(field);
    type = arrow::Type::type::STRING;
  }
  auto data = cursor->read(schema);
  if (data == nullptr) {
    LOG(ERROR) << "read data failed for dataset id: " << dataset_id;
    return nullptr;
  }
  return data;
}

retcode PirTask::SaveResult() {
  if (!NeedSaveResult()) {
    return retcode::SUCCESS;
  }
  VLOG(0) << "save query result to : " << result_file_path_;

  std::vector<std::shared_ptr<arrow::Field>> schema_vector;
  std::vector<std::string> tmp_colums{"value"};
  for (const auto& col_name : tmp_colums) {
    schema_vector.push_back(arrow::field(col_name, arrow::int64()));
  }
  std::vector<std::shared_ptr<arrow::Array>> arrow_array;
  arrow::StringBuilder value_builder;
  for (auto& [key, item_vec] : this->result_) {
    for (auto& item : item_vec) {
      item.erase(std::find(item.begin(), item.end(), '\0'), item.end());
      value_builder.Append(item);
    }
  }
  std::shared_ptr<arrow::Array> value_array;
  value_builder.Finish(&value_array);
  arrow_array.push_back(std::move(value_array));

  auto schema = std::make_shared<arrow::Schema>(schema_vector);
  // std::shared_ptr<arrow::Table>
  auto table = arrow::Table::Make(schema, arrow_array);
  auto driver = DataDirverFactory::getDriver("CSV", "test address");
  auto csv_driver = std::dynamic_pointer_cast<CSVDriver>(driver);
  // get correct seq for result data query from server
  std::vector<std::string> result_schema;
  for (const auto ind : this->server_label_columns_) {
    result_schema.push_back(server_dataset_schema_[ind]);
  }
  auto rtcode = csv_driver->Write(result_schema, table, result_file_path_);
  if (rtcode != retcode::SUCCESS) {
    LOG(ERROR) << "save PIR data to file " << result_file_path_ << " failed.";
    return retcode::FAIL;
  }

  return retcode::SUCCESS;
}

retcode PirTask::InitOperator() {
  auto type = static_cast<primihub::pir::PirType>(pir_type_);
  this->operator_ = primihub::pir::Factory::Create(type, options_);
  if (this->operator_ == nullptr) {
    LOG(ERROR) << "create pir operator failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode PirTask::ExecuteOperator() {
  return operator_->Execute(elements_, &result_);
}

int PirTask::execute() {
  SCopedTimer timer;
  std::string error_msg;
  bool has_error{true};
  do {
    auto ret = LoadParams(task_param_);
    BREAK_LOOP_BY_RETCODE(ret, "Pir load task params failed.")
    auto load_params_ts = timer.timeElapse();
    VLOG(5) << "LoadParams time cost(ms): " << load_params_ts;

    ret = LoadDataset();
    BREAK_LOOP_BY_RETCODE(ret, "Pir load dataset failed.")
    auto load_dataset_ts = timer.timeElapse();
    auto load_dataset_time_cost = load_dataset_ts - load_params_ts;
    VLOG(5) << "LoadDataset time cost(ms): " << load_dataset_time_cost;

    ret = InitOperator();
    BREAK_LOOP_BY_RETCODE(ret, "Pir init operator failed.")
    auto init_op_ts = timer.timeElapse();
    auto init_op_time_cost = init_op_ts - load_dataset_ts;
    VLOG(5) << "InitOperator time cost(ms): " << init_op_time_cost;

    ret = ExecuteOperator();
    BREAK_LOOP_BY_RETCODE(ret, "Pir execute operator failed.")
    auto exec_op_ts = timer.timeElapse();
    auto exec_op_time_cost = exec_op_ts - init_op_ts;
    VLOG(5) << "ExecuteOperator time cost(ms): " << exec_op_time_cost;

    ret = SaveResult();
    BREAK_LOOP_BY_RETCODE(ret, "Pir save result failed.")
    auto save_res_ts = timer.timeElapse();
    auto save_res_time_cost = save_res_ts - exec_op_ts;
    VLOG(5) << "SaveResult time cost(ms): " << save_res_time_cost;
    has_error = false;
  } while (0);
  if (has_error) {
    throw std::runtime_error(error_msg);
  }
  return 0;
}
std::vector<std::string> PirTask::GetSelectedContent(
    std::shared_ptr<arrow::Table>& data_tbl,
    const std::vector<int>& selected_col) {
  // return std::vector<std::string>();
  int col_count = data_tbl->num_columns();
  size_t row_count = data_tbl->num_rows();
  if (selected_col.empty()) {
    LOG(ERROR) << "no col selected for data";
    return std::vector<std::string>();
  }
  int total_columns = data_tbl->num_columns();
  std::vector<std::string> content_array;
  int col_index = selected_col[0];
  if (col_index >= total_columns) {
    std::stringstream ss;
    ss << "index out of range: " << col_index << " "
        << "total columns: " << total_columns;
    RaiseException(ss.str());
  }
  auto label_ptr = data_tbl->column(col_index);
  auto chunk_size = label_ptr->num_chunks();
  size_t total_row_count = col_count * chunk_size;
  content_array.reserve(total_row_count);
  for (int i = 0; i < chunk_size; ++i) {
    auto array =
        std::static_pointer_cast<arrow::StringArray>(label_ptr->chunk(i));
    for (int64_t j = 0; j < array->length(); j++) {
      content_array.push_back(array->GetString(j));
    }
  }
  // process left columns
  for (size_t i = 1; i < selected_col.size(); ++i) {
    size_t index{0};
    int col_index = selected_col[i];
    if (col_index >= total_columns) {
      std::stringstream ss;
      ss << "index out of range: " << col_index << " "
          << "total columns: " << total_columns;
      RaiseException(ss.str());
    }
    auto label_ptr = data_tbl->column(col_index);
    int chunk_size = label_ptr->num_chunks();
    for (int j = 0; j < chunk_size; ++j) {
      auto array =
          std::static_pointer_cast<arrow::StringArray>(label_ptr->chunk(j));
      for (int64_t k = 0; k < array->length(); ++k) {
        content_array[index++].append(",").append(array->GetString(k));
      }
    }
  }
  return content_array;
}

bool PirTask::NeedSaveResult() {
  if (RoleValidation::IsClient(this->party_name())) {
    return true;
  }
  return false;
}
}  // namespace primihub::task
