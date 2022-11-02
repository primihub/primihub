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

#ifndef __APPLE__
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/Defines.h"
#include "libPSI/PSI/Kkrt/KkrtPsiReceiver.h"
#endif

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <set>

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"


#ifndef __APPLE__
using namespace osuCrypto;
#endif
using primihub::rpc::Task;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
#ifndef __APPLE__
using osuCrypto::KkrtPsiReceiver;
#endif



namespace primihub::task {

class PSIKkrtTask : public TaskBase {
public:
    explicit PSIKkrtTask(const std::string &node_id,
                             const std::string &job_id,
                             const std::string &task_id,
                             const TaskParam *task_param,
                             std::shared_ptr<DatasetService> dataset_service);

    ~PSIKkrtTask() {}
    int execute() override;
    int saveResult(void);
    int send_result_to_server();
private:
    int _LoadParams(Task &task);
    int _LoadDataset(void);
    int _LoadDatasetFromCSV(std::string &filename, int data_col,
                            std::vector <std::string> &col_array);
#ifndef __APPLE__
    void _kkrtRecv(Channel& chl);
    void _kkrtSend(Channel& chl);
    int _GetIntsection(KkrtPsiReceiver &receiver);
#endif

    const std::string node_id_;
    const std::string job_id_;
    const std::string task_id_;
    int data_index_;
    int psi_type_;
    int role_tag_;
    std::string dataset_path_;
    std::string result_file_path_;
    std::vector <std::string> elements_;
    std::vector <std::string> result_;

    std::string host_address_;
    bool sync_result_to_server{false};
    std::string server_result_path;

};
}
#endif //SRC_PRIMIHUB_TASK_SEMANTIC_PSI_KKRT_TASK_H_
