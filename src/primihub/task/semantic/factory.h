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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_

#include <glog/logging.h>
#include <memory>

#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/semantic/mpc_task.h"
#ifdef PY_TASK_ENABLED
#include "src/primihub/task/semantic/fl_task.h"
#endif  // PY_TASK_ENABLED
#include "src/primihub/task/semantic/psi_task.h"
#include "src/primihub/task/semantic/pir_task.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"

using primihub::rpc::PushTaskRequest;
using primihub::rpc::Language;
using primihub::rpc::TaskType;
using primihub::service::DatasetService;
namespace pb_util = primihub::proto::util;
namespace primihub::task {

class TaskFactory {
 public:
  static std::shared_ptr<TaskBase> Create(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service,
      void* ra_service = nullptr,
      void* executor = nullptr) {
    auto task_language = request.task().language();
    const auto& task_info = request.task().task_info();
    std::string task_inof_str = pb_util::TaskInfoToString(task_info);
    auto task_type = request.task().type();
    std::shared_ptr<TaskBase> task_ptr{nullptr};
    switch (task_language) {
#ifdef PY_TASK_ENABLED
    case Language::PYTHON:
      task_ptr = TaskFactory::CreateFLTask(node_id, request, dataset_service);
      break;
#endif
    case Language::PROTO: {
      switch (task_type) {
      case rpc::TaskType::ACTOR_TASK:
        task_ptr = TaskFactory::CreateMPCTask(node_id, request, dataset_service);
        break;
      case rpc::TaskType::PSI_TASK:
        task_ptr =
            TaskFactory::CreatePSITask(node_id, request, dataset_service,
                                       ra_service, executor);
        break;
      case rpc::TaskType::PIR_TASK:
        task_ptr = TaskFactory::CreatePIRTask(node_id, request, dataset_service);
        break;
      default:
        LOG(ERROR) << task_inof_str << "unsupported task type: " << task_type;
        break;
      }
      break;
    }
    default:
      LOG(ERROR) << task_inof_str << "unsupported language: " << task_language;
      break;
    }
    return task_ptr;
  }
#ifdef PY_TASK_ENABLED
  static std::shared_ptr<TaskBase> CreateFLTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& task_param = request.task();
    return std::make_shared<FLTask>(node_id, &task_param,
                                    request, dataset_service);
  }
#endif  // PY_TASK_ENABLED
  static std::shared_ptr<TaskBase> CreateMPCTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& _function_name = request.task().code();
    const auto& task_param = request.task();
    return std::make_shared<MPCTask>(node_id, _function_name,
                                    &task_param, dataset_service);
  }

  static std::shared_ptr<TaskBase> CreatePSITask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service,
      void* ra_server,
      void* tee_engine) {
    const auto& task_config = request.task();
    std::shared_ptr<TaskBase> task_ptr{nullptr};
    task_ptr = std::make_shared<PsiTask>(&task_config, dataset_service,
                                         ra_server, tee_engine);
    return task_ptr;
  }

  static std::shared_ptr<TaskBase> CreatePIRTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& task_config = request.task();
    return std::make_shared<PirTask>(&task_config, dataset_service);
  }

};
}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_
