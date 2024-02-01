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
#ifndef SRC_PRIMIHUB_CLI_TASK_CONFIG_PARSER_H_
#define SRC_PRIMIHUB_CLI_TASK_CONFIG_PARSER_H_
#include <string>
#include <nlohmann/json.hpp>
#include "src/primihub/common/common.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/cli/cli.h"

namespace primihub {
struct DownloadFileInfo {
  std::string remote_file_path;
  std::string save_as;
};
using DownloadFileListType = std::vector<DownloadFileInfo>;
using TaskFlow = rpc::PushTaskRequest;
retcode BuildRequestWithTaskConfig(const nlohmann::json& js,
                                   TaskFlow* request);
retcode BuildFederatedRequest(const nlohmann::json& js_task_config,
                              rpc::Task* task_ptr);
void fillParamByScalar(const std::string& value_type,
                       const nlohmann::json& obj, rpc::ParamValue* pv);
void fillParamByArray(const std::string& value_type,
                      const nlohmann::json& obj, rpc::ParamValue* pv);
retcode ParseTaskConfigFile(const std::string& file_path,
                            TaskFlow* task_flow,
                            DownloadFileListType* download_file_cfg);
retcode ParseDownFileConfig(const nlohmann::json& js_cfg,
                            DownloadFileListType* download_file_cfg);

retcode DownloadData(primihub::SDKClient& client,
                     const rpc::TaskContext& task_info,
                     const DownloadFileListType& download_files_cfg);
void RunTask(const std::string& task_config_file, SDKClient& client);
}  // namespace primihub
#endif  // SRC_PRIMIHUB_CLI_TASK_CONFIG_PARSER_H_
