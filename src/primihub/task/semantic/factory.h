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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_

#include <glog/logging.h>
#include <memory>

#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/semantic/mpc_task.h"
#include "src/primihub/task/semantic/fl_task.h"
#include "src/primihub/task/semantic/psi_kkrt_task.h"
#include "src/primihub/task/semantic/psi_ecdh_task.h"
#include "src/primihub/task/semantic/private_server_base.h"

#ifndef USE_MICROSOFT_APSI
#include "src/primihub/task/semantic/pir_client_task.h"
#include "src/primihub/task/semantic/pir_server_task.h"
#else
#include "src/primihub/task/semantic/keyword_pir_client_task.h"
#include "src/primihub/task/semantic/keyword_pir_server_task.h"
#endif

#include "src/primihub/task/semantic/tee_task.h"
#include "src/primihub/service/dataset/service.h"

using primihub::rpc::PushTaskRequest;
using primihub::rpc::Language;
using primihub::rpc::TaskType;
using primihub::service::DatasetService;
using primihub::rpc::PsiTag;
using primihub::rpc::PirType;

namespace primihub::task {

class TaskFactory {
 public:
  static std::shared_ptr<TaskBase> Create(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    auto task_language = request.task().language();
    auto task_type = request.task().type();
    std::shared_ptr<TaskBase> task_ptr{nullptr};
    switch (task_language) {
    case Language::PYTHON:
      task_ptr = TaskFactory::CreateFLTask(node_id, request, dataset_service);
      break;
    case Language::PROTO: {
      switch (task_type) {
      case rpc::TaskType::ACTOR_TASK:
        task_ptr = TaskFactory::CreateMPCTask(node_id, request, dataset_service);
        break;
      case rpc::TaskType::PSI_TASK:
        task_ptr = TaskFactory::CreatePSITask(node_id, request, dataset_service);
        break;
      case rpc::TaskType::PIR_TASK:
        task_ptr = TaskFactory::CreatePIRTask(node_id, request, dataset_service);
        break;
      case rpc::TaskType::TEE_DATAPROVIDER_TASK:
        task_ptr = TaskFactory::CreateTEETask(node_id, request, dataset_service);
        break;
      default:
        LOG(ERROR) << "unsupported task type: " << task_type;
        break;
      }
    }
    default:
      LOG(ERROR) << "unsupported language: " << task_language;
      break;
    }
    return task_ptr;
  }

  static std::shared_ptr<TaskBase> CreateFLTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& task_param = request.task();
    return std::make_shared<FLTask>(node_id, &task_param,
                                    request, dataset_service);
  }

  static std::shared_ptr<TaskBase> CreateMPCTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& code_map = request.task().code();
    std::string _function_name;
    auto it = code_map.find(DEFAULT);
    if (it != code_map.end()) {
      _function_name = it->second;
    }
    const auto& task_param = request.task();
    return std::make_shared<MPCTask>(node_id, _function_name,
                                    &task_param, dataset_service);
  }

  static std::shared_ptr<TaskBase> CreatePSITask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    std::shared_ptr<TaskBase> task_ptr{nullptr};
    const auto& task_config = request.task();
    const auto& param_map = task_config.params().param_map();
    int psi_tag{PsiTag::ECDH};
    auto param_it = param_map.find("psiTag");
    if (param_it != param_map.end()) {
      psi_tag = param_it->second.value_int32();
    }
    switch (psi_tag) {
    case PsiTag::ECDH:
      task_ptr = std::make_shared<PSIEcdhTask>(&task_config, dataset_service);
    case PsiTag::KKRT:
      task_ptr = std::make_shared<PSIKkrtTask>(&task_config, dataset_service);
    default:
      LOG(ERROR) << "Unsupported psi tag: " << psi_tag;
      break;
    }
    return task_ptr;
  }

  static std::shared_ptr<TaskBase> CreatePIRTask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& task_config = request.task();
    const auto& param_map = task_config.params().param_map();
    int pir_type = PirType::ID_PIR;
    auto param_it = param_map.find("pirType");
    if (param_it != param_map.end()) {
      pir_type = param_it->second.value_int32();
    }
#ifndef USE_MICROSOFT_APSI
    const auto& job_id = request.task().task_info().job_id();
    const auto& task_id = request.task().task_info().task_id();
    if (pir_type == PirType::ID_PIR) {
      return std::make_shared<PIRClientTask>(
          node_id, job_id, task_id, &task_config, dataset_service);
    } else {
      // TODO, using condition compile, fix in future
      LOG(WARNING) << "ID_PIR is not supported when MICROSOFT_APSI enabled";
      return nullptr;
    }
#else   // KEYWORD PIR
    if (pir_type == PirType::KEY_PIR) {
      std::string party_name = task_config.party_name();
      if (party_name == PARTY_SERVER) {
        return std::make_shared<KeywordPIRServerTask>(&task_config,
                                                      dataset_service);
      } else {
        return std::make_shared<KeywordPIRClientTask>(&task_config,
                                                      dataset_service);
      }
    } else {
      LOG(ERROR) << "Unsupported pir type: " << pir_type;
      return nullptr;
    }
#endif
  }

  static std::shared_ptr<TaskBase> CreateTEETask(const std::string& node_id,
      const PushTaskRequest& request,
      std::shared_ptr<DatasetService> dataset_service) {
    const auto& task_config = request.task();
    return std::make_shared<TEEDataProviderTask>(node_id,
                                                &task_config,
                                                dataset_service);
  }

  static std::shared_ptr<ServerTaskBase> Create(const std::string& node_id,
        rpc::TaskType task_type,
        const ExecuteTaskRequest& request,
        ExecuteTaskResponse *response,
        std::shared_ptr<DatasetService> dataset_service) {
    if (task_type == rpc::TaskType::NODE_PIR_TASK) {
#ifdef USE_MICROSOFT_APSI
      // TODO, using condition compile, fix in future
      LOG(WARNING) << "ID_PIR is not supported when using MICROSOFT_APSI";
      return nullptr;
#else
      return std::make_shared<PIRServerTask>(node_id, request,
                                            response, dataset_service);
#endif
    } else {
      LOG(ERROR) << "Unsupported task type at server node: "<< task_type <<".";
      return nullptr;
    }
  }
};


}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_
