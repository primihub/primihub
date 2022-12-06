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
#include "src/primihub/task/semantic/private_server_base.h"
#include "src/primihub/task/semantic/psi_client_task.h"
#include "src/primihub/task/semantic/psi_server_task.h"
#include "src/primihub/task/semantic/psi_kkrt_task.h"

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
        std::string job_id = request.task().job_id();
        std::string task_id = request.task().task_id();
        std::string submit_client_id = request.submit_client_id();
        if (task_language == Language::PYTHON && task_type == rpc::TaskType::NODE_TASK) {
            auto task_param = request.task();
            auto fl_task = std::make_shared<FLTask>(node_id, &task_param, request, dataset_service);
            return std::dynamic_pointer_cast<TaskBase>(fl_task);

        } else if (task_language == Language::PROTO && task_type == rpc::TaskType::NODE_TASK) {
            auto _function_name = request.task().code();
            auto task_param = request.task();
            auto mpc_task = std::make_shared<MPCTask>(node_id,
                                                    _function_name, &task_param,
                                                    dataset_service);
            return std::dynamic_pointer_cast<TaskBase>(mpc_task);
        } else if (task_language == Language::PROTO && task_type == rpc::TaskType::NODE_PSI_TASK) {
            auto task_param = request.task();
            const auto& param_map = task_param.params().param_map();
            int psi_tag = PsiTag::ECDH;
            auto param_it = param_map.find("psiTag");
            if (param_it != param_map.end()) {
                psi_tag = param_it->second.value_int32();
            }

            if (psi_tag == PsiTag::ECDH) {
                auto psi_task =
                    std::make_shared<PSIClientTask>(&task_param, dataset_service);
                psi_task->setTaskInfo(node_id, job_id, task_id, submit_client_id);
                return std::dynamic_pointer_cast<TaskBase>(psi_task);
            } else if (psi_tag == PsiTag::KKRT) {
                auto psi_task = std::make_shared<PSIKkrtTask>(node_id,
                                                              request.task().job_id(),
                                                              request.task().task_id(),
                                                              &task_param,
                                                              dataset_service);
                psi_task->setTaskInfo(node_id, job_id, task_id, submit_client_id);
                return std::dynamic_pointer_cast<TaskBase>(psi_task);
            } else {
                LOG(ERROR) << "Unsupported psi tag: " << psi_tag;
                return nullptr;
            }
        } else if (task_language == Language::PROTO && task_type == rpc::TaskType::NODE_PIR_TASK) {
            const auto& task_param = request.task();
            const auto& param_map = task_param.params().param_map();
            int pir_type = PirType::ID_PIR;
            auto param_it = param_map.find("pirType");
            if (param_it != param_map.end()) {
                pir_type = param_it->second.value_int32();
            }
            const auto& job_id = request.task().job_id();
            const auto& task_id = request.task().task_id();
#ifndef USE_MICROSOFT_APSI
            if (pir_type == PirType::ID_PIR) {

                return std::make_shared<PIRClientTask>(
                    node_id, job_id, task_id, &task_param, dataset_service);
            } else {
                // TODO, using condition compile, fix in future
                LOG(WARNING) << "ID_PIR is not supported when using MICROSOFT_APSI";
                return nullptr;
            }
#else
            if (pir_type == PirType::KEY_PIR) {
                auto param_map_it = param_map.find("serverAddress");
                if (param_map_it == param_map.end()) {
                    return std::make_shared<KeywordPIRServerTask>(
                        node_id, job_id, task_id, &task_param, dataset_service);
                } else {
                   return std::make_shared<KeywordPIRClientTask>(
                        node_id, job_id, task_id, &task_param, dataset_service);
                }
            } else {
                LOG(ERROR) << "Unsupported pir type: " << pir_type;
                return nullptr;
            }
#endif
        } else if (task_language == Language::PROTO &&
                task_type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
            auto task_param = request.task();
            auto tee_task = std::make_shared<TEEDataProviderTask>(node_id,
                                                    &task_param,
                                                    dataset_service);
            return std::dynamic_pointer_cast<TaskBase>(tee_task);
        } else {
            LOG(ERROR) << "Unsupported task type: "<< task_type <<","
                        << "language: "<< task_language;
            return nullptr;
        }
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
            return std::make_shared<PIRServerTask>(node_id, request, response, dataset_service);
#endif
        } else if (task_type == rpc::TaskType::NODE_PSI_TASK) {
            auto psi_task = std::make_shared<PSIServerTask>(node_id, request,
                                                            response, dataset_service);
            return std::dynamic_pointer_cast<ServerTaskBase>(psi_task);
        } else {
            LOG(ERROR) << "Unsupported task type at server node: "<< task_type <<".";
            return nullptr;
        }
    }
};


} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_FACTORY_H_
