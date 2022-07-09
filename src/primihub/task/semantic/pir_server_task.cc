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


#include "src/primihub/task/semantic/pir_server_task.h"

namespace primihub::task {

void initRequest(const PirRequest * request, pir::Request & pir_request) {
    pir_request.set_galois_keys(request->galois_keys());
    pir_request.set_relin_keys(request->relin_keys());
    const size_t num_query = static_cast<size_t>(request->query().size());
    for (size_t i = 0; i < num_query; i++) {
        pir::Ciphertexts* ptr_query = pir_request.add_query();
        const size_t num_ct =
            static_cast<size_t>(request->query()[i].ct().size());
        for (size_t j = 0; j < num_ct; j++) {
            ptr_query->add_ct(request->query()[i].ct()[j]);
        }
    }
}

PIRServerTask::PIRServerTask(const std::string &node_id,
                             const ExecuteTaskRequest& request,
                             ExecuteTaskResponse *response,
                             std::shared_ptr<DatasetService> dataset_service)
: ServerTaskBase(&(request.params()), dataset_service) {
    request_ = &(request.pir_request());
    response_ = response->mutable_pir_response();
}

int PIRServerTask::loadParams(Params & params) {
    auto param_map = params.param_map();
    try {
        dataset_path_ = param_map["serverData"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load pir server params: " << e.what();
        return -1;
    }
    return 0;
}

int PIRServerTask::loadDataset() {
    int ret = loadDatasetFromCSV(dataset_path_, 0, elements_, db_size_);
    if (ret) {
        LOG(ERROR) << "Load dataset for psi client failed.";
        return -1;
    }
    return 0;
}

int PIRServerTask::_SetUpDB(size_t dbsize, size_t dimensions,
                            size_t elem_size, uint32_t poly_modulus_degree,
                            uint32_t plain_mod_bit_size, uint32_t bits_per_coeff,
                            bool use_ciphertext_multiplication) {
    encryption_params_ = 
        pir::GenerateEncryptionParams(poly_modulus_degree, plain_mod_bit_size);
    pir_params_ = *(pir::CreatePIRParameters(dbsize, elem_size, dimensions, encryption_params_,
                                        use_ciphertext_multiplication, bits_per_coeff));
    db_size_ = dbsize;
    int ret = loadDataset();
    if (ret)
        return -1;

    if (elements_.size() > dbsize) {
        LOG(ERROR) << "Dataset size is not equal dbsize:" << elements_.size();
        for (int i = 0; i < elements_.size(); i++) {
            LOG(INFO) << "elem: " << elements_[i];
        }
        return -1;
    } else if (elements_.size() < dbsize) {
        uint32_t seed = 42;
        auto prng =
	    seal::UniformRandomGeneratorFactory::DefaultFactory()->create({seed});
        for (int64_t i = elements_.size(); i < dbsize; i++) {
            int rand_num = rand() % 80;

            std::string rand_str(rand_num, 0);
            prng->generate(rand_str.size(), reinterpret_cast<seal::SEAL_BYTE*>(rand_str.data()));
	    elements_.push_back(std::to_string(i) + rand_str);
        }
    }

    std::vector<std::string> string_db;
    string_db.resize(dbsize, std::string(elem_size, 0));
    for (size_t i = 0; i < dbsize; ++i) {
        for (int j = 0; j < elements_[i].length(); j++) {
            string_db[i][j] = elements_[i][j];
        }
    }
    auto db_status = pir::PIRDatabase::Create(string_db, pir_params_);
    if (!db_status.ok()) {
        LOG(ERROR) << db_status.status();
        return -1;
    } else {
        pir_db_ = std::move(db_status).value();
    }

    return 0;
}

int PIRServerTask::execute() {
    int ret = loadParams(params_);
    if (ret) {
        LOG(ERROR) << "Load parameters for pir server fialed.";
        return -1;
    }

    size_t db_size = 400;
    size_t dimensions = 1;
    size_t elem_size = ELEM_SIZE_SVR;
    uint32_t plain_mod_bit_size = 28;
    bool use_ciphertext_multiplication = false;
    uint32_t poly_modulus_degree = POLY_MODULUS_DEGREE_SVR;
    uint32_t bits_per_coeff = 0;

    pir::Request pir_request;
    initRequest(request_, pir_request);
    ret = _SetUpDB(db_size, dimensions, elem_size, poly_modulus_degree,
                       plain_mod_bit_size, bits_per_coeff,
                       use_ciphertext_multiplication);
    if (ret) {
        LOG(ERROR) << "Create pir db failed.";
        return -1;
    }

    std::unique_ptr<pir::PIRServer> server = 
        *(pir::PIRServer::Create(pir_db_, pir_params_));

    if (server == nullptr) {
        LOG(ERROR) << "Failed to create pir server";
        return -1;
    }

    auto  result_status = server->ProcessRequest(pir_request);
    if (!result_status.ok()) {
        LOG(ERROR) << "Process pir request failed:"
                   << result_status.status();
        return -1;
    }

    auto result_raw = std::move(result_status).value();
    const size_t num_reply = static_cast<size_t>(result_raw.reply().size());
    for (size_t i = 0; i < num_reply; i++) {
	Ciphertexts* ptr_reply = response_->add_reply();
        const size_t num_ct = static_cast<size_t>(result_raw.reply()[i].ct().size());
        for (size_t j = 0; j < num_ct; j++) {
            ptr_reply->add_ct(result_raw.reply()[i].ct()[j]);
        }
    }

    return 0;
}

} // namespace primihub::task
