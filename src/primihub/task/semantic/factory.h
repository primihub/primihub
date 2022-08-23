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
#include "src/primihub/task/semantic/pir_client_task.h"
#include "src/primihub/task/semantic/pir_server_task.h"
#include "src/primihub/task/semantic/tee_task.h"
#include "src/primihub/service/dataset/service.h"


using primihub::rpc::PushTaskRequest;
using primihub::rpc::Language;
using primihub::rpc::TaskType;
using primihub::service::DatasetService;

namespace primihub::task {

class TaskFactory {
  public:
    static std::shared_ptr<TaskBase> Create(const std::string& node_id,
                                        const PushTaskRequest& request,
                                        std::shared_ptr<DatasetService> dataset_service) {
        auto task_language = request.task().language();
        auto task_type = request.task().type();
       

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
            auto psi_task = std::make_shared<PSIClientTask>(node_id,
                                                            request.task().job_id(),
                                                            request.task().task_id(),
                                                            &task_param, 
                                                            dataset_service);
            return std::dynamic_pointer_cast<TaskBase>(psi_task);
        } else if (task_language == Language::PROTO && task_type == rpc::TaskType::NODE_PIR_TASK) {
            auto task_param = request.task();
            auto pir_task = std::make_shared<PIRClientTask>(node_id,
                                                            request.task().job_id(),
                                                            request.task().task_id(),
                                                            &task_param, 
                                                            dataset_service);
            return std::dynamic_pointer_cast<TaskBase>(pir_task);
        }  else if (task_language == Language::PROTO && task_type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
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
            auto pir_task = std::make_shared<PIRServerTask>(node_id, request,
                                                            response, dataset_service);
            return std::dynamic_pointer_cast<ServerTaskBase>(pir_task);
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
