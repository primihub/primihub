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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_CLIENT_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_CLIENT_TASK_H_

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <map>
#include <memory>
#include <string>
#include <set>

#include "private_set_intersection/cpp/psi_client.h"

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"

using grpc::ClientContext;
using grpc::Status;
using grpc::Channel;

using primihub::rpc::Task;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;
using primihub::rpc::PsiRequest;
using primihub::rpc::PsiResponse;
using primihub::rpc::VMNode;

using private_set_intersection::PsiClient;

namespace primihub::task {

class PSIClientTask : public TaskBase {
public:
    explicit PSIClientTask(const std::string &node_id,
                           const std::string &job_id,
                           const std::string &task_id,
                           const TaskParam *task_param,
                           std::shared_ptr<DatasetService> dataset_service);
    ~PSIClientTask() {};

    int execute() override;
    int saveResult(void);
private:
    int _LoadParams(Task &task);
    int _LoadDataset(void);
    int _LoadDatasetFromCSV(std::string &filename, int data_col,
                            std::vector <std::string> &col_array);
    int _GetIntsection(const std::unique_ptr<PsiClient> &client, 
                       ExecuteTaskResponse & taskResponse);

    

    const std::string node_id_;
    const std::string job_id_;
    const std::string task_id_;
    int data_index_;
    int psi_type_;
    std::string dataset_path_;
    std::string result_file_path_;
    bool reveal_intersection_;
    std::vector <std::string> elements_;
    std::vector <std::int64_t> result_;

    std::string server_address_;
    std::string server_dataset_;
    ParamValue server_index_;
    
};

} // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_CLIENT_TASK_H_