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
#include "src/primihub/task/language/py_parser.h"
#include <glog/logging.h>
#include <iostream>

namespace primihub::task {
/**
 * @brief Parse datasets from python code.
 * @return Dataset names.
 */
retcode PyParser::parseDatasets() {
  auto& push_task = getPushTaskRequest();
  const auto& task_config = push_task.task();
  const auto& party_datasets = task_config.party_datasets();
  VLOG(0) << "party_datasets: " << party_datasets.size();
  for (const auto& [party_name,  dataset_map] : party_datasets) {
    for (const auto& [dataset_index, dataset_id] : dataset_map.data()) {
      if (dataset_id.empty()) {
        LOG(WARNING) << "dataset id for party: " << party_name << " is empty";
        continue;
      }
      VLOG(0) << "party_name: " << party_name << " dataset_id: " << dataset_id;
      auto dataset_with_tag = std::make_pair(dataset_id, party_name);
      this->input_datasets_with_tag_.push_back(std::move(dataset_with_tag));
    }
  }
  return retcode::SUCCESS;
}

PyParser::~PyParser() {}

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
}

retcode PyParser::parseNodes() {
  // TODO
  return retcode::SUCCESS;
}

} // namespace primihub::task
