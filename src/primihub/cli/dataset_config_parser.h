/*
* Copyright (c) 2024 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef SRC_PRIMIHUB_CLI_DATASET_CONFIG_PARSER_H_
#define SRC_PRIMIHUB_CLI_DATASET_CONFIG_PARSER_H_
#include <vector>
#include <string>
#include "src/primihub/common/common.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/cli/cli.h"

namespace primihub {
retcode ParseDatasetMetaInfo(const std::string& config_file,
                             std::vector<rpc::NewDatasetRequest>* requests);
retcode ParseTaskConfigFile(const std::string& file_path,
                            std::vector<rpc::NewDatasetRequest>* requests);
void RegisterDataset(const std::string& task_config_file, SDKClient& client);
}  // namespace primihub
#endif  // SRC_PRIMIHUB_CLI_DATASET_CONFIG_PARSER_H_
