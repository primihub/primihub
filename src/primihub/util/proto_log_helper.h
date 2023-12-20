/*
* Copyright (c) 2023 by PrimiHub
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
#ifndef SRC_PRIMIHUB_UTIL_PROTO_LOG_HELPER_H_
#define SRC_PRIMIHUB_UTIL_PROTO_LOG_HELPER_H_

#include <string>
#include "src/primihub/protos/worker.pb.h"
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

namespace rpc = primihub::rpc;

namespace primihub::proto::util {
std::string TaskInfoToString(const rpc::TaskContext& task_info,
                              bool json_format = false);
std::string TaskInfoToString(const std::string& task_info);
std::string TaskInfoToString(const std::string& request_id, const std::string& task_id);
std::string TaskConfigToString(const rpc::Task& task_config);
std::string TaskRequestToString(const rpc::PushTaskRequest& task_req);
std::string TaskStatusToString(const rpc::TaskStatus& status);
template <typename T>
std::string TypeToString(const T& pb_item) {
  std::string info;
  auto pb_printer = google::protobuf::TextFormat::Printer();
  pb_printer.SetSingleLineMode(true);
  pb_printer.PrintToString(pb_item, &info);
  return info;
}

}  // namespace primihub::proto::util
#endif  // SRC_PRIMIHUB_UTIL_PROTO_LOG_HELPER_H_
