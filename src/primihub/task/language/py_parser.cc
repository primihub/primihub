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
#include <glog/logging.h>
#include <iostream>

#include "src/primihub/task/language/py_parser.h"


namespace primihub::task {
/**
 * @brief Parse datasets from python code.
 * @return Dataset names.
 */
void PyParser::parseDatasets() {

}

PyParser::~PyParser() {
    ph_context_.release();
    ph_exec_m_.release();
    // py::gil_scoped_release;
}

/**
 * @brief Get 
 *            1. What datasets to use. 
 *            2. What protocol to use.
 *            3. What roles join nodes should have. 
 *            4. What python code and params to execute in each role.
 *         from python code.
 */
void PyParser::parseTask() {

    try {
        py::gil_scoped_acquire acquire;
        ph_context_ = py::module::import("primihub.context").attr("Context");
        ph_exec_m_ = py::module::import("primihub.executor").attr("Executor");
        ph_exec_m_.attr("execute")(this->py_code_);
        
        // get protocol
        procotol_ = ph_context_.attr("get_protocol")().cast<std::string>();
        
        // get roles
        auto roles = ph_context_.attr("get_roles")().cast<py::list>();

        // get func params map
        auto func_params_ = ph_context_.attr("get_func_params_map")().cast<py::tuple>();

        for (auto &role : roles) {
            std::cout << role.cast<std::string>() << std::endl;
            this->roles_.push_back(role.cast<std::string>());
        }

        // get NodeContext map
        auto nodes_context_map = ph_context_.attr("nodes_context").cast<py::dict>();
        for (auto &node_context : nodes_context_map) {
            auto node_context_obj = node_context.second.cast<py::object>();
            NodeContext _node_context;
            _node_context.role = node_context_obj.attr("role").cast<std::string>();
            _node_context.protocol = node_context_obj.attr("protocol").cast<std::string>();
            _node_context.dumps_func = node_context_obj.attr("dumps_func").cast<std::string>();
            auto datasets = node_context_obj.attr("datasets").cast<py::list>();
            for (auto &dataset : datasets) {
                LOG(INFO) << "Python assign dataset: " << dataset.cast<std::string>();
                _node_context.datasets.push_back(dataset.cast<std::string>());
                // Datasets with role tag.
                this->input_datasets_with_tag_.push_back(std::make_pair(dataset.cast<std::string>(), _node_context.role));
            }
            this->nodes_context_map_[_node_context.role] = _node_context;
        }
        
    } catch  (std::exception &e) {
          LOG(ERROR) << "Failed to parse python: " << e.what();
          return;
    }
}

void PyParser::parseNodes() {
    // TODO
}

} // namespace primihub::task
