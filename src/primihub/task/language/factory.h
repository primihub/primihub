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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_

#include <glog/logging.h>
#include <memory>

#include "src/primihub/task/language/parser.h"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/task/language/py_parser.h"

using primihub::rpc::Language;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::TaskType;
using primihub::service::DatasetService;

namespace primihub::task {

using primihub::rpc::Language;

class LanguageParserFactory {
  public:
    static std::shared_ptr<LanguageParser>
    Create(const PushTaskRequest &pushTaskRequest) {
        auto language = pushTaskRequest.task().language();
        if (language == Language::PROTO) {
            auto parser = std::make_shared<ProtoParser>(pushTaskRequest);
            return std::dynamic_pointer_cast<LanguageParser>(parser);
        } else if (language == Language::PYTHON) {
            auto parser = std::make_shared<PyParser>(pushTaskRequest);
            return std::dynamic_pointer_cast<LanguageParser>(parser);
        } else if (language == Language::CPP) {
            // TODO
        } else {
            LOG(ERROR) << "Unsupported language: " << language;
            return nullptr;
        }
        return nullptr;
    }
}; // class LanguageParserFactory
} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_LANGUAGE_FACTORY_H_
