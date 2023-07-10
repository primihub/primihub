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
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/util/util.h"

using primihub::rpc::Params;
using primihub::rpc::ParamValue;

namespace primihub::task {
ProtoParser::ProtoParser(const rpc::PushTaskRequest& task_request)
    : LanguageParser(task_request) {}

retcode ProtoParser::parseDatasets() {
  const auto& task_config = this->getPushTaskRequest().task();
  // get datasets params first
  auto dataset_names = task_config.input_datasets();
  // get dataset from params
  auto param_map = task_config.params().param_map();
  try {
    for (auto &dataset_name : dataset_names) {
      ParamValue datasets = param_map[dataset_name];
      std::string datasets_str = datasets.value_string();
      std::vector<std::string> dataset_list;
      str_split(datasets_str, &dataset_list, ';');
      for (auto &dataset : dataset_list) {
        VLOG(5) << "dataset: " << dataset << " dataset_name: " << dataset_name;
        auto dataset_with_tag = std::make_pair(dataset, dataset_name);
        this->input_datasets_with_tag_.push_back(std::move(dataset_with_tag));
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "parse dataset error: " << e.what();
    return retcode::FAIL;
  }
  const auto& party_datasets = task_config.party_datasets();
  VLOG(0) << "party_datasets: " << party_datasets.size();
  for (const auto& [party_name,  dataset_map] : party_datasets) {
    for (const auto& [dataset_index, datasetid] : dataset_map.data()) {
      this->input_datasets_with_tag_.emplace_back(std::make_pair(datasetid, party_name));
    }
  }
  return retcode::SUCCESS;
}

} // namespace primihub::task
