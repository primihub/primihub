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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_PROTO_PARSER_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_PROTO_PARSER_H_

#include <vector>

#include "src/primihub/task/language/parser.h"
#include "src/primihub/common/common.h"

using primihub::service::DatasetWithParamTag;

namespace primihub::task {
class ProtoParser : public LanguageParser {
 public:
  ProtoParser(const rpc::PushTaskRequest &pushTaskRequest);
  ~ProtoParser() = default;
  retcode parseTask() override {return retcode::SUCCESS;}
  retcode parseDatasets() override;
  retcode parseNodes() override {return retcode::SUCCESS;}
};

}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_LANGUAGE_PROTO_PARSER_H_
