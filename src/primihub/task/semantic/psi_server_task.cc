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

#include "private_set_intersection/cpp/psi_server.h"
#include "src/primihub/task/semantic/psi_server_task.h"
#include "src/primihub/util/util.h"

using psi_proto::Request;
using private_set_intersection::PsiServer;

namespace primihub::task {

void initRequest(const PsiRequest * request, Request & psi_request) {
    // use rvalue in future
    psi_request.set_reveal_intersection(request->reveal_intersection());
    std::int64_t num_client_elements =
        static_cast<std::int64_t>(request->encrypted_elements().size());
    for (std::int64_t i = 0; i < num_client_elements; i++) {
        psi_request.add_encrypted_elements(request->encrypted_elements()[i]);
    }
}

PSIServerTask::PSIServerTask(const std::string &node_id,
                             const ExecuteTaskRequest& request,
                             ExecuteTaskResponse *response,
                             std::shared_ptr<DatasetService> dataset_service)
: ServerTaskBase(&(request.params()), dataset_service), fpr_(0.0001) {

    //fpr_ = 0.0001;
    request_ = &(request.psi_request());
    response_ = response->mutable_psi_response();
}

int PSIServerTask::loadParams(Params & params) {
    auto param_map = params.param_map();

    try {
        data_index_ = param_map["serverIndex"].value_int32();
        dataset_path_ = param_map["serverData"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load psi server params: " << e.what();
        return -1;
    }
    return 0;
}

int PSIServerTask::loadDataset() {
    std::string match_word{"sqlite"};
    std::string driver_type;
    if (dataset_path_.size() > match_word.size()) {
        driver_type = dataset_path_.substr(0, match_word.size());
    } else {
        driver_type = dataset_path_;
    }
    VLOG(5) << "driver_type: " << driver_type;
    // current we supportes [csv, sqlite] as dataset
    int ret = 0;
    if (match_word == driver_type) {
        ret = loadDatasetFromSQLite(dataset_path_, data_index_, elements_);
    } else {
        ret = loadDatasetFromCSV(dataset_path_, data_index_, elements_);
    }
     // file reading error or file empty
    if (ret <= 0) {
        LOG(ERROR) << "Load dataset for psi server failed. dataset size: " << ret;
        return -1;
    }
    return 0;
}

int PSIServerTask::execute() {
    SCopedTimer timer;
    int ret = loadParams(params_);
    if (ret) {
        LOG(ERROR) << "Load parameters for psi server fialed.";
        return -1;
    }
    auto load_param_time_cost = timer.timeElapse();
    VLOG(5) << "load param time cost; " << load_param_time_cost;
    ret = loadDataset();
    if (ret) {
        return -1;
    }
    auto load_dataset_ts = timer.timeElapse();
    auto time_cost = load_dataset_ts - load_param_time_cost;
    VLOG(5) << "loadDataset time cost(ms): " << time_cost;
    Request psi_request;
    initRequest(request_, psi_request);
    auto init_req_ts = timer.timeElapse();
    auto init_req_time_cost = init_req_ts - load_dataset_ts;
    VLOG(5) << "init_req_time_cost(ms): " << init_req_time_cost;
    std::unique_ptr<PsiServer> server =
        PsiServer::CreateWithNewKey(psi_request.reveal_intersection()).value();
    auto create_new_key_ts = timer.timeElapse();
    auto create_new_key_time_cost = create_new_key_ts - init_req_ts;
    VLOG(5) << "create_new_key_time_cost(ms): " << create_new_key_time_cost;
    std::int64_t num_client_elements =
        static_cast<std::int64_t>(psi_request.encrypted_elements().size());
    psi_proto::ServerSetup server_setup =
        std::move(server->CreateSetupMessage(fpr_, num_client_elements, elements_)).value();
    auto create_server_setup_ts = timer.timeElapse();
    auto create_server_setup_tiem_cost = create_server_setup_ts - create_new_key_ts;
    VLOG(5) << "create_server_setup_tiem_cost(ms): " << create_server_setup_tiem_cost;
    psi_proto::Response server_response = server->ProcessRequest(psi_request).value();
    auto proceess_request_ts = timer.timeElapse();
    auto proceess_request_time_cost = proceess_request_ts - init_req_ts;
    VLOG(5) << "proceess_request_time_cost(ms): " << proceess_request_time_cost;
    auto _start = timer.timeElapse();
    // std::int64_t num_response_elements =
    //     static_cast<std::int64_t>(server_response.encrypted_elements().size());

    // for (std::int64_t i = 0; i < num_response_elements; i++) {
    //     response_->add_encrypted_elements(server_response.encrypted_elements()[i]);
    // }
    *(response_->mutable_encrypted_elements()) = std::move(*(server_response.mutable_encrypted_elements()));
    auto _end = timer.timeElapse();
    auto copy_time_cost = _end - _start;
    VLOG(5) << "encrypted_elements_time cost(ms): " << copy_time_cost;
    auto ptr_server_setup = response_->mutable_server_setup();
    ptr_server_setup->set_bits(server_setup.bits());
    if (server_setup.data_structure_case() ==
        psi_proto::ServerSetup::DataStructureCase::kGcs) {
        ptr_server_setup->mutable_gcs()->set_div(server_setup.gcs().div());
        ptr_server_setup->mutable_gcs()->set_hash_range(server_setup.gcs().hash_range());
    } else if (server_setup.data_structure_case() ==
               psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
        ptr_server_setup->mutable_bloom_filter()->
            set_num_hash_functions(server_setup.bloom_filter().num_hash_functions());
    }
    auto build_response_ts = timer.timeElapse();
    auto build_response_time_cost = build_response_ts - proceess_request_ts;
    VLOG(5) << "build_response_time_cost(ms): " << build_response_time_cost;
    return 0;

}

} //namespace primihub::task
