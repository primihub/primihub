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
#include "src/primihub/task/semantic/pir_client_task.h"

#include <string>

#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/util.h"


using arrow::Table;
using arrow::StringArray;
using arrow::Int64Builder;
using primihub::rpc::VarType;

namespace primihub::task {

int validateDirection(std::string file_path) {
    int pos = file_path.find_last_of('/');
    std::string path;
    if (pos > 0) {
        path = file_path.substr(0, pos);
        if (access(path.c_str(), 0) == -1) {
            std::string cmd = "mkdir -p " + path;
            int ret = system(cmd.c_str());
            if (ret)
                return -1;
        }
    }
    return 0;
}

PIRClientTask::PIRClientTask(const std::string &node_id,
                             const std::string &job_id,
                             const std::string &task_id,
                             const TaskParam *task_param,
                             std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int PIRClientTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();

    try {
        result_file_path_ = param_map["outputFullFilename"].value_string();
        server_address_ = param_map["serverAddress"].value_string();
        server_dataset_ = param_map[server_address_].value_string();
        db_size_ = stoi(param_map["databaseSize"].value_string());  // temperarily read db size direly from frontend
        std::vector<std::string> tmp_indices;
        str_split(param_map["queryIndeies"].value_string(), &tmp_indices, ',');
        for (std::string &index : tmp_indices) {
            int idx = stoi(index);
            indices_.push_back(idx);
        }
        std::vector<std::string> server_info;
        str_split(server_address_, &server_info, ':');
        if (server_info.size() == 3) {
            server_address_ = server_info[0] + ":" + server_info[1];
            if (std::stoi(server_info[2])) {
                this->set_use_tls(true);
            }
        }
    } catch (std::exception &e) {
        LOG_ERROR() << "Failed to load params: " << e.what();
        return -1;
    }

    return 0;
}


int PIRClientTask::_SetUpDB(size_t __dbsize, size_t dimensions, size_t elem_size,
                             uint32_t plain_mod_bit_size, uint32_t bits_per_coeff,
                             bool use_ciphertext_multiplication = false) {
    // db_size_ = dbsize;
    encryption_params_ = pir::GenerateEncryptionParams(POLY_MODULUS_DEGREE,
                                                       plain_mod_bit_size);
    pir_params_ = *(pir::CreatePIRParameters(db_size_, elem_size, dimensions, encryption_params_,
                                        use_ciphertext_multiplication, bits_per_coeff));
    client_ = *(PIRClient::Create(pir_params_));
    if (client_ == nullptr) {
        LOG_ERROR() << "Failed to create pir client.";
        return -1;
    }

    return 0;
}

int PIRClientTask::_ProcessResponse(const ExecuteTaskResponse &taskResponse) {
    pir::Response response;
    size_t num_reply =
        static_cast<size_t>(taskResponse.pir_response().reply().size());
    for (size_t i = 0; i < num_reply; i++) {
        pir::Ciphertexts* ptr_reply = response.add_reply();
        size_t num_ct =
            static_cast<std::int64_t>(taskResponse.pir_response().reply()[i].ct().size());
        for (size_t j = 0; j < num_ct; j++) {
            ptr_reply->add_ct(taskResponse.pir_response().reply()[i].ct()[j]);
        }
    }
    auto result = client_->ProcessResponse(indices_, response);
    if (result.ok()) {
        for (size_t i = 0; i < std::move(result).value().size(); i++) {
            result_.push_back(std::move(result).value()[i]);
        }
    } else {
        LOG_ERROR() << "Failed to process pir server response: "
                   << result.status();
        return -1;
    }
    return 0;
}

int PIRClientTask::saveResult() {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::StringBuilder builder(pool);

    for (std::int64_t i = 0; i < result_.size(); i++) {
        builder.Append(result_[i]);
    }

    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);

    std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
        arrow::field("reslut", arrow::utf8())};
    auto schema = std::make_shared<arrow::Schema>(schema_vector);
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});

    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", "pir result");
    std::shared_ptr<CSVDriver> csv_driver =
        std::dynamic_pointer_cast<CSVDriver>(driver);

    if (validateDirection(result_file_path_)) {
        LOG_ERROR() << "can't access file path: "
                   << result_file_path_;
        return -1;
    }

    int ret = csv_driver->write(table, result_file_path_);

    if (ret != 0) {
        LOG_ERROR() << "Save PIR result to file " << result_file_path_ << " failed.";
        return -1;
    }

    LOG_INFO() << "Save PIR result to " << result_file_path_ << ".";
    return 0;
}

uint32_t compute_plain_mod_bit_size(size_t dbsize, size_t elem_size) {
    uint32_t plain_mod_bit_size = PLAIN_MOD_BIT_SIZE_UPBOUND;
    while (true) {
        plain_mod_bit_size--;
        uint64_t elem_per_plaintext = POLY_MODULUS_DEGREE \
                                    * (plain_mod_bit_size - 1) / 8 / elem_size;
        uint64_t num_plaintext = dbsize / elem_per_plaintext + 1;
        if (num_plaintext <=
                (uint64_t)1 << (NOISE_BUDGET_BASE - 2 * plain_mod_bit_size))
        {
            break;
        }
    }
    return plain_mod_bit_size;
}

