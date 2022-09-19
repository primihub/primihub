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
using primihub::rpc::Node;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::task::TaskFactory;
using primihub::rpc::PsiTag;

namespace primihub {

void Worker::execute(const PushTaskRequest *pushTaskRequest) {
    auto type = pushTaskRequest->task().type();
    VLOG(2) << "Worker::execute task type: " << type;
    if (type == rpc::TaskType::NODE_TASK ||
        type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create task failed.";
            return;
        }
        LOG(INFO) << " 🚀 Worker start execute task ";
        int ret = pTask->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute task.";
        }
    } else if (type == rpc::TaskType::NODE_PSI_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task now.";
            return;
        }

        const auto& param_map = pushTaskRequest->task().params().param_map();
        int psiTag = PsiTag::ECDH;
        auto param_it = param_map.find("psiTag");
        if (param_it != param_map.end()) {
            psiTag = param_it->second.value_int32();
        }

        if (psiTag == PsiTag::ECDH) {
            auto param_map_it = param_map.find("serverAddress");
            if (param_map_it == param_map.end()) {
                return;
            }
        }

        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create psi task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0)
            LOG(ERROR) << "Error occurs during execute psi task.";
    } else if (type == rpc::TaskType::NODE_PIR_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task now.";
            return;
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
                return ;
            }
        }

        auto& dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create pir task failed.";
            return ;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute pir task.";
        }
    } else {
        LOG(WARNING) << "unsupported Requested task type: " << type;
    }
}


// PIR /PSI Server worker execution
void Worker::execute(const ExecuteTaskRequest *taskRequest,
                     ExecuteTaskResponse *taskResponse) {
    auto request_type = taskRequest->algorithm_request_case();
    if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id,
			                             rpc::TaskType::NODE_PSI_TASK,
			                             *taskRequest,
                                         taskResponse,
					                     dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create server node task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during server node execute task.";
        }
    } else if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        VLOG(0) << "algorithm_request_case kPirRequest Worker::execute";
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id,
                                         rpc::TaskType::NODE_PIR_TASK,   // convert into internal task type
                                         *taskRequest,
                                         taskResponse,
                                         dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create server node task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during server node execute task.";
        }
    } else {
        LOG(WARNING) << "Requested task type is not supported.";
    }
}

} // namespace primihub
