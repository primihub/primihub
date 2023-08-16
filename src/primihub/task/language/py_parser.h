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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_PY_PARSER_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_PY_PARSER_H_

#include <string>
#include <vector>

#include "src/primihub/task/language/parser.h"
#include "src/primihub/task/common.h"

namespace primihub::task {

class PyParser : public LanguageParser {
 public:
    PyParser(const rpc::PushTaskRequest &pushTaskRequest)
        : LanguageParser(pushTaskRequest) {
    }
    ~PyParser();
    retcode parseTask() override;
    retcode parseDatasets();
    retcode parseNodes() override;

    std::map<std::string, NodeContext> getNodeContextMap() const {
        return nodes_context_map_;
    }

 private:
    std::string py_code_;
    std::string procotol_;
    std::vector<std::string> roles_;
    std::vector<std::string> func_params_;
    std::map<std::string, NodeContext> nodes_context_map_;
};

}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_LANGUAGE_PY_PARSER_H_
