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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_CLIENT_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_CLIENT_TASK_H_

#include "apsi/item.h"
#include "apsi/match_record.h"

#include "src/primihub/task/semantic/task.h"

namespace primihub::task {

class KeywordPIRClientTask : public TaskBase {
public:
    explicit KeywordPIRClientTask(const std::string &node_id,
                                  const std::string &job_id,
                                  const std::string &task_id,
                                  const TaskParam *task_param,
                                  std::shared_ptr<DatasetService> dataset_service);

    ~KeywordPIRClientTask() {};
    int execute() override;
    int saveResult(
        const vector<string> &orig_items,
        const vector<Item> &items,
        const vector<MatchRecord> &intersection);
private:
    int _LoadParams(Task &task);
    pair<unique_ptr<CSVReader::DBData>, vector<string>> _LoadDataset(void);
    int _GetIntsection();

    const std::string node_id_;
    const std::string job_id_;
    const std::string task_id_;

    std::string dataset_path_;
    std::string result_file_path_;
    std::string server_address_;
}
}  // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_CLIENT_TASK_H_
