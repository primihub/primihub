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
#include "src/primihub/task/semantic/tee_task.h"

#include <glog/logging.h>
#include <pybind11/embed.h>

namespace primihub::task {
TEEDataProviderTask::TEEDataProviderTask(
    const std::string& node_id,
    const TaskParam *task_param,
    std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {
    auto param_map = task_param->params().param_map();
    try {
        this->server_addr_ = param_map["server"].value_string();
        this->dataset_ = param_map["dataset"].value_string();
    } catch (std::exception& e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return;
    }

}

TEEDataProviderTask::~TEEDataProviderTask() {
    flight_client_.release();
}

int TEEDataProviderTask::execute() {
    // TODO start apache arrow flight python client to upload dataset.
    py::gil_scoped_acquire acquire;
    flight_client_ = py::module::import("primihub.TEE").attr("flight_client");
    flight_client_.attr("upload_dataset")(this->dataset_, this->server_addr_);

    return 0;
}

} // namespace primihub::task
