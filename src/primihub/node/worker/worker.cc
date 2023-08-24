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
#include <memory>

#include "src/primihub/task/semantic/factory.h"
#include "src/primihub/task/semantic/task.h"
#include "base64.h"

using TaskFactory = primihub::task::TaskFactory;
using Process = Poco::Process;
using ProcessHandle = Poco::ProcessHandle;

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

retcode Worker::execute(const PushTaskRequest *task_request) {
  auto ret{retcode::SUCCESS};
  auto task_run_mode = ExecuteMode(*task_request);
  switch (task_run_mode) {
  case TaskRunMode::THREAD:
    ret = ExecuteTaskByThread(task_request);
    break;
  case TaskRunMode::PROCESS:
    ret = ExecuteTaskByProcess(task_request);
    break;
  }
  return ret;
}

Worker::TaskRunMode Worker::ExecuteMode(const PushTaskRequest& request) {
  const auto& map_info = request.task().params().param_map();
  auto it = map_info.find("psiTag");
  if (it != map_info.end()) {
    int psi_tag = it->second.value_int32();
    if (psi_tag == rpc::PsiTag::TEE) {
      LOG(ERROR) << "here";
      return TaskRunMode::THREAD;
    }
  }
  return task_run_mode_;
}

retcode Worker::ExecuteTaskByThread(const PushTaskRequest* task_request) {
  auto type = task_request->task().type();
  VLOG(2) << "Worker::execute task type: " << type;
  auto& dataset_service = nodelet->getDataService();
#ifdef SGX
  auto& ra_service = nodelet->GetRaService();
  auto& executor = nodelet->GetTeeExecutor();
  auto ra_service_ptr = reinterpret_cast<void*>(ra_service.get());
  auto tee_executor_ptr = reinterpret_cast<void*>(executor.get());
  task_ptr = TaskFactory::Create(this->node_id,
      *task_request, dataset_service, ra_service_ptr, tee_executor_ptr);
#else
  task_ptr = TaskFactory::Create(this->node_id, *task_request,
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

retcode Worker::ExecuteTaskByProcess(const PushTaskRequest* task_request) {
  this->task_ptr = std::make_shared<task::TaskBase>();
  auto& server_config = ServerConfig::getInstance();
  PushTaskRequest send_request;
  send_request.CopyFrom(*task_request);
  // change datasetid to datset access info
  auto& dataset_service = nodelet->getDataService();
  auto party_datasets = send_request.mutable_task()->mutable_party_datasets();
  auto param_map_ptr =
      send_request.mutable_task()->mutable_params()->mutable_param_map();
  for (auto& [party_name, datasets] : *party_datasets) {
    auto datset_ptr = datasets.mutable_data();
    for (auto& [dataset_name, dataset_id] : *(datset_ptr)) {
      auto driver = dataset_service->getDriver(dataset_id);
      if (driver == nullptr) {
        LOG(WARNING) << "no dataset access info is found for id: " << dataset_id;
        continue;
      }
      auto& access_info = driver->dataSetAccessInfo();
      if (access_info == nullptr) {
        LOG(WARNING) << "no dataset access info is found for id: " << dataset_id;
        continue;
      }
      rpc::ParamValue pv;
      pv.set_is_array(false);
      pv.set_value_string(dataset_id);
      (*param_map_ptr)[dataset_name] = std::move(pv);
      dataset_id = access_info->toString();
    }
    datasets.set_dataset_detail(true);
  }
  std::string str;
  google::protobuf::TextFormat::PrintToString(send_request, &str);
  LOG(INFO) << "Task Process::execute: " << str;


  std::string task_config_str;
  bool status = send_request.SerializeToString(&task_config_str);
  if (!status) {
    LOG(ERROR) << "serialize task config failed";
    return retcode::FAIL;
  }
  std::string request_base64_str = base64_encode(task_config_str);

  // std::future<std::string> data;
  std::string current_process_dir = getCurrentProcessDir();
  VLOG(5) << "current_process_dir: " << current_process_dir;
  std::string execute_app = current_process_dir + "/task_main";
  std::string execute_cmd;
  std::vector<std::string> args;
  args.push_back("--node_id=" + this->node_id);
  args.push_back("--config_file=" + server_config.getConfigFile());
  args.push_back("--request=" + request_base64_str);
  args.push_back("--request_id=" +
                 task_request->task().task_info().request_id());
  // execute_cmd.append(execute_app).append(" ")
  //     .append("--node_id=").append(node_id_).append(" ")
  //     .append("--config_file=")
  //     .append(server_config.getConfigFile()).append(" ")
  //     .append("--request=").append(request_base64_str);
  // using POCO process
  auto handle_ = Process::launch(execute_app, args);
  task_ready_promise_.set_value(true);
  LOG(INFO) << "Worker start execute task ";
  process_handler_ = std::make_unique<ProcessHandle>(handle_);
  try {
    int ret = handle_.wait();
    if (ret != 0) {
      LOG(ERROR) << "ERROR: " << ret;
      return retcode::FAIL;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return retcode::FAIL;
  }
  process_handler_.reset();
  // POCO process end
  return retcode::SUCCESS;
}

// kill task which is running in the worker
void Worker::kill_task() {
  if (task_ptr) {
    task_ptr->kill_task();
  }
  if (process_handler_ != nullptr) {
    Poco::Process::kill(*process_handler_);
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
