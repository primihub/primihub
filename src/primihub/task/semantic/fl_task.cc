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
#include <memory>
#include <pybind11/stl.h>
#include "Python.h"
#include "src/primihub/util/util.h"
#include "src/primihub/service/notify/model.h"
// namespace py = pybind11;

using primihub::service::EventBusNotifyDelegate;


PYBIND11_EMBEDDED_MODULE(embeded, module) {
  py::class_<DatasetService, std::shared_ptr<DatasetService>> dataset_service(module, "DatasetService");
}


namespace primihub::task {
FLTask::FLTask(const std::string& node_id,
               const TaskParam* task_param,
               const PushTaskRequest& task_request,
               std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), task_request_(task_request) {

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
        this->node_context_.topic = param_map["topic"].value_string();
    } catch (std::exception& e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return;
    }

    this->node_context_.dumps_func = task_param->code();
    auto input_datasets = task_param->input_datasets();
    for (auto& input_dataset : input_datasets) {
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

    for (auto& vm : vm_list) {
        std::string full_addr = 
            vm.next().ip() + ":" + std::to_string(vm.next().port());

        // Key is the combine of node's nodeid and role,
        // and value is 'ip:port'.
        node_addr_map_[vm.next().name()] = full_addr;
    }
    
    {
        uint32_t count = 0;
        auto iter = node_addr_map_.begin();

        LOG(INFO)
            << "Dump node id with role and it's address used by FL algorithm.";

        for (iter; iter != node_addr_map_.end(); iter++) {
            LOG(INFO) << "Node " << iter->first << ": [" << iter->second << "].";
            count++;
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

   
    for (auto &pair : param_map)
        this->params_map_[pair.first] = pair.second.value_string();
    
    std::string node_key = node_id + "_dataset";
    auto iter = param_map.find(node_key);
    if (iter == param_map.end()) {
      LOG(ERROR) << "Can't find dataset name of node " << node_id << ".";
      return;
    }

    this->params_map_["local_dataset"] = iter->second.value_string();

}

FLTask::~FLTask() {
    ph_context_m_.release();
    ph_exec_m_.release();
}

int FLTask::execute() {
    py::gil_scoped_acquire acquire;
    try {
        ph_exec_m_ = py::module::import("primihub.executor").attr("Executor");
        ph_context_m_ = py::module::import("primihub.context");

//        // Run set_task_uuid
//        py::object set_task_uuid;
//        set_task_uuid = ph_context_m_.attr("set_task_uuid");
//        auto task_uuid = set_task_uuid();
//        LOG(INFO) << ">>> task uuid: " << task_uuid;
//        set_task_uuid.release();

        // Run set_node_context method.
        py::object set_node_context; 
        set_node_context = ph_context_m_.attr("set_node_context");
        set_node_context(node_context_.role,
                node_context_.protocol,
                node_context_.topic,
              py::cast(node_context_.datasets));
        set_node_context.release();
        
        // Run set_task_context_dataset_map method. 
        py::object set_task_context_dataset_map;
        set_task_context_dataset_map =
            ph_context_m_.attr("set_task_context_dataset_map");

        for (auto& dataset_meta : this->dataset_meta_map_) {
            LOG(INFO) << "Insert DATASET : " << dataset_meta.first
                      << ", " << dataset_meta.second;
            set_task_context_dataset_map(dataset_meta.first,
                                         dataset_meta.second);
        }

        set_task_context_dataset_map.release();

        // Run set_task_context_params_map. 
        py::object set_task_context_params_map;
        set_task_context_params_map =
            ph_context_m_.attr("set_task_context_params_map");

        for (auto &pair : this->params_map_)
            set_task_context_params_map(pair.first, pair.second);

        {        
          std::string nodelet_addr = 
            this->dataset_service_->getNodeletAddr();
          auto pos = nodelet_addr.find(":");
          set_task_context_params_map("DatasetServiceAddr", 
              nodelet_addr.substr(pos + 1, nodelet_addr.length()));
        }

        set_task_context_params_map.release();

        // Run set_task_context_node_addr_map.
        py::object set_node_addr_map;
        set_node_addr_map = ph_context_m_.attr("set_task_context_node_addr_map");

        for (auto &pair : this->node_addr_map_)
            set_node_addr_map(pair.first, pair.second);

        set_node_addr_map.release();
    } catch (std::exception& e) {
        LOG(ERROR) << "Failed: " << e.what();
        py::gil_scoped_release release;
        return -1;
    }


    try {
        LOG(INFO) << "<<<<<<<<< 🐍 Start executing Python code <<<<<<<<<" << std::endl;

        // Execute python code.
        ph_exec_m_.attr("execute_py")(py::bytes(node_context_.dumps_func));
        LOG(INFO) << "<<<<<<<<< 🐍 Execute Python Code End <<<<<<<<<" << std::endl;

        // Fire task status event
        auto taskId = task_param_.task_id();
        auto submitClientId = task_request_.submit_client_id();
        EventBusNotifyDelegate::getInstance().notifyStatus(taskId, submitClientId, 
                                                            "SUCCESS", 
                                                            "task finished");

    } catch (std::exception &e) {
        py::gil_scoped_release release;
        LOG(ERROR) << "Failed to execute python: " << e.what();
        return -1;
    }

    py::gil_scoped_release release;
    return 0;
}

}  // namespace primihub::task
