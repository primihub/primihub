/* Copyright 2022 Primihub

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
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/process/search_path.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include "src/primihub/util/util.h"
#include "base64.h"
#include <google/protobuf/text_format.h>

// using primihub::service::EventBusNotifyDelegate;
namespace bp = boost::process;

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
    bool status = this->task_request_->SerializeToString(&task_config_str);
    if (!status) {
        LOG(ERROR) << "serialize task config failed";
        return -1;
    }
    std::string request_base64_str = base64_encode(task_config_str);
    boost::asio::io_service ios;
    // std::future<std::string> data;
    std::string current_process_dir = getCurrentProcessDir();
    VLOG(5) << "current_process_dir: " << current_process_dir;
    std::string execute_app = current_process_dir + "/py_main";
    std::string execute_cmd;
    execute_cmd.append(execute_app).append(" ")
        .append("--node_id=").append(node_id_).append(" ")
        .append("--config_file=").append(server_config.getConfigFile()).append(" ")
        .append("--request=").append(request_base64_str);
    // LOG(INFO) << "execute_cmd: " << execute_cmd;
    bp::child c(execute_cmd, //set the input
              bp::std_in.close(),
              bp::std_out > stdout, //so it can be written without anything
              bp::std_err > stdout,
              ios);
    ios.run(); //this will actually block until the py_main is finished
    uint64_t count{0};
    do {
        if (count < 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (has_stopped()) {
            LOG(ERROR) << "begin to terminate py_main";
            if (c.running()) {
                ios.stop();
                c.terminate();
            }
            break;
        }
        count++;
    } while (c.running());

    int result = c.exit_code();
    LOG(INFO) << "py_main executes result code: " << result;
    if (result != 0) {
        // auto err =  data.get();
        // if (!err.empty()) {
        //     LOG(INFO) << err;
        //     auto& event_notify_delegeate = EventBusNotifyDelegate::getInstance();
        //     event_notify_delegeate.notifyStatus(
        //         job_id(), task_id(), submit_client_id(), "FAIL", err);
        // }
        return -1;
    }
    // fut.get();
    return 0;
}

}  // namespace primihub::task
