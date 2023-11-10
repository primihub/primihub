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
#include <string>
#include "src/primihub/task/semantic/factory.h"
#include "src/primihub/task/semantic/task.h"
#include "base64.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"

using TaskFactory = primihub::task::TaskFactory;
using Process = Poco::Process;
using ProcessHandle = Poco::ProcessHandle;

namespace pb_util = primihub::proto::util;
namespace primihub {

retcode Worker::waitForTaskReady() {
  bool ready = task_ready_future_.get();
  auto TASK_INFO_STR = pb_util::TaskInfoToString(worker_id_);
  if (!ready) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR
        << "Initialize task failed, worker id " << worker_id_ << ".";
    return retcode::FAIL;
  }
  PH_VLOG(7, LogType::kTask)
      << TASK_INFO_STR
      << "task_ready_future_ task is ready for worker id: " << worker_id_;
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
      return TaskRunMode::THREAD;
    }
  }
  return task_run_mode_;
}

retcode Worker::ExecuteTaskByThread(const PushTaskRequest* task_request) {
  auto type = task_request->task().type();
  const auto& task_info = task_request->task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  VLOG(2) << TASK_INFO_STR << "Worker::execute task type: " << type;
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
    LOG(ERROR) << TASK_INFO_STR << "Woker create task failed.";
    task_ready_promise_.set_value(false);
    return retcode::FAIL;
  }
  task_ready_promise_.set_value(true);
  LOG(INFO) << TASK_INFO_STR << "Worker start execute task ";
  int ret = task_ptr->execute();
  if (ret != 0) {
    LOG(ERROR) << TASK_INFO_STR << "Error occurs during execute task.";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode Worker::ExecuteTaskByProcess(const PushTaskRequest* task_request) {
  this->task_ptr = std::make_shared<task::TaskBase>();
  auto& server_config = ServerConfig::getInstance();
  PushTaskRequest send_request;
  send_request.CopyFrom(*task_request);
  const auto& task_info = send_request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  // change datasetid to dataset access info
  auto& dataset_service = nodelet->getDataService();
  auto party_datasets = send_request.mutable_task()->mutable_party_datasets();
  auto param_map_ptr =
      send_request.mutable_task()->mutable_params()->mutable_param_map();
  auto self_party_name = send_request.task().party_name();
  for (auto& [party_name, datasets] : *party_datasets) {
    if (party_name != self_party_name) {
      continue;
    }
    auto datset_ptr = datasets.mutable_data();
    for (auto& [dataset_name, dataset_id] : *(datset_ptr)) {
      auto driver = dataset_service->getDriver(dataset_id);
      if (driver == nullptr) {
        LOG(WARNING)
            << TASK_INFO_STR
            << "no dataset access info is found for id: " << dataset_id;
        continue;
      }
      auto& access_info = driver->dataSetAccessInfo();
      if (access_info == nullptr) {
        LOG(WARNING)
            << TASK_INFO_STR
            << "no dataset access info is found for id: " << dataset_id;
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
  std::string request_str = pb_util::TaskRequestToString(send_request);
  LOG(INFO) << TASK_INFO_STR << "Task Process::execute: " << request_str;

  std::string task_config_str;
  bool status = send_request.SerializeToString(&task_config_str);
  if (!status) {
    LOG(ERROR) << TASK_INFO_STR << "serialize task config failed";
    return retcode::FAIL;
  }
  std::string request_base64_str = base64_encode(task_config_str);

  // std::future<std::string> data;
  std::string current_process_dir = getCurrentProcessDir();
  VLOG(5) << TASK_INFO_STR << "current_process_dir: " << current_process_dir;
  std::string execute_app = current_process_dir + "/task_main";
  std::string execute_cmd;
  std::vector<std::string> args;
  args.push_back("--node_id=" + this->node_id);
  args.push_back("--config_file=" + server_config.getConfigFile());
  args.push_back("--request=" + request_base64_str);
  args.push_back("--request_id=" + task_info.request_id());
  // using POCO process
  Poco::Pipe outPipe;
  auto handle_ = Process::launch(execute_app, args, 0, &outPipe, &outPipe);
  Poco::PipeInputStream istr(outPipe);

  task_ready_promise_.set_value(true);
  LOG(INFO) << TASK_INFO_STR << "Worker start execute task ";
  process_handler_ = std::make_unique<ProcessHandle>(handle_);
  std::string log_content;
  while (std::getline(istr, log_content)) {
    if (log_content.empty()) {
      continue;
    }
    const char* log_data = log_content.data();
    size_t log_length = log_content.size();
    auto log_sv = std::string_view(log_data, log_length);
    char first_ch = log_sv[0];
    if ((first_ch == 'I' ||
         first_ch == 'W' || first_ch == 'E')) {    // glog format
      size_t name_pos = log_sv.find(']');
      auto prefix_content =
          std::string_view(log_data, name_pos+1);
      auto output_log =
          std::string_view(log_data + name_pos + 1, log_length-name_pos);
      std::cout << prefix_content << " "
                << TASK_INFO_STR << output_log << std::endl;
    } else {
      LOG(INFO) << TASK_INFO_STR << log_content;
    }
  }
  try {
    int ret = handle_.wait();
    if (ret != 0) {
      LOG(ERROR) << TASK_INFO_STR << "ERROR: " << ret;
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
    const auto& task_info = task_status->task_info();
    auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    VLOG(2) << TASK_INFO_STR << " "
            << "New Status: " << pb_util::TaskStatusToString(*task_status);
    return retcode::SUCCESS;
  }
  return retcode::FAIL;
}

retcode Worker::updateTaskStatus(const rpc::TaskStatus& task_status) {
  const auto& status_code = task_status.status();
  const auto& party = task_status.party();
  const auto& status_msg = task_status.message();
  const auto& task_info = task_status.task_info();
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  if (status_code == rpc::TaskStatus::SUCCESS ||
      status_code == rpc::TaskStatus::FAIL) {
    std::unique_lock<std::shared_mutex> lck(final_status_mtx_);
    final_status_[party] = status_code;

    if (status_code == rpc::TaskStatus::FAIL) {
      LOG(ERROR) << "Scheduler, " << TASK_INFO_STR
                 << pb_util::TaskStatusToString(task_status);
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
    VLOG(0) << TASK_INFO_STR
            << "collected finished party count: " << final_status_.size();
  }
  VLOG(0) << TASK_INFO_STR
          << "Update " << pb_util::TaskStatusToString(task_status);
  task_status_.push(task_status);
  return retcode::SUCCESS;
}

retcode Worker::waitUntilTaskFinish() {
  auto ret = task_finish_future_.get();
  VLOG(5) << "waitUntilTaskFinish finished: status: " << static_cast<int>(ret);
  return ret;
}

} // namespace primihub
