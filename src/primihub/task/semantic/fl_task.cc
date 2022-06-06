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

#include "src/primihub/task/semantic/fl_task.h"
#include <glog/logging.h>
#include <pybind11/embed.h>
// #include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// namespace py = pybind11;

namespace primihub::task {
FLTask::FLTask(const std::string &node_id, const TaskParam *task_param,
               std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {

    // Convert TaskParam to NodeContext
    auto param_map = task_param->params().param_map();
    try {
        this->node_context_.role = param_map["role"].value_string();
        this->node_context_.protocol = param_map["protocol"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return;
    }
    this->node_context_.dumps_func = task_param->code();
    auto input_datasets = task_param->input_datasets();
    for (auto &input_dataset : input_datasets) {
        this->node_context_.datasets.push_back(input_dataset);
    }
    // get next peer address
    auto node_it = task_param->node_map().find(node_id);
    if (node_it == task_param->node_map().end()) {
        LOG(ERROR) << "Failed to find node: " << node_id;
        return;
    }
    auto vm_list = node_it->second.vm();
    if (vm_list.empty()) {
        LOG(ERROR) << "Failed to find vm list for node: " << node_id;
        return;
    }
    for (auto &vm : vm_list) {
        auto ip = vm.next().ip();
        auto port = vm.next().port();
        next_peer_address_ = ip + ":" + std::to_string(port);
        LOG(INFO) << "Next peer address: " << next_peer_address_;
        break;
    }


}

FLTask::~FLTask() {
    set_node_context_.release();
    ph_context_m.release();
    ph_exec_m.release();

}

int FLTask::execute() {
     try {
          LOG(INFO) << "before gil_scoped_acquire" << std::endl;
          py::gil_scoped_acquire acquire;
          LOG(INFO) << "before ph_context" << std::endl;

          ph_exec_m = py::module::import("primihub.executor").attr("Executor");
          ph_context_m = py::module::import("primihub.context");
          set_node_context_ = ph_context_m.attr("set_node_context");

          set_node_context_(node_context_.role, 
                    node_context_.protocol, 
                    py::cast(node_context_.datasets),
                    this->next_peer_address_);
          
          LOG(INFO) << node_context_.dumps_func;
          // Execute python code.
          ph_exec_m.attr("execute1")(py::bytes(node_context_.dumps_func));
    
     } catch (std::exception &e) {
          LOG(ERROR) << "Failed to excute python: " << e.what();
          return -1;
    }
     
    return 0;
}

} // namespace primihub::task
