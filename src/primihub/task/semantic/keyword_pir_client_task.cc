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

namespace primihub::task {

KeywordPIRClientTask::KeywordPIRClientTask(const std::string &node_id,
                                           const std::string &job_id,
                                           const std::string &task_id,
                                           const TaskParam *task_param,
                                           std::shared_ptr<DatasetService> dataset_service) 
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int KeywordPIRClientTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();
    try {
        dataset_path_ = param_map["clientData"].value_string();
        result_file_path_ = param_map["outputFullFilename"].value_string();
        server_address_ = param_map["serverAddress"].value_string();
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    return 0;
}

pair<unique_ptr<CSVReader::DBData>, vector<string>> KeywordPIRClientTask::_LoadDataset(void) {
    CSVReader::DBData db_data;
    vector<string> orig_items;
    try {
        CSVReader reader(dataset_path_);
        tie(db_data, orig_items) = reader.read();
    } catch (const exception &ex) {
        LOG(ERROR) << "Could not open or read file `"
                   << dataset_path_ << "`: "
                   << ex.what();
        return { nullptr, orig_items };
    }

    return { make_unique<CSVReader::DBData>(move(db_data)), move(orig_items) };
}

int KeywordPIRClientTask::_GetIntsection() {
    return 0;
}

int KeywordPIRClientTask::saveResult(
        const vector<string> &orig_items,
        const vector<Item> &items,
        const vector<MatchRecord> &intersection) {
    if (orig_items.size() != items.size()) {
        LOG(ERROR) << "Keyword PIR orig_items must have same size as items";
        return -1;
    }

    stringstream csv_output;
    for (size_t i = 0; i < orig_items.size(); i++) {
        if (intersection[i].found) {
            csv_output << orig_items[i];
            if (intersection[i].label) {
                csv_output << "," << intersection[i].label.to_string();
            }
            csv_output << endl;
        }
    }

    if (!out_file.empty()) {
        ofstream ofs(out_file);
        ofs << csv_output.str();
    }

    return 0;
}

int KeywordPIRClientTask::execute() {
    ZMQReceiverChannel channel;
    channel.connect(server_address_);
    if (!channel.is_connected()) {
        LOG(ERROR) << "Failed to connect to keyword PIR server: " << conn_addr;
        return -1;
    }

    unique_ptr<PSIParams> params;
    try {
        params = make_unique<PSIParams>(Receiver::RequestParams(channel));
    } catch (const exception &ex) {
        LOG(ERROR) << "Failed to receive keyword PIR valid parameters: " << ex.what();
        return -1;
    }

    ThreadPoolMgr::SetThreadCount(1);
    LOG(INFO) << "Keyword PIR setting thread count to " << ThreadPoolMgr::GetThreadCount();

    Receiver receiver(*params);
    auto [query_data, orig_items] = _LoadDataset();
    if (!query_data || !holds_alternative<CSVReader::UnlabeledData>(*query_data)) {
        LOG(ERROR) << "Failed to read keyword PIR query file: terminating";
        return -1;
    }

    auto &items = get<CSVReader::UnlabeledData>(*query_data);
    vector<Item> items_vec(items.begin(), items.end());
    vector<HashedItem> oprf_items;
    vector<LabelKey> label_keys;
    try {
        tie(oprf_items, label_keys) = Receiver::RequestOPRF(items_vec, channel);
    } catch (const exception &ex) {
        LOG(ERROR) << "Keyword PIR OPRF request failed: " << ex.what();
        return -1;
    }

    vector<MatchRecord> query_result;
    try {
        query_result = receiver.request_query(oprf_items, label_keys, channel);
    } catch (const exception &ex) {
        LOG(ERROR) << "Failed sending keyword PIR query: " << ex.what();
        return -1;
    }

    return 0;
}
} // namespace primihub::task