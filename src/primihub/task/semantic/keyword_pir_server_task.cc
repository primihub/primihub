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
    VLOG(5) << "begin to build reader for dataset: " << dataset_path_;
    auto reader_ptr = std::make_unique<CSVReader::DBData>(std::move(db_data));
    VLOG(5) << "begin to build reader for dataset end ";
    return reader_ptr;
}

std::unique_ptr<PSIParams> KeywordPIRServerTask::_SetPsiParams() {
    PSIParams::TableParams table_params;
    // range [1, 8]
    table_params.hash_func_count = 1;
    // > 0
    table_params.table_size = 409;
    // range > 0
    table_params.max_items_per_bin = 20;

    PSIParams::ItemParams item_params;
    // range [2, 32]
    item_params.felts_per_item = 5;

    PSIParams::QueryParams query_params;
    // ps_low_degree < max_items_per_bin
    query_params.ps_low_degree = 0;
    //  is verified to not contain 0, to contain 1,
    // and to not contain values larger than max_items_per_bin.
    // Any value larger than ps_low_degree is verified to be divisible by ps_low_degree + 1
    std::vector<uint32_t> query_powers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    query_params.query_powers.insert(1);
    for (const auto &power : query_powers) {
        query_params.query_powers.insert(power);
    }

    PSIParams::SEALParams seal_params;
    //const auto &coeff_modulus_bits = {49, 40, 20};
    //
    size_t poly_modulus_degree = 2048;
    seal_params.set_poly_modulus_degree(poly_modulus_degree);
    // is prime and congruent to 1 modulo 2 * poly_modulus_degree
    seal_params.set_plain_modulus(65537);
    vector<int> coeff_modulus_bit_sizes = {48};
    seal_params.set_coeff_modulus(
        CoeffModulus::Create(poly_modulus_degree, coeff_modulus_bit_sizes));
    LOG(ERROR) << "begin to create PSIParams";
    auto ptr = std::make_unique<PSIParams>(item_params, table_params, query_params, seal_params);

    LOG(ERROR) << "begin to create PSIParams end";
    return ptr;
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
    int port = 1212;
    // bool done_exit = true;
    VLOG(5) << "ZMQSenderDispatcher begin to run port: " << std::to_string(port);
    dispatcher.run(stop, port);
    if (stop.load(std::memory_order::memory_order_relaxed)) {
        VLOG(5) << "key word pir task execute finished";
    }
    return 0;
}

}  // namespace primihub::task