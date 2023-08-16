/* Copyright 2022 PrimiHub

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

#include "src/primihub/task/semantic/fl_task.h"
#include <glog/logging.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include "src/primihub/util/util.h"
#include "base64.h"
#include <google/protobuf/text_format.h>

using Process = Poco::Process;
using ProcessHandle = Poco::ProcessHandle;
namespace primihub::task {
FLTask::FLTask(const std::string& node_id,
               const TaskParam* task_param,
               const PushTaskRequest& task_request,
               std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), task_request_(&task_request) {
}

int FLTask::execute() {
    auto& server_config = ServerConfig::getInstance();
    auto node_id_ = node_id();
    PushTaskRequest send_request;
    send_request.CopyFrom(*task_request_);
    // change datasetid to datset access info
    auto& dataset_service = this->getDatasetService();
    auto party_datasets = send_request.mutable_task()->mutable_party_datasets();
    std::vector<std::string> dataset_ids;
    std::string party_name = send_request.task().party_name();
    auto it = party_datasets->find(party_name);
    if (it != party_datasets->end()) {   // current party has datasets
      for (auto& [_, dataset_id] : (*it->second.mutable_data())) {
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
        dataset_id = access_info->toString();
      }
    }
    std::string str;
    google::protobuf::TextFormat::PrintToString(send_request, &str);
    LOG(INFO) << "FLTask::execute: " << str;


    std::string task_config_str;
    bool status = send_request.SerializeToString(&task_config_str);
    if (!status) {
        LOG(ERROR) << "serialize task config failed";
        return -1;
    }
    std::string request_base64_str = base64_encode(task_config_str);

    // std::future<std::string> data;
    std::string current_process_dir = getCurrentProcessDir();
    VLOG(5) << "current_process_dir: " << current_process_dir;
    std::string execute_app = current_process_dir + "/py_main";
    std::string execute_cmd;
    std::vector<std::string> args;
    args.push_back("--node_id="+node_id_);
    args.push_back("--config_file="+server_config.getConfigFile());
    args.push_back("--request="+request_base64_str);
    // execute_cmd.append(execute_app).append(" ")
    //     .append("--node_id=").append(node_id_).append(" ")
    //     .append("--config_file=").append(server_config.getConfigFile()).append(" ")
    //     .append("--request=").append(request_base64_str);
    // using POCO process
    auto handle_ = Process::launch(execute_app, args);
    process_handler_ = std::make_unique<ProcessHandle>(handle_);
    try {
      int ret = handle_.wait();
      if (ret != 0) {
        LOG(ERROR) << "ERROR: " << ret;
        return -1;
      }
    } catch (std::exception& e) {
      LOG(ERROR) << e.what();
      return -1;
    }
    // POCO process end
    return 0;
}

}  // namespace primihub::task
