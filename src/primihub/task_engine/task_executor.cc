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
#include "src/primihub/task_engine/task_executor.h"
#include "base64.h"                         // NOLINT
#include "src/primihub/util/util.h"
#include "src/primihub/common/config/server_config.h"
#include "src/primihub/service/dataset/meta_service/factory.h"
#include "src/primihub/task/semantic/factory.h"

namespace primihub::task_engine {
retcode TaskEngine::Init(const std::string& server_id,
                         const std::string& config_file,
                         const std::string& request) {
  this->node_id_ = server_id;
  this->config_file_ = config_file;
  VLOG(5) << "ParseTaskRequest";
  auto ret = ParseTaskRequest(request);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ParseTaskRequest failed";
    return retcode::FAIL;
  }
  VLOG(5) << "GetScheduleNode";
  ret = GetScheduleNode();
  VLOG(5) << "InitCommunication";
  ret = InitCommunication();
  VLOG(5) << "InitDatasetSerivce";
  ret = InitDatasetSerivce();
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "InitDatasetSerivce failed";
    return retcode::FAIL;
  }
  VLOG(5) << "CreateTask";
  ret = CreateTask();
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "CreateTask failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}
retcode TaskEngine::ParseTaskRequest(const std::string& request_str) {
  std::string pb_task_request_ = base64_decode(request_str);
  task_request_ = std::make_unique<TaskRequest>();
  bool status = task_request_->ParseFromString(pb_task_request_);
  if (!status) {
    LOG(ERROR) << "parse task request error";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode TaskEngine::InitCommunication() {
  using LinkFactory = primihub::network::LinkFactory;
  link_mode_ = LinkMode::GRPC;
  link_ctx_ = LinkFactory::createLinkContext(link_mode_);
  return retcode::SUCCESS;
}

retcode TaskEngine::InitDatasetSerivce() {
  auto& server_config = primihub::ServerConfig::getInstance();
  auto ret = server_config.initServerConfig(this->config_file_);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "init server config failed";
    return retcode::FAIL;
  }
  // service for dataset meta control
  auto& node_cfg = server_config.getNodeConfig();
  auto& meta_service_cfg = node_cfg.meta_service_config;
  using MetaServiceFactory = primihub::service::MetaServiceFactory;
  auto meta_service = MetaServiceFactory::Create(meta_service_cfg.mode,
                                                 meta_service_cfg.host_info);
  dataset_service_ = std::make_shared<DatasetService>(std::move(meta_service));
  return retcode::SUCCESS;
}

retcode TaskEngine::GetScheduleNode() {
  const auto& task_config = task_request_->task();
  const auto& party_access_info = task_config.auxiliary_server();
  auto it = party_access_info.find(SCHEDULER_NODE);
  if (it != party_access_info.end()) {
    const auto& pb_node = it->second;
    pbNode2Node(pb_node, &schedule_node_);
    schedule_node_available_ = true;
  }
  return retcode::SUCCESS;
}

retcode TaskEngine::UpdateStatus(rpc::TaskStatus::StatusCode code_status,
                                 const std::string& msg_info) {
  const auto& task_config = task_request_->task();
  primihub::rpc::TaskStatus task_status;
  primihub::rpc::Empty reply;
  auto task_info_ptr = task_status.mutable_task_info();
  task_info_ptr->CopyFrom(task_config.task_info());
  task_status.set_party(task_config.party_name());
  task_status.set_message(msg_info);
  task_status.set_status(code_status);
  if (!schedule_node_available_) {
    LOG(WARNING) << "schedule node is not available";
    return retcode::FAIL;
  }
  auto channel = link_ctx_->getChannel(schedule_node_);
  channel->updateTaskStatus(task_status, &reply);
  return retcode::SUCCESS;
}

retcode TaskEngine::CreateTask() {
  try {
    using TaskFactory = primihub::task::TaskFactory;
    task_ = TaskFactory::Create(this->node_id_, *task_request_,
                                this->dataset_service_);
    if (task_ == nullptr) {
      LOG(ERROR) << "create Task Failed";
      return retcode::FAIL;
    }
  } catch (std::exception& e) {
    int exception_len = strlen(e.what());
    size_t err_len = exception_len > 1024 ? 1024 : exception_len;
    std::string err_msg(e.what(), exception_len);
    LOG(ERROR) << "Failed to CreateTask: " << err_msg;
    UpdateStatus(rpc::TaskStatus::FAIL, err_msg);
    return retcode::FAIL;
  }

  return retcode::SUCCESS;
}

retcode TaskEngine::Execute() {
  if (task_ == nullptr) {
    LOG(ERROR) << "task is not available";
    return retcode::FAIL;
  }
  try {
    auto ret = task_->execute();
    if (ret != 0) {
      LOG(ERROR) << "run task failed";
      return retcode::FAIL;
    }
  } catch (std::exception& e) {
    int exception_len = strlen(e.what());
    size_t err_len = exception_len > 1024 ? 1024 : exception_len;
    std::string err_msg(e.what(), exception_len);
    LOG(ERROR) << "Failed to execute task: " << err_msg;
    UpdateStatus(rpc::TaskStatus::FAIL, err_msg);
    return retcode::FAIL;
  }
  std::string msg = "SUCCESS";
  LOG(INFO) << "run task success";
  UpdateStatus(rpc::TaskStatus::SUCCESS, msg);
  return retcode::SUCCESS;
}

} // namespace primihub::task_engine
