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

#include "apsi/thread_pool_mgr.h"
#include "apsi/sender_db.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/zmq/sender_dispatcher.h"

#include "seal/context.h"
#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace std;
using namespace apsi;
using namespace apsi::sender;
using namespace apsi::oprf;
using namespace apsi::network;
using namespace seal;
using namespace seal::util;

namespace primihub::task {

shared_ptr<SenderDB> create_sender_db(
    const CSVReader::DBData &db_data,
    unique_ptr<PSIParams> psi_params,
    OPRFKey &oprf_key,
    size_t nonce_byte_count,
    bool compress)
{
    if (!psi_params) {
        LOG(ERROR) << "No Keyword pir parameters were given";
        return nullptr;
    }

    shared_ptr<SenderDB> sender_db;
    if (holds_alternative<CSVReader::LabeledData>(db_data)) {
        try {
            auto &labeled_db_data = get<CSVReader::LabeledData>(db_data);
            // Find the longest label and use that as label size
            size_t label_byte_count =
                max_element(labeled_db_data.begin(), labeled_db_data.end(), [](auto &a, auto &b) {
                    return a.second.size() < b.second.size();
                })->second.size();

            sender_db =
                make_shared<SenderDB>(*psi_params, label_byte_count, nonce_byte_count, compress);
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
        CSVReader reader(dataset_path_);
        tie(db_data, ignore) = reader.read();
    } catch (const exception &ex) {
        LOG(ERROR) << "Could not open or read file `" 
                   << dataset_path_ << "`: " << ex.what();
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
        query_params.query_powers.insert(power);
    }

    PSIParams::SEALParams seal_params;
    //const auto &coeff_modulus_bits = {49, 40, 20};
    size_t poly_modulus_degree = 4096;
    seal_params.set_poly_modulus_degree(poly_modulus_degree);
    seal_params.set_plain_modulus(40961);
    vector<int> coeff_modulus_bit_sizes = {49, 40, 20};
    seal_params.set_coeff_modulus(
        CoeffModulus::Create(poly_modulus_degree, coeff_modulus_bit_sizes));

    return make_unique<PSIParams>(
        PSIParams(item_params, table_params, query_params, seal_params)); 
}

int KeywordPIRServerTask::execute() {
    ThreadPoolMgr::SetThreadCount(1);
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

    atomic<bool> stop = false;
    ZMQSenderDispatcher dispatcher(sender_db, oprf_key);
    int port = 1212;
    bool done_exit = true;

    dispatcher.run(stop, port, done_exit);

    return 0;
}

}
