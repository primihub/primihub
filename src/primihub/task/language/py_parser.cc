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
#include <mutex>

#include "src/primihub/task/language/py_parser.h"

std::mutex global_mtx;
namespace primihub::task {
/**
 * @brief Parse datasets from python code.
 * @return Dataset names.
 */
retcode PyParser::parseDatasets() {
  auto& push_task = getPushTaskRequest();
  const auto& task_config = push_task.task();
  const auto& party_datasets = task_config.party_datasets();
  LOG(ERROR) << "party_datasets: " << party_datasets.size();
  for (const auto& [key,  dataset_ids] : party_datasets) {
    for (const auto& dataset_id : dataset_ids.item()) {
      LOG(ERROR) << "role: " << key << " dataset_id: " << dataset_id;
      auto dataset_with_tag = std::make_pair(dataset_id, key);
      this->input_datasets_with_tag_.push_back(std::move(dataset_with_tag));
    }
  }
  return retcode::SUCCESS;
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
retcode PyParser::parseTask() {
  return retcode::SUCCESS;
  try {
    std::lock_guard<std::mutex> lck(global_mtx);
    py::gil_scoped_acquire acquire;
    ph_context_ = py::module::import("primihub.context").attr("Context");
    ph_exec_m_ = py::module::import("primihub.executor").attr("Executor");
    ph_exec_m_.attr("execute")(this->py_code_);
    // // get protocol
    // procotol_ = ph_context_.attr("get_protocol")().cast<std::string>();
    // // get roles
    // auto roles = ph_context_.attr("get_roles")().cast<py::list>();
    // // get func params map
    auto func_params_ =
        ph_context_.attr("get_func_params_map")().cast<py::tuple>();

    // for (auto &role : roles) {
    //   this->roles_.push_back(role.cast<std::string>());
    // }

    // get NodeContext map
    auto nodes_context_map = ph_context_.attr("nodes_context").cast<py::dict>();
    for (auto &node_context : nodes_context_map) {
      auto node_context_obj = node_context.second.cast<py::object>();
      NodeContext _node_context;
      _node_context.role = node_context_obj.attr("role").cast<std::string>();
      _node_context.dumps_func =
          node_context_obj.attr("dumps_func").cast<std::string>();
      // _node_context.task_type = task_type;
      auto& push_request = this->getPushTaskRequest();
      const auto& party_datasets = push_request.task().party_datasets();
      for (const auto& [role, datasets] : party_datasets) {
        for (const auto& dataset : datasets.item()) {
          this->input_datasets_with_tag_.push_back(std::make_pair(dataset, role));
        }
      }
      // for (auto& dataset : datasets) {
      //   VLOG(3) << "Python assign dataset: " << dataset.cast<std::string>();
      //   _node_context.datasets.push_back(dataset.cast<std::string>());
      //   // Datasets with role tag.
      //   this->input_datasets_with_tag_.push_back(
      //       std::make_pair(dataset.cast<std::string>(), _node_context.role));
      // }

      // auto dataset_port_map =
      //     node_context_obj.attr("dataset_port_map").cast<py::dict>();
      // for (auto ds_port : dataset_port_map) {
      //   std::string dataset = ds_port.first.cast<std::string>();
      //   std::string port = ds_port.second.cast<std::string>();
      //   _node_context.dataset_port_map[dataset] = port;
      // }
      // this->nodes_context_map_[_node_context.role] = _node_context;
    }
    // clean code parse result
    ph_context_.attr("clean_content")();
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to parse python: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode PyParser::parseNodes() {
  // TODO
  return retcode::SUCCESS;
}

} // namespace primihub::task
