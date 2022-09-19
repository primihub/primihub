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
#include "src/primihub/task/semantic/keyword_pir_client_task.h"

#include <thread>
#include <chrono>

#include "apsi/network/zmq/zmq_channel.h"
#include "apsi/receiver.h"
#include "apsi/util/common_utils.h"

using namespace apsi;
using namespace apsi::network;
using namespace apsi::receiver;

namespace primihub::task {

KeywordPIRClientTask::KeywordPIRClientTask(
        const std::string &node_id,
        const std::string &job_id,
        const std::string &task_id,
        const TaskParam *task_param,
        std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), node_id_(node_id), job_id_(job_id), task_id_(task_id) {}

int KeywordPIRClientTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();
    try {
        dataset_path_ = param_map["clientData"].value_string();
        VLOG(5) << "dataset_path_: " << dataset_path_;
        result_file_path_ = param_map["outputFullFilename"].value_string();
        VLOG(5) << "result_file_path_: " << result_file_path_;
        server_address_ = param_map["serverAddress"].value_string();
        VLOG(5) << "server_address_: " << server_address_;
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    return 0;
}

std::pair<unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>>
KeywordPIRClientTask::_LoadDataset(void) {
    apsi::util::CSVReader::DBData db_data;
    std::vector<std::string> orig_items;
    try {
        apsi::util::CSVReader reader(dataset_path_);
        std::tie(db_data, orig_items) = reader.read();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Could not open or read file `"
                   << dataset_path_ << "`: "
                   << ex.what();
        return { nullptr, orig_items };
    }

    return {std::make_unique<apsi::util::CSVReader::DBData>(std::move(db_data)), std::move(orig_items)};
}

int KeywordPIRClientTask::_GetIntsection() {
    return 0;
}

int KeywordPIRClientTask::saveResult(
        const std::vector<std::string>& orig_items,
        const std::vector<Item>& items,
        const std::vector<MatchRecord>& intersection) {
    if (orig_items.size() != items.size()) {
        LOG(ERROR) << "Keyword PIR orig_items must have the same size as items, detail: "
            << "orig_items size: " << orig_items.size() << " items size: " << items.size();
        return -1;
    }

    std::stringstream csv_output;
    for (size_t i = 0; i < orig_items.size(); i++) {
        if (!intersection[i].found) {
            VLOG(5) << "no match result found for query: [" << orig_items[i] << "]";
            continue;
        }
        csv_output << orig_items[i];    // original query
        if (intersection[i].label) {
            csv_output << "," << intersection[i].label.to_string();     // matched result
        }
        csv_output << endl;
    }
    VLOG(5) << "result_file_path_: " << result_file_path_;
    if (!result_file_path_.empty()) {
        std::ofstream ofs(result_file_path_);
        ofs << csv_output.str();
    }

    return 0;
}

int KeywordPIRClientTask::execute() {
    auto ret = _LoadParams(task_param_);
    if (ret) {
        LOG(ERROR) << "Pir client load task params failed.";
        return ret;
    }
    ZMQReceiverChannel channel;
    // extract server ip from server_address_

    server_address_ = "tcp://127.0.0.1:1212";  // TODO
    VLOG(5) << "begin to connect to server: " << server_address_;
    channel.connect(server_address_);
    VLOG(5) << "connect to server: " << server_address_ << " end";
    if (!channel.is_connected()) {
        LOG(ERROR) << "Failed to connect to keyword PIR server: " << server_address_;
        return -1;
    }
    VLOG(5) << "connect to server: " << server_address_ << " success, begin to create PSIParams";
    std::unique_ptr<PSIParams> params{nullptr};
    try {
        // params = std::make_unique<PSIParams>(Receiver::RequestParams(channel));
        VLOG(5) << "begin to create PSIParams";
        auto psi_params = Receiver::RequestParams(channel);
        VLOG(5) << "get reqeust param success";
        VLOG(5) << "PSI parameters set to: " << psi_params.to_string();
        VLOG(5) << "Derived parameters: "
            << "item_bit_count_per_felt: " << psi_params.item_bit_count_per_felt()
            << "; item_bit_count: " << psi_params.item_bit_count()
            << "; bins_per_bundle: " << psi_params.bins_per_bundle()
            << "; bundle_idx_count: " << psi_params.bundle_idx_count();
        params = std::make_unique<PSIParams>(psi_params);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Failed to receive keyword PIR valid parameters: " << ex.what();
        return -1;
    }

    ThreadPoolMgr::SetThreadCount(8);
    LOG(INFO) << "Keyword PIR setting thread count to " << ThreadPoolMgr::GetThreadCount();

    Receiver receiver(*params);
    auto [query_data, orig_items] = _LoadDataset();
    if (!query_data || !holds_alternative<CSVReader::UnlabeledData>(*query_data)) {
        LOG(ERROR) << "Failed to read keyword PIR query file: terminating";
        return -1;
    }
    VLOG(5) << "end of _LoadDataset, " << orig_items.size();
    for (const auto& item : orig_items) {
        VLOG(5) << "item: " << item;
    }
    auto& items = std::get<CSVReader::UnlabeledData>(*query_data);

    std::vector<Item> items_vec(items.begin(), items.end());
    std::vector<HashedItem> oprf_items;
    std::vector<LabelKey> label_keys;
    VLOG(5) << "begin to Receiver::RequestOPRF";
    try {
        VLOG(5) << "Sending OPRF request for " << items_vec.size() << " items";
        std::tie(oprf_items, label_keys) = Receiver::RequestOPRF(items_vec, channel);
        VLOG(5) << "Received OPRF request for " << items_vec.size() << " items"
            << " oprf_items: " << oprf_items.size() << " label_keys: " << label_keys.size();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Keyword PIR OPRF request failed: " << ex.what();
        return -1;
    }
    VLOG(5) << "Receiver::RequestOPRF end, begin to receiver.request_query";

    std::vector<MatchRecord> query_result;
    try {
        query_result = receiver.request_query(oprf_items, label_keys, channel);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Failed sending keyword PIR query: " << ex.what();
        return -1;
    }
    VLOG(5) << "receiver.request_query end";

    this->saveResult(orig_items, items, query_result);
    return 0;
}
} // namespace primihub::task
