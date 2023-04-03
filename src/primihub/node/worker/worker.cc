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
retcode Worker::waitForTaskReady() {
    bool ready = task_ready_future_.get();
    if (!ready) {
        LOG(ERROR) << "Initialize task failed, worker id " << worker_id_ << ".";
        return retcode::FAIL;
    }
    VLOG(7) << "task_ready_future_ task is ready for worker id: " << worker_id_;
    return retcode::SUCCESS;
}

retcode Worker::execute(const PushTaskRequest *pushTaskRequest) {
    auto type = pushTaskRequest->task().type();
    VLOG(2) << "Worker::execute task type: " << type;
    if (type == rpc::TaskType::NODE_TASK ||
        type == rpc::TaskType::TEE_DATAPROVIDER_TASK) {
        auto dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create task failed.";
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
        task_ready_promise_.set_value(true);
        LOG(INFO) << "Worker start execute task ";
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute task.";
            return retcode::FAIL;
        }
    } else if (type == rpc::TaskType::NODE_PSI_TASK) {
        if (pushTaskRequest->task().party_access_info().size() < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task now.";
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
        const auto& param_map = pushTaskRequest->task().params().param_map();
        auto dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create psi task failed.";
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
        task_ready_promise_.set_value(true);
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute psi task.";
            return retcode::FAIL;
        }
    } else if (type == rpc::TaskType::NODE_PIR_TASK) {
        size_t party_node_count = pushTaskRequest->task().party_access_info().size();
        if (party_node_count < 2) {
            LOG(ERROR) << "At least 2 nodes srunning with 2PC task. "
                       << "current_node_size: " << party_node_count;
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
        auto& dataset_service = nodelet->getDataService();
        task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest, dataset_service);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "Woker create pir task failed.";
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
        task_ready_promise_.set_value(true);
        int ret = task_ptr->execute();
        if (ret != 0) {
            LOG(ERROR) << "Error occurs during execute pir task.";
            task_ready_promise_.set_value(false);
            return retcode::FAIL;
        }
    } else {
        LOG(WARNING) << "unsupported Requested task type: " << type;
        task_ready_promise_.set_value(false);
    }
    task_ptr.reset();
    return retcode::SUCCESS;
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
        return;
    }
    if (task_server_ptr) {
        task_server_ptr->kill_task();
    }
}

retcode Worker::fetchTaskStatus(rpc::TaskStatus* task_status) {
    bool has_new_status = task_status_.try_pop(*task_status);
    if (has_new_status) {
        return retcode::SUCCESS;
    }
    return retcode::FAIL;
}

retcode Worker::updateTaskStatus(const rpc::TaskStatus& task_status) {
    const auto& status = task_status.status();
    const auto& party = task_status.party();
    if (status == rpc::TaskStatus::SUCCESS || status == rpc::TaskStatus::FAIL) {
        std::unique_lock<std::shared_mutex> lck(final_status_mtx_);
        final_status_[party] = status;
        if (status == rpc::TaskStatus::FAIL) {
            if (!scheduler_finished.load(std::memory_order::memory_order_relaxed)) {
                task_finish_promise_.set_value(retcode::FAIL);
                scheduler_finished.store(true);
            }
        }
        if (final_status_.size() == party_count_) {
            if (!scheduler_finished.load(std::memory_order::memory_order_relaxed)) {
                task_finish_promise_.set_value(retcode::SUCCESS);
                scheduler_finished.store(true);
            }
        }
        VLOG(5) << "collected finished party count: " << final_status_.size();
    }
    task_status_.push(task_status);
    return retcode::SUCCESS;
}

retcode Worker::waitUntilTaskFinish() {
    auto ret = task_finish_future_.get();
    VLOG(5) << "waitUntilTaskFinish finished: status: " << static_cast<int>(ret);
    return ret;
}

} // namespace primihub