int PIRClientTask::execute() {
    int ret = _LoadParams(task_param_);
    if (ret) {
        LOG_ERROR() << "Pir client load task params failed.";
        return ret;
    }

    size_t dimensions = 1;
    size_t elem_size = ELEM_SIZE;
    uint32_t plain_mod_bit_size = compute_plain_mod_bit_size(db_size_, elem_size);
    bool use_ciphertext_multiplication = true;
    uint32_t poly_modulus_degree = POLY_MODULUS_DEGREE;
    uint32_t bits_per_coeff = 0;

    ret = _SetUpDB(0, dimensions, elem_size,                // temperarily read db size direly from frontend
                       plain_mod_bit_size, bits_per_coeff,
                       use_ciphertext_multiplication);

    if (ret) {
        LOG_ERROR() << "Failed to initialize pir client.";
        return -1;
    }
    //pir::Request request_proto = std::move(client_->CreateRequest(indices_)).value();
    pir::Request request_proto;
    auto request_or = client_->CreateRequest(indices_);
    if (request_or.ok()) {
        request_proto = std::move(request_or).value();
    } else {
        LOG_ERROR() << "Pir create request failed: "
                   << request_or.status();
        return -1;
    }

    grpc::ClientContext client_context;
    auto channel = buildChannel(server_address_, node_id(), use_tls());
    // grpc::ChannelArguments channel_args;
    // channel_args.SetMaxReceiveMessageSize(128*1024*1024);
    // std::shared_ptr<grpc::Channel> channel =
    //     grpc::CreateCustomChannel(server_address_, grpc::InsecureChannelCredentials(), channel_args);
    std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(channel);
    using stream_t = std::shared_ptr<grpc::ClientReaderWriter<ExecuteTaskRequest, ExecuteTaskResponse>>;
    stream_t client_stream(stub->ExecuteTask(&client_context));

    size_t limited_size = 1 << 21;
    size_t query_num = request_proto.query().size();
    const auto& querys = request_proto.query();
    size_t sended_index{0};
    std::vector<ExecuteTaskRequest> send_requests;
    do {
        ExecuteTaskRequest taskRequest;
        PirRequest * ptr_request = taskRequest.mutable_pir_request();
        ptr_request->set_galois_keys(request_proto.galois_keys());
        ptr_request->set_relin_keys(request_proto.relin_keys());
        size_t pack_size = 0;
        for (size_t i = sended_index; i < query_num; i++) {
            // calculate length of query
            size_t query_size = 0;
            const auto& query = querys[i];
            for (const auto& ct : query.ct()) {
                query_size += ct.size();
            }
            if (pack_size + query_size > limited_size) {
                break;
            }
            auto query_ptr = ptr_request->add_query();
            for (const auto& ct : query.ct()) {
                query_ptr->add_ct(ct);
            }
            sended_index++;
        }
        auto *ptr_params = taskRequest.mutable_params()->mutable_param_map();
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        pv.set_value_string(server_dataset_);
        (*ptr_params)["serverData"] = pv;
        send_requests.push_back(std::move(taskRequest));
        if (sended_index >= query_num) {
            break;
        }
    } while (true);
    // send request to server
    for (const auto& request : send_requests) {
        client_stream->Write(request);
    }
    client_stream->WritesDone();
    ExecuteTaskResponse taskResponse;
    ExecuteTaskResponse recv_response;
    auto pir_response = taskResponse.mutable_pir_response();
    bool is_initialized{false};
    while (client_stream->Read(&recv_response)) {
        const auto& recv_pir_response = recv_response.pir_response();
        if (!is_initialized) {
            pir_response->set_ret_code(recv_pir_response.ret_code());
            is_initialized = true;
        }
        for (const auto& reply : recv_pir_response.reply()) {
            auto reply_ptr = pir_response->add_reply();
            for (const auto& ct : reply.ct()) {
                reply_ptr->add_ct(ct);
            }
        }
    }
    Status status = client_stream->Finish();
    if (status.ok()) {
        if (taskResponse.psi_response().ret_code()) {
            LOG_ERROR() << "Node pir server process request error.";
            return -1;
        }
        int ret = _ProcessResponse(taskResponse);
        if (ret) {
            LOG_ERROR() << "Node pir client process response failed.";
            return -1;
        }

        ret = saveResult();
        if (ret) {
            LOG_ERROR() << "Pir save result failed.";
            return -1;
        }
    } else {
        LOG_ERROR() << "Pir server return error: "
                   << status.error_code() << " " << status.error_message().c_str();
        return -1;
    }
    return 0;
}

}
