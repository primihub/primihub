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

std::string TaskStatusCodeToString(rpc::TaskStatus::StatusCode code) {
  switch (code) {
  case rpc::TaskStatus::RUNNING:
    return "RUNNING";
  case rpc::TaskStatus::SUCCESS:
    return "SUCCESS";
  case rpc::TaskStatus::FAIL:
    return "FAIL";
  case rpc::TaskStatus::NONEXIST:
    return "NONEXIST";
  case rpc::TaskStatus::FINISHED:
    return "FINISHED";
  default:
    return "UNKNOWN";
  }
}

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
  auto dataset_service = nodelet->getDataService();
#ifdef SGX
  auto& ra_service = nodelet->GetRaService();
  auto& executor = nodelet->GetTeeExecutor();
  auto ra_service_ptr = reinterpret_cast<void*>(ra_service.get());
  auto tee_executor_ptr = reinterpret_cast<void*>(executor.get());
  task_ptr = TaskFactory::Create(this->node_id,
      *pushTaskRequest, dataset_service, ra_service_ptr, tee_executor_ptr);
#else
  task_ptr = TaskFactory::Create(this->node_id, *pushTaskRequest,
                                 dataset_service);
#endif
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
  return retcode::SUCCESS;
}

// kill task which is running in the worker
void Worker::kill_task() {
  if (task_ptr) {
    task_ptr->kill_task();
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
  const auto& status_code = task_status.status();
  const auto& party = task_status.party();
  const auto& request_id = task_status.task_info().request_id();
  if (status_code == rpc::TaskStatus::SUCCESS ||
      status_code == rpc::TaskStatus::FAIL) {
    std::unique_lock<std::shared_mutex> lck(final_status_mtx_);
    final_status_[party] = status_code;

    if (status_code == rpc::TaskStatus::FAIL) {
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
    VLOG(0) << "collected finished party count: " << final_status_.size();
  }
  std::string task_status_str = TaskStatusCodeToString(status_code);
  VLOG(0) << "Request id: " << request_id << " "
          << "update party: " << party << " "
          << "status to: " << task_status_str;
  task_status_.push(task_status);
  return retcode::SUCCESS;
}

retcode Worker::waitUntilTaskFinish() {
  auto ret = task_finish_future_.get();
  VLOG(5) << "waitUntilTaskFinish finished: status: " << static_cast<int>(ret);
  return ret;
}

} // namespace primihub
