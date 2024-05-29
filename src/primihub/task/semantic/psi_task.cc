/*
 Copyright 2022 PrimiHub

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
#include "src/primihub/task/semantic/psi_task.h"
#include <glog/logging.h>
#include <chrono>
#include <numeric>
#include <utility>
#include <map>

#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/endian_util.h"
#include "src/primihub/common/value_check_util.h"
#include "src/primihub/kernel/psi/operator/factory.h"
#include "src/primihub/common/config/server_config.h"

using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
using arrow::Int64Builder;
using primihub::rpc::VarType;


namespace primihub::task {
PsiTask::PsiTask(const TaskParam *task_param) :
                 TaskBase(task_param, nullptr) {
}

PsiTask::PsiTask(const TaskParam* task_param,
                 std::shared_ptr<DatasetService> dataset_service)
                 : TaskBase(task_param, dataset_service) {
}

PsiTask::PsiTask(const TaskParam *task_param,
                 std::shared_ptr<DatasetService> dataset_service,
                 void* ra_server, void* tee_engine)
                 : TaskBase(task_param, dataset_service),
                  ra_server_(ra_server), tee_executor_(tee_engine) {}

retcode PsiTask::BuildOptions(const rpc::Task& task, psi::Options* options) {
  // build Options for operator
  options->self_party = this->party_name();
  if (!options->link_ctx_ref) {
    options->link_ctx_ref = getTaskContext().getLinkContext().get();
  }
  options->psi_result_type = psi::PsiResultType::INTERSECTION;
  options->code = task.code();
  auto& party_info = options->party_info;
  const auto& pb_party_info = task.party_access_info();
  for (const auto& [_party_name, pb_node] : pb_party_info) {
    Node node_info;
    pbNode2Node(pb_node, &node_info);
    party_info[_party_name] = std::move(node_info);
  }
  auto ret = this->ExtractProxyNode(task, &options->proxy_node);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "get proxy node failed";
    return retcode::FAIL;
  }

  const auto& param_map = task.params().param_map();
  // TODO(XXX) rename to PsiResultType {intersection or DIFFERENCE}
  auto it = param_map.find("psiType");   // psi result
  if (it != param_map.end()) {
    options->psi_result_type =
        static_cast<psi::PsiResultType>(it->second.value_int32());
  }
  // end of build Options
  return retcode::SUCCESS;
}

retcode PsiTask::LoadParams(const rpc::Task& task) {
  std::string party_name = task.party_name();
  auto ret = BuildOptions(task, &this->options_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "build operator options for party: "
               << party_name << " failed";
    return retcode::FAIL;
  }
  // psi type TODO rename to psiType
  const auto& param_map = task.params().param_map();
  auto tag_it = param_map.find("psiTag");
  if (tag_it != param_map.end()) {
    psi_type_ = tag_it->second.value_int32();
  }
  // check validataion of psi type value
  if (!rpc::PsiTag_IsValid(psi_type_)) {
    std::stringstream ss;
    ss << "PsiTag is unknown, value: " << psi_type_;
    RaiseException(ss.str());
  }
  // get flag for remove duplicate data from origin dataset
  {
    auto it = param_map.find("UniqueValues");
    if (it != param_map.end()) {
      unique_values_ = it->second.value_int32() > 0;
      VLOG(0) << "unique_values_: " << unique_values_;
    }
  }

  // broadcast result flag
  auto iter = param_map.find("sync_result_to_server");
  if (iter != param_map.end()) {
    broadcast_result_ = iter->second.value_int32() > 0;
    VLOG(5) << "broadcast result flag: " << broadcast_result_;
  }

  // Parse dataset
  const auto& party_datasets = task.party_datasets();
  auto it = party_datasets.find(party_name);
  if (it == party_datasets.end()) {
    LOG(WARNING) << "no dataset is found for party_name: " << party_name;
    return retcode::SUCCESS;
  }
  auto& dataset_map = it->second.data();
  do {
    auto it = dataset_map.find(party_name);
    if (it == dataset_map.end()) {
      LOG(WARNING) << "no dataset is found for party_name: " << party_name;
      break;
    }
    dataset_id_ = it->second;

  } while (0);
  if (it->second.dataset_detail()) {
    is_dataset_detail_ = true;
  }

  // parse selected index
  std::string index_keyword;
  if (IsClient()) {
    VLOG(5) << "parse index for client";
    index_keyword = "clientIndex";
  } else if (IsServer()) {
    VLOG(5) << "parse index for server";
    index_keyword = "serverIndex";
  }
  auto index_it = param_map.find(index_keyword);
  if (index_it != param_map.end()) {
    const auto& client_index = index_it->second;
    if (client_index.is_array()) {
      const auto& array_values =
          client_index.value_int32_array().value_int32_array();
      for (const auto value : array_values) {
        data_index_.push_back(value);
      }
    } else {
      data_index_.push_back(client_index.value_int32());
    }
  }
  if (IsTeeCompute()) {
    return retcode::SUCCESS;
  }
  // parse result path
  std::string result_path_keyword;
  if (IsClient()) {
    VLOG(5) << "parse result path for client";
    result_path_keyword = "outputFullFilename";
  } else if (IsServer()) {
    VLOG(5) << "parse result path for server";
    result_path_keyword = "server_outputFullFilname";
  }
  auto path_it = param_map.find(result_path_keyword);
  if (path_it != param_map.end()) {
    result_file_path_ = CompletePath(path_it->second.value_string());
    LOG(INFO) << "result_file_path: " << result_file_path_;
  } else {
    if (IsClient() || (IsServer() && broadcast_result_ )) {
      LOG(WARNING) << "no path found for " << party_name << " to save result";
    }
  }
  return retcode::SUCCESS;
}

retcode PsiTask::LoadDataset(void) {
  if (dataset_id_.empty() || data_index_.empty()) {
    return retcode::SUCCESS;
  }
  auto driver = this->getDatasetService()->getDriver(this->dataset_id_,
                                                     is_dataset_detail_);
  if (driver == nullptr) {
    LOG(ERROR) << "get driver for data set: " << this->dataset_id_ << " failed";
    return retcode::FAIL;
  }
  auto ret = LoadDatasetInternal(driver, data_index_,
                                 &elements_, &data_colums_name_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "Load dataset for psi server failed.";
    return retcode::FAIL;
  }
  // filter duplicated data
  if (unique_values_) {
    SCopedTimer timer;
    std::vector<std::string> filtered_data;
    filtered_data.reserve(elements_.size());
    std::unordered_set<std::string> dup(elements_.size());
    int64_t duplicate_num = 0;
    for (auto& item : elements_) {
      if (dup.find(item) != dup.end()) {
        duplicate_num++;
        continue;
      }
      dup.insert(item);
      filtered_data.push_back(item);
    }
    if (duplicate_num != 0) {
      LOG(WARNING) << "item has duplicated time, count: " << duplicate_num;
    }
    elements_ = std::move(filtered_data);
    auto time_cost = timer.timeElapse();
    VLOG(3) << "filter data time cost: " << time_cost;
  }
  return retcode::SUCCESS;
}

retcode PsiTask::InitOperator() {
  auto type = static_cast<primihub::psi::PsiType>(psi_type_);
  this->psi_operator_ =
      primihub::psi::Factory::Create(type, options_, tee_executor_);
  if (this->psi_operator_ == nullptr) {
    LOG(ERROR) << "create psi operator failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode PsiTask::ExecuteOperator() {
  return psi_operator_->Execute(elements_, broadcast_result_, &result_);
}

retcode PsiTask::SaveResult() {
  if (!NeedSaveResult()) {
    return retcode::SUCCESS;
  }
  auto ret = SaveDataToCSVFile(result_, result_file_path_, data_colums_name_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "save result to " << result_file_path_ << " failed";
    return retcode::FAIL;
  }
  return ret;
}

int PsiTask::execute() {
  SCopedTimer timer;
  std::string error_msg;
  bool has_error{true};
  do {
    auto ret = LoadParams(task_param_);
    BREAK_LOOP_BY_RETCODE(ret, "Psi load task params failed.")
    auto load_params_ts = timer.timeElapse();
    VLOG(5) << "LoadParams time cost(ms): " << load_params_ts;

    ret = LoadDataset();
    BREAK_LOOP_BY_RETCODE(ret, "Psi load dataset failed.")
    auto load_dataset_ts = timer.timeElapse();
    auto load_dataset_time_cost = load_dataset_ts - load_params_ts;
    VLOG(5) << "LoadDataset time cost(ms): " << load_dataset_time_cost;

    ret = InitOperator();
    BREAK_LOOP_BY_RETCODE(ret, "Psi init operator failed.")
    auto init_op_ts = timer.timeElapse();
    auto init_op_time_cost = init_op_ts - load_dataset_ts;
    VLOG(5) << "InitOperator time cost(ms): " << init_op_time_cost;

    ret = ExecuteOperator();
    BREAK_LOOP_BY_RETCODE(ret, "Psi execute operator failed.")
    auto exec_op_ts = timer.timeElapse();
    auto exec_op_time_cost = exec_op_ts - init_op_ts;
    VLOG(5) << "ExecuteOperator time cost(ms): " << exec_op_time_cost;

    ret = SaveResult();
    BREAK_LOOP_BY_RETCODE(ret, "Psi save result failed.")
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

retcode PsiTask::ExecuteTask(const std::vector<std::string>& input,
                             std::vector<std::string>* result) {
  do {
    std::string error_msg;
    auto ret = LoadParams(task_param_);
    BREAK_LOOP_BY_RETCODE(ret, "Psi load task params failed.")
    ret = InitOperator();
    BREAK_LOOP_BY_RETCODE(ret, "Psi init operator failed.")
    psi_operator_->Execute(input, broadcast_result_, result);
    BREAK_LOOP_BY_RETCODE(ret, "Psi execute operator failed.")
    LOG(INFO) << "ExecuteTask result size: " << result->size();
  } while (0);
  return retcode::SUCCESS;
}

bool PsiTask::NeedSaveResult() {
  if (IsTeeCompute()) {
    return false;
  }
  if (IsServer() && !broadcast_result_) {
    return false;
  }
  return true;
}

bool PsiTask::IsClient() {
  if (party_name() == PARTY_CLIENT) {
    return true;
  } else {
    return false;
  }
}

bool PsiTask::IsServer() {
  if (party_name() == PARTY_SERVER) {
    return true;
  } else {
    return false;
  }
}
bool PsiTask::IsTeeCompute() {
  if (party_name() == PARTY_TEE_COMPUTE) {
    return true;
  } else {
    return false;
  }
}
}  // namespace primihub::task
