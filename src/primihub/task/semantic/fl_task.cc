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
#include <pybind11/embed.h>
// #include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "Python.h"
#include "src/primihub/util/util.h"
// namespace py = pybind11;

namespace primihub::task {
FLTask::FLTask(const std::string& node_id,
               const TaskParam* task_param,
               std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {
    // Convert TaskParam to NodeContext
    auto param_map = task_param->params().param_map();

    auto& node_map_ref = task_param->node_map();
    std::string server_ip_str;
    for (auto iter = node_map_ref.begin(); iter != node_map_ref.end(); iter++) {
        if (!server_ip_str.empty()) {
            break;
        }
        auto& vm_list = iter->second.vm();
        for (auto& vm : vm_list) {
            if (vm.next().link_type() == primihub::rpc::LinkType::SERVER) {
                server_ip_str = iter->second.ip();
                break;
            }
            LOG(INFO) << "link type: " << vm.next().link_type();
        }
        LOG(INFO) << " --- iter first node ip: " << iter->second.ip()
                  << ", port: " << iter->second.port();
    }

    try {
        this->node_context_.role = param_map["role"].value_string();
        this->node_context_.protocol = param_map["protocol"].value_string();
    } catch (std::exception& e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return;
    }

    this->node_context_.dumps_func = task_param->code();
    auto input_datasets = task_param->input_datasets();
    for (auto& input_dataset : input_datasets)
        this->node_context_.datasets.push_back(input_dataset);
    
    auto iter = task_param->node_map().find(node_id);
    if (iter == task_param->node_map().end()) {
      LOG(ERROR) << "Can't find Node structure with node id " << node_id << ".";
      return;
    }
    
    auto &vm_list = iter->second.vm();
    for (auto &vm : vm_list) {
      std::string full_addr = vm.next().ip() + ":" + 
                              std::to_string(vm.next().port());
      node_addr_map_[vm.next().name()] = full_addr;
    }

    {
      uint32_t count = 0;
      auto iter = node_addr_map_.begin();

      LOG(INFO) << "Dump node id and it's address used by FL algorithm.";
      for (iter; iter != node_addr_map_.end(); iter ++) {
        LOG(INFO) << "Node " << iter->first << ": [" 
                  << iter->second << "].";
        count ++;
      }

      LOG(INFO) << "Dump finish, dump count " << count << "."; 
    }

    // Set datasets meta list in context
    for (auto& input_dataset : input_datasets) {
        // Get dataset path from task params map
        auto data_meta = param_map.find(input_dataset);
        if (data_meta == param_map.end()) {
            LOG(ERROR) << "Failed to find dataset: " << input_dataset;
            return;
        }
        std::string data_path = data_meta->second.value_string();
        this->dataset_meta_map_.insert(
            std::make_pair(input_dataset, data_path));
    }

    // output file path
    this->model_file_path_ = param_map["modelFileName"].value_string();
    this->host_lookup_file_path_ = param_map["hostLookupTable"].value_string();
    this->guest_lookup_file_path_ = param_map["guestLookupTable"].value_string();
    this->predict_file_path_ = param_map["predictFileName"].value_string();
    this->indicator_file_path_ = param_map["indicatorFileName"].value_string();
}

FLTask::~FLTask() {
    set_task_context_predict_file_.release();
    set_task_context_indicator_file_.release();
    set_task_context_model_file_.release();
    set_task_context_host_lookup_file_.release();
    set_task_context_guest_lookup_file_.release();

    set_task_context_dataset_map_.release();
    set_task_context_func_params_.release();
    set_node_context_.release();
    set_task_context_node_map_.release();

    ph_context_m.release();
    ph_exec_m.release();
}

int FLTask::execute() {

    LOG(INFO) << "🔐 Before gil_scoped_acquire, GIL is "
                << ((PyGILState_Check() == 1) ? "hold" : "not hold")
                << std::endl;
    /* Acquire GIL before calling Python code */
    py::gil_scoped_acquire acquire;
    try {
        LOG(INFO) << "🔐 Before ph_context, GIL is "
                  << ((PyGILState_Check() == 1) ? "hold" : "not hold")
                  << " now is runing " << std::endl;

        ph_exec_m = py::module::import("primihub.executor").attr("Executor");
        ph_context_m = py::module::import("primihub.context");
        set_node_context_ = ph_context_m.attr("set_node_context");

        set_node_context_(node_context_.role, node_context_.protocol,
                          py::cast(node_context_.datasets));

        set_task_context_dataset_map_ =
            ph_context_m.attr("set_task_context_dataset_map");
        for (auto& dataset_meta : this->dataset_meta_map_) {
            LOG(INFO) << "<<<<<<< insert DATASET : " << dataset_meta.first
                      << ", " << dataset_meta.second;
            set_task_context_dataset_map_(dataset_meta.first,
                                          dataset_meta.second);
        }
        set_task_context_predict_file_ =
            ph_context_m.attr("set_task_context_model_file");
        set_task_context_predict_file_(this->model_file_path_);

        set_task_context_predict_file_ =
            ph_context_m.attr("set_task_context_host_lookup_file");
        set_task_context_predict_file_(this->host_lookup_file_path_);

        set_task_context_indicator_file_ =
            ph_context_m.attr("set_task_context_guest_lookup_file");
        set_task_context_indicator_file_(this->guest_lookup_file_path_);

        set_task_context_predict_file_ =
            ph_context_m.attr("set_task_context_predict_file");
        set_task_context_predict_file_(this->predict_file_path_);

        set_task_context_indicator_file_ =
            ph_context_m.attr("set_task_context_indicator_file");
        set_task_context_indicator_file_(this->indicator_file_path_); 

        set_task_context_node_map_ = 
            ph_context_m.attr("set_task_context_node_addr_map");
        
        for (auto nodeid_addr : this->node_addr_map_)
          set_task_context_node_map_(nodeid_addr.first, nodeid_addr.second);

        //   LOG(INFO) << node_context_.dumps_func;
        LOG(INFO) << "🔐 After ph_context, GIL is "
                  << ((PyGILState_Check() == 1) ? "hold" : "not hold")
                  << " now is runing " << std::endl;

        py::gil_scoped_release release;

    } catch (std::exception& e) {
        LOG(ERROR) << "Failed: " << e.what();
        return -1;
    }

    try {
        LOG(INFO) << "🔐  GIL is "
            << ((PyGILState_Check() == 1) ? "hold" : "not hold")
            << " now is runing " << std::endl;
        LOG(INFO) << "<<<<<<<<< 🐍 Start executing Python code <<<<<<<<<" << std::endl;
        // Execute python code.
        ph_exec_m.attr("execute_py")(py::bytes(node_context_.dumps_func));
        LOG(INFO) << "<<<<<<<<< 🐍 Execute Python Code End <<<<<<<<<" << std::endl;

    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to execute python: " << e.what();
        return -1;
    }

    return 0;
}

}  // namespace primihub::task
