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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_

#include <string>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"

using primihub::service::DatasetWithParamTag;

using primihub::rpc::PushTaskRequest;

namespace primihub::task {
// Primihub language layer
class LanguageParser {
 public:
    LanguageParser(const PushTaskRequest &pushTaskRequest) {
        pushTaskRequest_.CopyFrom(pushTaskRequest);
    }
    ~LanguageParser() {}

    virtual retcode parseTask() = 0;
    virtual retcode parseDatasets() = 0;
    virtual retcode parseNodes()  = 0;

    PushTaskRequest getPushTaskRequest() { return pushTaskRequest_; }
    std::vector<DatasetWithParamTag> getDatasets() { return input_datasets_with_tag_; }

 protected:
    PushTaskRequest pushTaskRequest_;
    std::vector<DatasetWithParamTag> input_datasets_with_tag_;
};

} // namespace primihub::task


#endif // SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_
