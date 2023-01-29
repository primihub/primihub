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
#include <sstream>

#include "src/primihub/util/util.h"

#include "apsi/item.h"
#include "apsi/util/common_utils.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/protos/worker.pb.h"
#include "seal/util/common.h"
using namespace apsi;
using namespace apsi::network;


namespace primihub::task {

KeywordPIRClientTask::KeywordPIRClientTask(
    const TaskParam *task_param, std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {
}

int KeywordPIRClientTask::_LoadParams(Task &task) {
    const auto& param_map = task.params().param_map();
    try {
        auto client_data_it = param_map.find("clientData");
        if (client_data_it != param_map.end()) {
            auto& client_data = client_data_it->second;
            if (client_data.is_array()) {
                recv_query_data_direct = true;   // read query data from clientData key directly
            }
            dataset_path_ = client_data.value_string();
            dataset_id_ = client_data.value_string();
            VLOG(5) << "dataset_path_: " << dataset_path_;
        } else {
            LOG(ERROR) << "no keyword: clientData match found";
            return -1;
        }
        auto result_file_path_it = param_map.find("outputFullFilename");
        if (result_file_path_it != param_map.end()) {
            result_file_path_ = result_file_path_it->second.value_string();
            VLOG(5) << "result_file_path_: " << result_file_path_;
        } else  {
            LOG(ERROR) << "no keyword: outputFullFilename match found";
            return -1;
        }
        auto server_address_it = param_map.find("serverAddress");
        if (server_address_it != param_map.end()) {
            server_address_ = server_address_it->second.value_string();
            VLOG(5) << "server_address_: " << server_address_;
        } else {
            LOG(ERROR) << "no keyword: serverAddress match found";
            return -1;
        }
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    const auto& node_map = task.node_map();
    for (const auto& [_node_id, pb_node] : node_map) {
        if (_node_id == this->node_id()) {
            continue;
        }
        peer_node_ = Node(pb_node.ip(), pb_node.port(), pb_node.use_tls(), pb_node.role());
        VLOG(5) << "peer_node: " << peer_node_.to_string();
    }
    return 0;
}

std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>>
KeywordPIRClientTask::_LoadDataFromDataset() {
    apsi::util::CSVReader::DBData db_data;
    std::vector<std::string> orig_items;
    auto driver = this->getDatasetService()->getDriver(this->dataset_id_);
    if (driver == nullptr) {
        LOG(ERROR) << "get driver for dataset: " << this->dataset_id_ << " failed";
        return std::make_pair(nullptr, std::vector<std::string>());
    }
    auto access_info = dynamic_cast<CSVAccessInfo*>(driver->dataSetAccessInfo().get());
    if (access_info == nullptr) {
        LOG(ERROR) << "get data accessinfo for dataset: " << this->dataset_id_ << " failed";
        return std::make_pair(nullptr, std::vector<std::string>());
    }
    dataset_path_ = access_info->file_path_;
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

std::pair<std::unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>>
KeywordPIRClientTask::_LoadDataFromRecvData() {
    std::vector<std::string> orig_items;
    str_split(this->dataset_path_, &orig_items, ';');
    // build db_data;
    // std::unqiue_ptr<apsi::util::CSVReader::DBData>
    apsi::util::CSVReader::DBData db_data = apsi::util::CSVReader::UnlabeledData();
    for(const auto& item_str : orig_items) {
        apsi::Item db_item = item_str;
        std::get<apsi::util::CSVReader::UnlabeledData>(db_data).push_back(std::move(db_item));
    }
    return {std::make_unique<apsi::util::CSVReader::DBData>(std::move(db_data)), std::move(orig_items)};
    // return std::make_pair(std::move(db_data), std::move(orig_items));
}

std::pair<unique_ptr<apsi::util::CSVReader::DBData>, std::vector<std::string>>
KeywordPIRClientTask::_LoadDataset(void) {
    if (!recv_query_data_direct) {
        return _LoadDataFromDataset();
    } else {
        return _LoadDataFromRecvData();
    }
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
        if (intersection[i].label) {
            std::string label_info = intersection[i].label.to_string();
            std::vector<std::string> labels;
            std::string sep = "#####";
            str_split(label_info, &labels, sep);
            for (const auto& lable_ : labels) {
                csv_output << orig_items[i] << "," << lable_ << endl;
            }
        } else {
            csv_output << orig_items[i] << endl;
        }

    }
    VLOG(5) << "result_file_path_: " << result_file_path_;
    if (ValidateDir(result_file_path_)) {
        LOG(ERROR) << "can't access file path: " << result_file_path_;
        return -1;
    }
    if (!result_file_path_.empty()) {
        std::ofstream ofs(result_file_path_);
        ofs << csv_output.str();
    }

    return 0;
}
retcode KeywordPIRClientTask::requestPSIParams() {
    RequestType type = RequestType::PsiParam;
    std::string request{reinterpret_cast<char*>(&type), sizeof(type)};
    VLOG(5) << "send_data length: " << request.length();
    std::string response_str;
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node_);
    auto ret = channel->sendRecv(this->key, request, &response_str);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send requestPSIParams to peer: [" << peer_node_.to_string()
            << "] failed";
        return ret;
    }
    std::string tmp_str;
    for (const auto& chr : response_str) {
        tmp_str.append(std::to_string(static_cast<int>(chr))).append(" ");
    }
    VLOG(5) << "recv_data size: " << response_str.size() << " "
            << "data content: " << tmp_str;
    // create psi params
    // static std::pair<PSIParams, std::size_t> Load(std::istream &in);
    std::istringstream stream_in(response_str);
    auto [parse_data, ret_size] = PSIParams::Load(stream_in);
    psi_params_ = std::make_unique<PSIParams>(parse_data);
    VLOG(5) << "parsed psi param, size: " << ret_size << " "
            << "content: " << psi_params_->to_string();
    return retcode::SUCCESS;
}

static std::string to_hexstring(const Item &item)
{
    std::stringstream ss;
    ss << std::hex;

    auto item_string = item.to_string();
    for(int i(0); i < 16; ++i)
        ss << std::setw(2) << std::setfill('0') << (int)item_string[i];

    return ss.str();
}

retcode KeywordPIRClientTask::requestOprf(const std::vector<Item>& items,
        std::vector<apsi::HashedItem>* res_items_ptr,
        std::vector<apsi::LabelKey>* res_label_keys_ptr) {
    RequestType type = RequestType::Oprf;
    std::string oprf_response;
    auto oprf_receiver = this->receiver_->CreateOPRFReceiver(items);
    auto& res_items = *res_items_ptr;
    auto& res_label_keys = *res_label_keys_ptr;
    res_items.resize(oprf_receiver.item_count());
    res_label_keys.resize(oprf_receiver.item_count());
    auto oprf_request = oprf_receiver.query_data();
    VLOG(5) << "oprf_request data length: " << oprf_request.size();
    std::string_view oprf_request_sv{
        reinterpret_cast<char*>(const_cast<unsigned char*>(oprf_request.data())), oprf_request.size()};
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node_);

    auto ret = channel->sendRecv(this->key, oprf_request_sv, &oprf_response);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "requestOprf to peer: [" << peer_node_.to_string()
            << "] failed";
        return ret;
    }
    VLOG(5) << "received oprf response length: " << oprf_response.length() << " ";
    oprf_receiver.process_responses(oprf_response, res_items, res_label_keys);
    return retcode::SUCCESS;
}

