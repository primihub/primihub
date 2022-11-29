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


#include "src/primihub/node/worker/worker.h"
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/service/error.hpp"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/task/semantic/factory.h"
#include "src/primihub/task/semantic/task.h"


using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::task::TaskFactory;
using primihub::rpc::PsiTag;

namespace primihub {
int Worker::execute(const PushTaskRequest *pushTaskRequest) {
    auto type = pushTaskRequest->task().type();
    VLOG(2) << "Worker::execute task type: " << type;
    if (type == rpc::TaskType::NODE_TASK ||
        type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
        auto dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create task failed.";
            return -1;
        }
        LOG(INFO) << " 🚀 Worker start execute task ";
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute task.";
            return -1;
        }
    } else if (type == rpc::TaskType::NODE_PSI_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task now.";
            return -1;
        }
        const auto& param_map = pushTaskRequest->task().params().param_map();
        auto dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create psi task failed.";
            return -1;
        }
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute psi task.";
            return -1;
        }
    } else if (type == rpc::TaskType::NODE_PIR_TASK) {
        size_t party_node_count = pushTaskRequest->task().node_map().size();
        if (party_node_count < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task. "
                       << "current_node_size: " << party_node_count;
            return -1;
        }
        const auto& param_map = pushTaskRequest->task().params().param_map();
        int pirType = PirType::ID_PIR;
        auto param_it = param_map.find("pirType");
        if (param_it != param_map.end()) {
            pirType = param_it->second.value_int32();
            VLOG(5) << "get pirType: " << pirType;
        }

        if (pirType == PirType::ID_PIR) {
            auto param_map_it = param_map.find("serverAddress");
            if (param_map_it == param_map.end()) {
                return -1;
            }
        }

        auto& dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create pir task failed.";
            return -1;
        }
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute pir task.";
            return -1;
        }
    } else {
        LOG(WARNING) << "unsupported Requested task type: " << type;
    }
    task_ptr.reset();
}


// PIR /PSI Server worker execution
int Worker::execute(const ExecuteTaskRequest *taskRequest,
                     ExecuteTaskResponse *taskResponse) {
    auto request_type = taskRequest->algorithm_request_case();
    if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
        auto dataset_service = nodelet->getDataService();
        task_server_ptr = TaskFactory::Create(this->node_id,
			                             rpc::TaskType::NODE_PSI_TASK,
			                             *taskRequest,
                                         taskResponse,
					                     dataset_service);
    } else if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        VLOG(0) << "algorithm_request_case kPirRequest Worker::execute";
        auto dataset_service = nodelet->getDataService();
        task_server_ptr = TaskFactory::Create(this->node_id,
                                         rpc::TaskType::NODE_PIR_TASK,   // convert into internal task type
                                         *taskRequest,
                                         taskResponse,
                                         dataset_service);
    } else {
        LOG(WARNING) << "Requested task type is not supported.";
        return -1;
    }
    if (task_server_ptr == nullptr) {
        LOG(ERROR) << "Woker create server node task failed.";
        return -1;
    }
    int ret = task_server_ptr->execute();
    if (ret != 0) {
        LOG(ERROR) << "Error occurs during server node execute task.";
        return -1;
    }
    task_server_ptr.reset();
    return 0;
}

// kill task which is running in the worker
void Worker::kill_task() {
    if (task_ptr) {
        task_ptr->kill_task();
    }
    if (task_server_ptr) {
        task_server_ptr->kill_task();
    }
}

} // namespace primihub
