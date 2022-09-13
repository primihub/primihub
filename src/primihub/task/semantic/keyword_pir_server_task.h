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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_SERVER_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_SERVER_TASK_H_

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"

#include "apsi/util/csv_reader.h"
#include "apsi/util/common_utils.h"

using std::string;
using std::unique_ptr;
using apsi::PSIParams;
using apsi::util::CSVReader;

namespace primihub::task {

class KeywordPIRServerTask : public TaskBase {
public:
    explicit KeywordPIRServerTask(const std::string &node_id,
                                  const std::string &job_id,
                                  const std::string &task_id,
                                  const TaskParam *task_param,
                                  std::shared_ptr<DatasetService> dataset_service);

    ~KeywordPIRServerTask(){};
    int execute() override;
private:
    int _LoadParams(Task &task);
    unique_ptr<CSVReader::DBData> _LoadDataset(void);
    unique_ptr<PSIParams> _SetPsiParams();

    const std::string node_id_;
    const std::string job_id_;
    const std::string task_id_;

    std::string dataset_path_;

};
} // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_SERVER_TASK_H_
