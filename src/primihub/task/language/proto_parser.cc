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
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/util/util.h"

using primihub::rpc::Params;
using primihub::rpc::ParamValue;

namespace primihub::task {

ProtoParser::ProtoParser(const PushTaskRequest &pushTaskRequest)
    : LanguageParser(pushTaskRequest) {}

retcode ProtoParser::parseDatasets() {

    // get datasets params first
    auto dataset_names = this->pushTaskRequest_.task().input_datasets();
    // get dataset from params
    auto  param_map =
        this->pushTaskRequest_.task().params().param_map();
    try {
        for (auto &dataset_name : dataset_names) {
            ParamValue datasets = param_map[dataset_name];
            std::string datasets_str = datasets.value_string();
            std::vector<std::string> dataset_list;
            str_split(datasets_str, &dataset_list, ';');
            for (auto &dataset : dataset_list) {
                DLOG(INFO) << "dataset: " << dataset;
                auto dataset_with_tag = std::make_pair(dataset, dataset_name);
                this->input_datasets_with_tag_.push_back(std::move(dataset_with_tag));
            }
        }
    } catch (const std::exception &e) {
        LOG(ERROR) << "parse dataset error: " << e.what();
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

} // namespace primihub::task
