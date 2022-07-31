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

namespace primihub {

void Worker::execute(const PushTaskRequest *pushTaskRequest) {
    primihub::rpc::TaskType type = pushTaskRequest->task().type();
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
        if (ret != 0)
            LOG(ERROR) << "Error occurs during execute task.";

        return;
    } else if (type == rpc::TaskType::NODE_PSI_TASK || 
               type == rpc::TaskType::NODE_PIR_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task now.";
            return;
        }

        auto param_map_it = pushTaskRequest->task().params().param_map().find("serverAddress");
        if (param_map_it == pushTaskRequest->task().params().param_map().end()) {
            return;
        }

        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create psi/pir task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0)
            LOG(ERROR) << "Error occurs during execute psi/pir task.";
    }  else {
        LOG(WARNING) << "Requested task type is not supported.";
    }
}


// PIR /PSI Server worker execution
void Worker::execute(const ExecuteTaskRequest *taskRequest,
                     ExecuteTaskResponse *taskResponse) {
    //primihub::rpc::TaskType type;

    if (taskRequest->algorithm_request_case() ==
        ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
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
        if (ret != 0)
            LOG(ERROR) << "Error occurs during server node execute task.";
    } else if (taskRequest->algorithm_request_case() ==
	           ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id,
			                             rpc::TaskType::NODE_PIR_TASK,
			                             *taskRequest,
                                         taskResponse,
					                     dataset_service);
        if (pTask == nullptr) {
            LOG(ERROR) << "Woker create server node task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0)
            LOG(ERROR) << "Error occurs during server node execute task.";
    } else {
        LOG(WARNING) << "Requested task type is not supported.";
    }
}

} // namespace primihub
