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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PIR_CLIENT_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PIR_CLIENT_TASK_H_

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include "pir/cpp/client.h"
#include "pir/cpp/database.h"
#include "pir/cpp/utils.h"
#include "pir/cpp/string_encoder.h"

#include <map>
#include <memory>
#include <string>
#include <set>

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"

using pir::PIRParameters;
using pir::EncryptionParameters;
using pir::PIRClient;

// using grpc::ClientContext;
using grpc::Status;
using grpc::Channel;

using primihub::rpc::Ciphertexts;
using primihub::rpc::Task;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;
using primihub::rpc::PirRequest;
using primihub::rpc::PirResponse;
using primihub::rpc::VMNode;

namespace primihub::task {

constexpr uint32_t POLY_MODULUS_DEGREE = 4096;
constexpr uint32_t ELEM_SIZE = 1024;
constexpr uint32_t PLAIN_MOD_BIT_SIZE_UPBOUND = 29;
constexpr uint32_t NOISE_BUDGET_BASE = 57;

class PIRClientTask : public TaskBase {
public:
    explicit PIRClientTask(const std::string &node_id,
                           const std::string &job_id,
                           const std::string &task_id,
                           const TaskParam *task_param,
                           std::shared_ptr<DatasetService> dataset_service);
    ~PIRClientTask() {};

    int execute() override;
    int saveResult(void);
private:
    int _LoadParams(Task &task);
    int _SetUpDB(size_t dbsize, size_t dimensions, size_t elem_size,
		 uint32_t plain_mod_bit_size, uint32_t bits_per_coeff,
                 bool use_ciphertext_multiplication);
    int _ProcessResponse(const ExecuteTaskResponse &taskResponse);

    const std::string node_id_;
    const std::string job_id_;
    const std::string task_id_;
    std::string server_address_;
    std::string result_file_path_;
    std::vector<size_t> indices_;
    std::vector<std::string> result_;   

    std::string server_dataset_;

    size_t db_size_;
    std::shared_ptr<PIRParameters> pir_params_;
    EncryptionParameters encryption_params_;
    std::unique_ptr<PIRClient> client_;
};

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PIR_CLIENT_TASK_H_
