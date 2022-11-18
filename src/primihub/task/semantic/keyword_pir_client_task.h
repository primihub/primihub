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

#include <vector>

#include "apsi/item.h"
#include "apsi/match_record.h"
#include "apsi/util/csv_reader.h"
#include "src/primihub/task/semantic/task.h"

using apsi::Item;
using apsi::receiver::MatchRecord;
using apsi::util::CSVReader;

namespace primihub::task {

class KeywordPIRClientTask : public TaskBase {
 public:
    explicit KeywordPIRClientTask(const std::string &node_id,
                                  const std::string &job_id,
                                  const std::string &task_id,
                                  const TaskParam *task_param,
                                  std::shared_ptr<DatasetService> dataset_service);

    ~KeywordPIRClientTask() = default;
    int execute() override;
    int saveResult(const std::vector<std::string>& orig_items,
                   const std::vector<apsi::Item>& items,
                   const std::vector<apsi::receiver::MatchRecord>& intersection);

 private:
    int _LoadParams(Task &task);
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataset();
    // load dataset according by url
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataFromDataset();
    // load data from request directly
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataFromRecvData();
    int _GetIntsection();

 private:
    std::string node_id_;
    std::string job_id_;
    std::string task_id_;
    std::string dataset_path_;
    std::string result_file_path_;
    std::string server_address_;
    bool recv_query_data_direct{false};
};
}  // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_CLIENT_TASK_H_
