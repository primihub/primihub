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
#include "src/primihub/util/log_wrapper.h"

using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::Node;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::task::TaskFactory;
using primihub::rpc::PsiTag;

namespace primihub {
#define PLATFORM typeToName(PlatFormType::WORKER_NODE)
#undef V_VLOG
#define V_VLOG(level, PLATFORM, JOB_ID, TASK_ID) \
    VLOG_WRAPPER(level, PLATFORM, JOB_ID, TASK_ID)
#undef LOG_INFO
#define LOG_INFO(PLATFORM, JOB_ID, TASK_ID) \
    LOG_INFO_WRAPPER(PLATFORM, JOB_ID, TASK_ID)
#undef LOG_WRANING
#define LOG_WARNING(PLATFORM, JOB_ID, TASK_ID) \
    LOG_WARNING_WRAPPER(PLATFORM, JOB_ID, TASK_ID)
#undef LOG_ERROR
#define LOG_ERROR(PLATFORM, JOB_ID, TASK_ID) \
    LOG_ERROR_WRAPPER(PLATFORM, JOB_ID, TASK_ID)


void Worker::execute(const PushTaskRequest *pushTaskRequest) {
    auto type = pushTaskRequest->task().type();
    const auto& job_id = pushTaskRequest->task().job_id();
    const auto& task_id = pushTaskRequest->task().task_id();
    V_VLOG(2, PLATFORM, job_id, task_id) << "Worker::execute task type: " << type;
    if (type == rpc::TaskType::NODE_TASK ||
        type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (pTask == nullptr) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Woker create task failed.";
            return;
        }
        LOG_INFO(PLATFORM, job_id, task_id) << " 🚀 Worker start execute task ";
        int ret = pTask->execute();
        if (ret != 0) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Error occurs during execute task.";
        }
    } else if (type == rpc::TaskType::NODE_PSI_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "At least 2 nodes srunning with 2PC task now.";
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
            LOG_ERROR(PLATFORM, job_id, task_id) << "Woker create psi task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0)
            LOG_ERROR(PLATFORM, job_id, task_id) << "Error occurs during execute psi task.";
    } else if (type == rpc::TaskType::NODE_PIR_TASK) {
        if (pushTaskRequest->task().node_map().size() < 2) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "At least 2 nodes srunning with 2PC task now.";
            return;
        }

        const auto& param_map = pushTaskRequest->task().params().param_map();
        int pirType = PirType::ID_PIR;
        auto param_it = param_map.find("pirType");
        if (param_it != param_map.end()) {
            pirType = param_it->second.value_int32();
            V_VLOG(5, PLATFORM, job_id, task_id) << "get pirType: " << pirType;
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
            LOG_ERROR(PLATFORM, job_id, task_id) << "Woker create pir task failed.";
            return ;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Error occurs during execute pir task.";
        }
    } else {
        LOG_WARNING(PLATFORM, job_id, task_id) << "unsupported Requested task type: " << type;
    }
}


// PIR /PSI Server worker execution
void Worker::execute(const ExecuteTaskRequest *taskRequest,
                     ExecuteTaskResponse *taskResponse) {
    auto request_type = taskRequest->algorithm_request_case();
    if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
        const auto& task_id = taskRequest->psi_request().task_id();
        const auto& job_id = taskRequest->psi_request().job_id();
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id,
			                             rpc::TaskType::NODE_PSI_TASK,
			                             *taskRequest,
                                         taskResponse,
					                     dataset_service);
        pTask->set_task_info(primihub::PlatFormType::PSI, job_id, task_id);
        if (pTask == nullptr) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Woker create server node task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Error occurs during server node execute task.";
        }
    } else if (request_type == ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        const auto& task_id = taskRequest->pir_request().task_id();
        const auto& job_id = taskRequest->pir_request().job_id();
        V_VLOG(0, PLATFORM, job_id, task_id) << "algorithm_request_case kPirRequest Worker::execute";
        auto dataset_service = nodelet->getDataService();
        auto pTask = TaskFactory::Create(this->node_id,
                                         rpc::TaskType::NODE_PIR_TASK,   // convert into internal task type
                                         *taskRequest,
                                         taskResponse,
                                         dataset_service);
        if (pTask == nullptr) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Woker create server node task failed.";
            return;
        }
        int ret = pTask->execute();
        if (ret != 0) {
            LOG_ERROR(PLATFORM, job_id, task_id) << "Error occurs during server node execute task.";
        }
    } else {
        LOG_WARNING(PLATFORM, "job_id", "task_id") << "Requested task type is not supported.";
    }
}

} // namespace primihub
