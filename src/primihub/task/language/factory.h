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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_

#include <glog/logging.h>
#include <memory>

#include "src/primihub/task/language/parser.h"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"

using primihub::rpc::Language;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::TaskType;
using primihub::service::DatasetService;

namespace pb_util = primihub::proto::util;
namespace primihub::task {
using primihub::rpc::Language;
class LanguageParserFactory {
 public:
  static std::shared_ptr<LanguageParser> Create(
      const PushTaskRequest &task_request) {
    auto language = task_request.task().language();
    std::shared_ptr<LanguageParser> parser_ptr{nullptr};
    switch (language) {
    case Language::PROTO:
      parser_ptr = std::make_shared<ProtoParser>(task_request);
      break;
    case Language::PYTHON:
      parser_ptr = std::make_shared<PyParser>(task_request);
      break;
    default:
      const auto& task_info = task_request.task().task_info();
      std::string task_inof_str = pb_util::TaskInfoToString(task_info);
      LOG(WARNING) << task_inof_str << "Unsupported language: " << language;
      break;
    }
    return parser_ptr;
  }
};  // class LanguageParserFactory
}   // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_
