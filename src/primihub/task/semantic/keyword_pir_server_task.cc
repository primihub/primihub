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

#include "src/primihub/task/semantic/keyword_pir_server_task.h"
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/util/util.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/sender_db.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/zmq/sender_dispatcher.h"

#include "seal/context.h"
#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace apsi;
using namespace apsi::sender;
using namespace apsi::oprf;
using namespace apsi::network;

using namespace seal;
using namespace seal::util;

namespace primihub::task {

std::shared_ptr<SenderDB>
KeywordPIRServerTask::create_sender_db(
        const CSVReader::DBData &db_data,
        std::unique_ptr<PSIParams> psi_params,
        OPRFKey &oprf_key,
        size_t nonce_byte_count,
        bool compress) {
    SCopedTimer timer;
    if (!psi_params) {
        LOG(ERROR) << "No Keyword pir parameters were given";
        return nullptr;
    }

    std::shared_ptr<SenderDB> sender_db;
    if (holds_alternative<CSVReader::LabeledData>(db_data)) {
        VLOG(5) << "CSVReader::LabeledData";
        try {
            auto &labeled_db_data = std::get<CSVReader::LabeledData>(db_data);
            // Find the longest label and use that as label size
            size_t label_byte_count =
                 std::max_element(labeled_db_data.begin(), labeled_db_data.end(),
                    [](auto &a, auto &b) {
                        return a.second.size() < b.second.size();
                    })->second.size();
            VLOG(5) << "label_byte_count: " << label_byte_count << " nonce_byte_count: " << nonce_byte_count;
            sender_db =
                std::make_shared<SenderDB>(*psi_params, label_byte_count, nonce_byte_count, compress);
            sender_db->set_data(labeled_db_data);
        } catch (const exception &ex) {
            LOG(ERROR) << "Failed to create keyword pir SenderDB: " << ex.what();
            return nullptr;
        }

    } else if (holds_alternative<CSVReader::UnlabeledData>(db_data)) {
        LOG(ERROR) << "Loaded keyword pir database is without label";
        return nullptr;
    } else {
        LOG(ERROR) << "Loaded keyword pir database is in an invalid state";
        return nullptr;
    }

    oprf_key = sender_db->strip();
    auto time_cost = timer.timeElapse();
    VLOG(5) << "create_sender_db success, time cost(ms): " << time_cost;
    return sender_db;
}

KeywordPIRServerTask::KeywordPIRServerTask(const std::string &node_id,
                                           const std::string &job_id,
                                           const std::string &task_id,
                                           const TaskParam *task_param,
                                           std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int KeywordPIRServerTask::_LoadParams(Task &task) {
    const auto& node_map = task.node_map();
    for (const auto& node_info : node_map) {
        auto& _node_id = node_info.first;
        if (_node_id == this->node_id_) {
            continue;
        }
        auto& node = node_info.second;
        this->client_address = node.ip() + ":" + std::to_string(node.port());
        VLOG(5) << "client_address: " << this->client_address;
    }

    const auto& param_map = task.params().param_map();
    try {
        auto it = param_map.find("serverData");
        if (it != param_map.end()) {
            dataset_path_ = it->second.value_string();
        } else {
            LOG(ERROR) << "Failed to load params serverData, no match key find";
            return -1;
        }
        // dataset_path_ = param_map["serverData"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    return 0;
}

std::unique_ptr<CSVReader::DBData> KeywordPIRServerTask::_LoadDataset(void) {
    CSVReader::DBData db_data;

    try {
        VLOG(5) << "begin to read data, dataset path: " << dataset_path_;
        CSVReader reader(dataset_path_);
        tie(db_data, ignore) = reader.read();
    } catch (const exception &ex) {
        LOG(ERROR) << "Could not open or read file `"
                   << dataset_path_ << "`: " << ex.what();
        return nullptr;
    }
    auto reader_ptr = std::make_unique<CSVReader::DBData>(std::move(db_data));
    return reader_ptr;
}

std::unique_ptr<PSIParams> KeywordPIRServerTask::_SetPsiParams() {
    std::string params_json;
    std::string pir_server_config_path{"config/pir_server_config.json"};
    VLOG(5) << "pir_server_config_path: " << pir_server_config_path;
    try {
        // throw_if_file_invalid(cmd.params_file());
        std::fstream input_file(pir_server_config_path, std::ios_base::in);
        if (!input_file.is_open()) {
            LOG(ERROR) << "open " << pir_server_config_path << " encountes error";
            return nullptr;
        }
        std::string line;
        while (getline(input_file, line)) {
            params_json.append(line);
            params_json.append("\n");
        }
        input_file.close();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Error trying to read input file " << pir_server_config_path << ": " << ex.what();
        return nullptr;
    }

    std::unique_ptr<PSIParams> params{nullptr};
    try {
        params = std::make_unique<PSIParams>(PSIParams::Load(params_json));
    } catch (const std::exception &ex) {
        LOG(ERROR) << "APSI threw an exception creating PSIParams: " << ex.what();
        return nullptr;
    }
    return params;
}

int KeywordPIRServerTask::execute() {
    int ret = _LoadParams(task_param_);
    if (ret) {
        LOG(ERROR) << "Pir client load task params failed.";
        return ret;
    }
    ThreadPoolMgr::SetThreadCount(1);
    OPRFKey oprf_key;
    std::unique_ptr<PSIParams> params = _SetPsiParams();
    std::unique_ptr<CSVReader::DBData> db_data = _LoadDataset();
    std::shared_ptr<SenderDB> sender_db
        = create_sender_db(*db_data, move(params), oprf_key, 16, false);
    uint32_t max_bin_bundles_per_bundle_idx = 0;
    for (uint32_t bundle_idx = 0; bundle_idx < sender_db->get_params().bundle_idx_count();
         bundle_idx++) {
        max_bin_bundles_per_bundle_idx =
            std::max(max_bin_bundles_per_bundle_idx,
                static_cast<uint32_t>(sender_db->get_bin_bundle_count(bundle_idx)));
    }

    atomic<bool> stop = false;
    VLOG(5) << "begin to create ZMQSenderDispatcher";
    ZMQSenderDispatcher dispatcher(sender_db, oprf_key);
    getAvailablePort(&data_port);
    broadcastPortInfo();
    // bool done_exit = true;
    VLOG(5) << "ZMQSenderDispatcher begin to run port: " << std::to_string(data_port);
    dispatcher.run(stop, data_port);
    if (stop.load(std::memory_order::memory_order_relaxed)) {
        VLOG(5) << "key word pir task execute finished";
    }
    return 0;
}
int KeywordPIRServerTask::broadcastPortInfo() {
    rpc::TaskRequest request;
    request.set_task_id(this->task_id_);
    request.set_job_id(this->job_id_);
    rpc::TaskResponse task_response;
    auto param_map = request.mutable_params()->mutable_param_map();
    // dataset size
    rpc::ParamValue pv_data_port;
    pv_data_port.set_var_type(rpc::VarType::INT32);
    pv_data_port.set_value_int32(this->data_port);
    pv_data_port.set_is_array(false);
    (*param_map)["data_port"] = pv_data_port;
    grpc::ClientContext context;
    auto channel = grpc::InsecureChannelCredentials();
    auto stub = rpc::VMNode::NewStub(grpc::CreateChannel(this->client_address, channel));
    std::unique_ptr<grpc::ClientWriter<primihub::rpc::TaskRequest>> writer(stub->Send(&context, &task_response));
    writer->Write(request);
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (status.ok()) {
        LOG(INFO) << "send param to client success";
    } else {
        LOG(ERROR) << "send param to client failed, "
                   << status.error_code() << " : " << status.error_message();
    }
    return 0;
}
}  // namespace primihub::task
