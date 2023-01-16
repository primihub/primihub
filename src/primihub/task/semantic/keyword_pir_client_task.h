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
#include "src/primihub/common/defines.h"
#include "apsi/receiver.h"

using apsi::Item;
using apsi::receiver::MatchRecord;
using apsi::util::CSVReader;
using namespace apsi::receiver;

namespace primihub::task {
class KeywordPIRClientTask : public TaskBase {
 public:
   enum class RequestType : uint8_t {
      PsiParam = 0,
      Oprf,
      Query,
   };
   explicit KeywordPIRClientTask(const TaskParam *task_param,
                                 std::shared_ptr<DatasetService> dataset_service);

   ~KeywordPIRClientTask() = default;
   int execute() override;
   int saveResult(const std::vector<std::string>& orig_items,
                  const std::vector<apsi::Item>& items,
                  const std::vector<apsi::receiver::MatchRecord>& intersection);
   int waitForServerPortInfo();

 protected:
   /**
    * Performs a parameter request from sender
   */
   retcode requestPSIParams();
   /**
    * Performs an OPRF request on a vector of items and returns a
      vector of OPRF hashed items of the same size as the input vector.
   */
   retcode requestOprf(const std::vector<Item>& items,
         std::vector<apsi::HashedItem>*, std::vector<apsi::LabelKey>*);
   /**
    * Performs a labeled PSI query. The query is a vector of
      items, and the result is a same-size vector of MatchRecord objects. If an item is in the
      intersection, the corresponding MatchRecord indicates it in the `found` field, and the
      `label` field may contain the corresponding label if a sender's data included it.
   */
   retcode requestQuery();

 private:
    int _LoadParams(Task &task);
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataset();
    // load dataset according by url
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataFromDataset();
    // load data from request directly
    std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>> _LoadDataFromRecvData();
    int _GetIntsection();

 private:
    std::string dataset_path_;
    std::string dataset_id_;
    std::string result_file_path_;
    std::string server_address_;
    bool recv_query_data_direct{false};
    uint32_t server_data_port{2222};
    primihub::Node peer_node_;
    std::string key{"default"};
    std::unique_ptr<apsi::PSIParams> psi_params_{nullptr};
    std::unique_ptr<apsi::receiver::Receiver> receiver_{nullptr};
};
}  // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_CLIENT_TASK_H_
