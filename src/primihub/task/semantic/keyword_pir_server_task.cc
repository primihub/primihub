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

namespace primihub::task {

KeywordPIRServerTask::KeywordPIRServerTask(const std::string &node_id,
                                           const std::string &job_id,
                                           const std::string &task_id,
                                           const TaskParam *task_param,
                                           std::shared_ptr<DatasetService> dataset_service) 
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int KeywordPIRServerTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();
    try {
        dataset_path_ = param_map["serverData"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    return 0;
}

unique_ptr<CSVReader::DBData> KeywordPIRServerTask::_LoadDataset(void) {
    CSVReader::DBData db_data;

    try {
        CSVReader reader(db_file);
        tie(db_data, ignore) = reader.read();
    } catch (const exception &ex) {
        LOG(ERROR) << "Could not open or read file `" 
                   << db_file << "`: " << ex.what();
        return nullptr;
    }
    return make_unique<CSVReader::DBData>(move(db_data));
}

unique_ptr<PSIParams> KeywordPIRServerTask::_SetPsiParams() {
    PSIParams::TableParams table_params;
    table_params.hash_func_count = 3;
    table_params.table_size = 512;
    table_params.max_items_per_bin = 9;

    PSIParams::ItemParams item_params;
    item_params.felts_per_item = 8;

    PSIParams::QueryParams query_params;
    query_params.ps_low_degree = 0;
    const auto &query_powers = {3, 4, 5, 8, 14, 20, 26,
                                32,38, 41, 42, 43, 45, 46};
    query_params.query_powers.insert(1);
    for (const auto &power : query_powers) {
        query_params.query_powers.insert(json_value_ui32(power));
    }

    PSIParams::SEALParams seal_params;
    //const auto &coeff_modulus_bits = {49, 40, 20};
    size_t poly_modulus_degree = 4096;
    seal_params.set_poly_modulus_degree(poly_modulus_degree);
    seal_params.set_plain_modulus(40961);
    vector<int> coeff_modulus_bit_sizes = {49, 40, 20};
    seal_params.set_coeff_modulus(
        CoeffModulus::Create(poly_modulus_degree, coeff_modulus_bit_sizes));

    return PSIParams(item_params, table_params, query_params, seal_params); 
}

int KeywordPIRServerTask::execute() {
    ThreadPoolMgr::SetThreadCount(cmd.threads());
    shared_ptr<SenderDB> sender_db;
    OPRFKey oprf_key;
    unique_ptr<PSIParams> params = _SetPsiParams();
    unique_ptr<CSVReader::DBData> db_data = _LoadDataset();
    shared_ptr<SenderDB> sender_db = create_sender_db(
            *db_data, move(params), oprf_key, 16, false);

    uint32_t max_bin_bundles_per_bundle_idx = 0;
    for (uint32_t bundle_idx = 0; bundle_idx < sender_db->get_params().bundle_idx_count();
         bundle_idx++) {
        max_bin_bundles_per_bundle_idx =
            max(max_bin_bundles_per_bundle_idx,
                static_cast<uint32_t>(sender_db->get_bin_bundle_count(bundle_idx)));
    }

    ZMQSenderDispatcher dispatcher(sender_db, oprf_key);
    ZMQSenderChannel chl;
    int port = 1212;
    stringstream ss;
    ss << "tcp://*:" << port;
    chl.bind(ss.str());

    auto seal_context = sender_db->get_seal_context();
    //bool logged_waiting = false;
    while (true) {
        unique_ptr<ZMQSenderOperation> sop;
        if (!(sop = chl.receive_network_operation(seal_context))) {
            this_thread::sleep_for(50ms);
            continue;
        }

        switch (sop->sop->type()) {
            
        }
    }
}
}