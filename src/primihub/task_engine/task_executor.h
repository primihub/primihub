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
#ifndef SRC_PRIMIHUB_TASK_ENGINE_TASK_EXECUTOR_H_
#define SRC_PRIMIHUB_TASK_ENGINE_TASK_EXECUTOR_H_
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/task/semantic/task.h"

namespace primihub::task_engine {
using TaskRequest = rpc::PushTaskRequest;
using TaskRequestPtr = std::unique_ptr<TaskRequest>;
using LinkMode = network::LinkMode;
using LinkContext = network::LinkContext;
using LinkContextPtr = std::unique_ptr<LinkContext>;
using TaskPtr = std::shared_ptr<primihub::task::TaskBase>;
using DatasetService = primihub::service::DatasetService;
using DatasetServicePtr = std::shared_ptr<DatasetService>;
class TaskEngine {
 public:
  TaskEngine() = default;
  ~TaskEngine() = default;
  retcode Init(const std::string& server_id,
               const std::string& server_config_file,
               const std::string& request);
  retcode Execute();

  retcode GetScheduleNode();
  retcode UpdateStatus(rpc::TaskStatus::StatusCode code_status,
                       const std::string& msg_info);

 protected:
  retcode ParseTaskRequest(const std::string& request_str);
  retcode InitCommunication();
  retcode InitDatasetSerivce();
  retcode CreateTask();
 private:
  TaskRequestPtr task_request_{nullptr};
  std::string node_id_;
  std::string config_file_;
  Node schedule_node_;
  bool schedule_node_available_{false};
  LinkMode link_mode_{LinkMode::GRPC};
  LinkContextPtr link_ctx_{nullptr};
  TaskPtr task_{nullptr};
  DatasetServicePtr dataset_service_{nullptr};
};
}  // namespace primihub::task_engine
#endif  // SRC_PRIMIHUB_TASK_ENGINE_TASK_EXECUTOR_H_
