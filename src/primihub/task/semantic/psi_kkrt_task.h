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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_KKRT_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_KKRT_TASK_H_

#if defined(__linux__) && defined(__x86_64__)
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/Defines.h"
#include "libPSI/PSI/Kkrt/KkrtPsiReceiver.h"
#endif

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <set>

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/task/semantic/psi_task.h"

#if defined(__linux__) && defined(__x86_64__)
using namespace osuCrypto;
using osuCrypto::KkrtPsiReceiver;
#endif

namespace rpc = primihub::rpc;


namespace primihub::task {

class PSIKkrtTask : public TaskBase, public PsiCommonOperator {
public:
    explicit PSIKkrtTask(const TaskParam *task_param,
                        std::shared_ptr<DatasetService> dataset_service);

    ~PSIKkrtTask() {}
    int execute() override;
    int saveResult(void);
    retcode broadcastResultToServer();
    int recvIntersectionData();
private:
    retcode exchangeDataPort();
    int _LoadParams(Task &task);
    int _LoadDataset(void);
#if defined(__linux__) && defined(__x86_64__)
    void _kkrtRecv(Channel& chl);
    void _kkrtSend(Channel& chl);
    int _GetIntsection(KkrtPsiReceiver &receiver);
#endif

    std::vector<int> data_index_;
    int psi_type_;
    int role_tag_;
    std::string dataset_path_;
    std::string result_file_path_;
    std::vector <std::string> elements_;
    std::vector <std::string> result_;
    std::string host_address_;
    bool sync_result_to_server{false};
    std::string server_result_path;
    uint32_t data_port{0};
    uint32_t peer_data_port{1212};
    primihub::Node peer_node;
    std::string key{"default"};
};
}
#endif //SRC_PRIMIHUB_TASK_SEMANTIC_PSI_KKRT_TASK_H_
