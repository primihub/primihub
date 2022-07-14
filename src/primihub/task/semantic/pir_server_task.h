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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SERVER_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SERVER_TASK_H_

#include <map>
#include <memory>
#include <string>
#include <stdlib.h>

#include "pir/cpp/server.h"
#include "pir/cpp/database.h"
#include "pir/cpp/utils.h"
#include "pir/cpp/string_encoder.h"

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/private_server_base.h"

using std::shared_ptr;

using primihub::rpc::Params;
using primihub::rpc::Ciphertexts;
using primihub::rpc::PirRequest;
using primihub::rpc::PirResponse;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;

namespace primihub::task {

constexpr uint32_t POLY_MODULUS_DEGREE_SVR = 4096;
constexpr uint32_t ELEM_SIZE_SVR = 1024;
constexpr uint32_t PLAIN_MOD_BIT_SIZE_UPBOUND_SVR = 29;
constexpr uint32_t NOISE_BUDGET_BASE_SVR = 57;


class PIRServerTask : public ServerTaskBase {
public:
    explicit PIRServerTask(const std::string &node_id,
                           const ExecuteTaskRequest& request,
                           ExecuteTaskResponse *response,
                           std::shared_ptr<DatasetService> dataset_service);
    ~PIRServerTask(){}

    int loadParams(Params & params) override;
    int loadDataset(void) override;
    int execute() override;

private:
    int _SetUpDB(size_t dbsize, size_t dimensions,
                 size_t elem_size, uint32_t poly_modulus_degree,
                 uint32_t plain_mod_bit_size, uint32_t bits_per_coeff,
                 bool use_ciphertext_multiplication);
                 
    //int data_col_;
    std::string dataset_path_;
    size_t db_size_;
    shared_ptr<pir::PIRParameters> pir_params_;
    pir::EncryptionParameters encryption_params_;
    std::vector<std::string> elements_;
    shared_ptr<pir::PIRDatabase> pir_db_;

    const PirRequest * request_;
    PirResponse * response_;
};

} // namespace primihub::task

#endif SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SERVER_TASK_H_
