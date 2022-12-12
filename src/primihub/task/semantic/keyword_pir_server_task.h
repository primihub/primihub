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

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/task/semantic/task.h"

#include "apsi/util/csv_reader.h"
#include "apsi/util/common_utils.h"
#include "apsi/sender.h"
#include "apsi/oprf/oprf_sender.h"

namespace primihub::task {

class KeywordPIRServerTask : public TaskBase {
 public:
    enum class RequestType : uint8_t {
      PsiParam = 0,
      Opfr,
      Query,
   };
    explicit KeywordPIRServerTask(const TaskParam* task_param,
                                  std::shared_ptr<DatasetService> dataset_service);
    ~KeywordPIRServerTask() = default;
    int execute() override;
    retcode broadcastPortInfo();

 protected:
    /**
     * process a Get Parameters request to the Sender.
    */
    retcode processPSIParams();
    /**
      process an OPRF query request to the Sender.
    */
    retcode processOprf();
    /**
      process a Query request to the Sender.
    */
    retcode processQuery();

 private:
    int _LoadParams(Task &task);
    std::unique_ptr<apsi::util::CSVReader::DBData> _LoadDataset(void);
    std::unique_ptr<apsi::PSIParams> _SetPsiParams();
    std::shared_ptr<apsi::sender::SenderDB> create_sender_db(
        const apsi::util::CSVReader::DBData &db_data,
        std::unique_ptr<apsi::PSIParams> psi_params,
        apsi::oprf::OPRFKey &oprf_key,
        size_t nonce_byte_count,
        bool compress);

 private:
    std::string dataset_path_;
    uint32_t data_port{2222};
    std::string client_address;
    primihub::Node client_node_;
    std::string key{"default"};
    std::string psi_params_str_;
    std::unique_ptr<apsi::oprf::OPRFKey> oprf_key_{nullptr};
};
} // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_KEYWORD_PIR_SERVER_TASK_H_