retcode KeywordPIRClientTask::requestQuery() {
    RequestType type = RequestType::Query;
    std::string send_data{reinterpret_cast<char*>(&type), sizeof(type)};
    VLOG(5) << "send_data length: " << send_data.length();
    return retcode::SUCCESS;
}

int KeywordPIRClientTask::execute() {
    auto ret = _LoadParams(task_param_);
    if (ret) {
        LOG(ERROR) << "Pir client load task params failed.";
        return ret;
    }
    VLOG(5) << "begin to request psi params";
    auto ret_code = requestPSIParams();
    if (ret_code != retcode::SUCCESS) {
      return -1;
    }
    auto [query_data, orig_items] = _LoadDataset();
    if (!query_data || !holds_alternative<CSVReader::UnlabeledData>(*query_data)) {
        LOG(ERROR) << "Failed to read keyword PIR query file: terminating";
        return -1;
    }

    auto& items = std::get<CSVReader::UnlabeledData>(*query_data);
    std::vector<Item> items_vec(items.begin(), items.end());
    std::vector<HashedItem> oprf_items;
    std::vector<LabelKey> label_keys;
    VLOG(5) << "begin to Receiver::RequestOPRF";
    ret_code = requestOprf(items_vec, &oprf_items, &label_keys);
    if (ret_code != retcode::SUCCESS) {
      return -1;
    }

    if (VLOG_IS_ON(5)) {
        for (int i = 0; i < items_vec.size(); i++) {
            VLOG(5) << "item[" << i << "]'s PRF value: " << to_hexstring(oprf_items[i]);
        }
    }

    VLOG(5) << "Receiver::RequestOPRF end, begin to receiver.request_query";

    // request query
    this->receiver_ = std::make_unique<Receiver>(*psi_params_);
    std::vector<MatchRecord> query_result;
    try {
        auto query = this->receiver_->create_query(oprf_items);
        // chl.send(move(query.first));
        auto request_query_data = std::move(query.first);
        std::ostringstream string_ss;
        request_query_data->save(string_ss);
        std::string query_data_str = string_ss.str();
        auto itt = move(query.second);
        VLOG(5) << "query_data_str size: " << query_data_str.size();
        this->send(this->key, peer_node_, query_data_str);
        // receive package count
        uint32_t package_count = 0;
        this->recv("package_count", reinterpret_cast<char*>(&package_count), sizeof(package_count));
        VLOG(5) << "received package count: " << package_count;
        std::vector<apsi::ResultPart> result_packages;
        for (size_t i = 0; i < package_count; i++) {
            std::string recv_data;
            this->recv(this->key, &recv_data);
            VLOG(5) << "client received data length: " << recv_data.size();
            std::istringstream stream_in(recv_data);
            apsi::ResultPart result_part = std::make_unique<apsi::network::ResultPackage>();
            auto seal_context = this->receiver_->get_seal_context();
            result_part->load(stream_in, seal_context);
            result_packages.push_back(std::move(result_part));
        }
        query_result = this->receiver_->process_result(label_keys, itt, result_packages);
        VLOG(5) << "query_resultquery_resultquery_resultquery_result: " << query_result.size();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Failed sending keyword PIR query: " << ex.what();
        return -1;
    }
    VLOG(5) << "receiver.request_query end";
    this->saveResult(orig_items, items, query_result);
    return 0;
}

int KeywordPIRClientTask::waitForServerPortInfo() {
    std::string recv_data_port_info_str;
    this->recv(this->key, &recv_data_port_info_str);
    rpc::Params recv_data_port_info;
    recv_data_port_info.ParseFromString(recv_data_port_info_str);
    const auto& recv_param_map = recv_data_port_info.param_map();
    auto it = recv_param_map.find("data_port");
    if (it != recv_param_map.end()) {
        server_data_port = it->second.value_int32();
        VLOG(5) << "server_data_port: " << server_data_port;
    }
    return 0;
}

} // namespace primihub::task

