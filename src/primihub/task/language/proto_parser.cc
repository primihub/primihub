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
  const auto& party_datasets = task_config.party_datasets();
  VLOG(0) << "party_datasets: " << party_datasets.size();
  for (const auto& [party_name,  dataset_map] : party_datasets) {
    for (const auto& [dataset_index, datasetid] : dataset_map.data()) {
      if (datasetid.empty()) {
        LOG(WARNING) << "dataset for party: " << party_name << " is empty";
        continue;
      }
      this->input_datasets_with_tag_.emplace_back(
          std::make_pair(datasetid, party_name));
    }
  }
  return retcode::SUCCESS;
}

} // namespace primihub::task
