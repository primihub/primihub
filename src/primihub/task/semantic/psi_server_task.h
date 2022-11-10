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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SERVER_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SERVER_TASK_H_

#include <map>
#include <memory>
#include <string>

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/private_server_base.h"
#include "src/primihub/util/log_wrapper.h"

using primihub::rpc::Params;
using primihub::rpc::PsiRequest;
using primihub::rpc::PsiResponse;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;

namespace primihub::task {

class PSIServerTask : public ServerTaskBase {
public:
    explicit PSIServerTask(const std::string &node_id,
                           const ExecuteTaskRequest& request,
                           ExecuteTaskResponse *response,
                           std::shared_ptr<DatasetService> dataset_service);
    ~PSIServerTask(){}

    int loadParams(Params & params) override;
    int loadDataset(void) override;
    int execute() override;

private:
    const double fpr_;
    int data_index_;
    std::string dataset_path_;
    std::vector <std::string> elements_;
    const PsiRequest * request_;
    PsiResponse * response_;
};

} // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SERVER_TASK_H_