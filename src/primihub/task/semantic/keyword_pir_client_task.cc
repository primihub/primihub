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

retcode KeywordPIRClientTask::_LoadParams(Task &task) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    std::string party_name = task.party_name();
    const auto& param_map = task.params().param_map();
    try {
        auto client_data_it = param_map.find("clientData");
        if (client_data_it != param_map.end()) {
          auto& client_data = client_data_it->second;
          if (client_data.is_array()) {
            recv_query_data_direct = true;   // read query data from clientData key directly
            const auto& items = client_data.value_string_array().value_string_array();
            for (const auto& item : items) {
              recv_data_.push_back(item);
            }
          } else {
            dataset_path_ = client_data.value_string();
            dataset_id_ = client_data.value_string();
          }
        } else {
          // check client has dataset
          const auto& party_datasets = task.party_datasets();
          auto it = party_datasets.find(party_name);
          if (it == party_datasets.end()) {
            LOG(ERROR) << "no query data found for client, party_name: " << party_name;
            return retcode::FAIL;
          }
          const auto& datasets_map = it->second.data();
          auto iter = datasets_map.find(party_name);
          if (iter == datasets_map.end()) {
            LOG(ERROR) << "no query data found for client, party_name: " << party_name;
            return retcode::FAIL;
          }
          dataset_id_ = iter->second;
        }
        VLOG(7) << "dataset_id: " << dataset_id_;
        auto result_file_path_it = param_map.find("outputFullFilename");
        if (result_file_path_it != param_map.end()) {
            result_file_path_ = result_file_path_it->second.value_string();
            VLOG(5) << "result_file_path_: " << result_file_path_;
        } else  {
            LOG(ERROR) << "no keyword outputFullFilename match";
            return retcode::FAIL;
        }
        // get server dataset id
        do {
          const auto& party_datasets = task.party_datasets();
          auto it = party_datasets.find(PARTY_SERVER);
          if (it == party_datasets.end()) {
            LOG(WARNING) << "no dataset found for party_name: " << PARTY_SERVER;
            break;
          }
          const auto& datasets_map = it->second.data();
          auto iter = datasets_map.find(PARTY_SERVER);
          if (iter == datasets_map.end()) {
            LOG(WARNING) << "no dataset found for party_name: " << PARTY_SERVER;
            break;
          }
          std::string server_dataset_id = iter->second;
          auto& dataset_service = this->getDatasetService();
          auto driver = dataset_service->getDriver(server_dataset_id);
          if (driver == nullptr) {
            LOG(WARNING) << "no dataset access info found for id: " << server_dataset_id;
            break;
          }
          auto& access_info = driver->dataSetAccessInfo();
          if (access_info == nullptr) {
            LOG(WARNING) << "no dataset access info found for id: " << server_dataset_id;
            break;
          }
          auto& schema = access_info->Schema();
          for (const auto& field : schema) {
            server_dataset_schema_.push_back(std::get<0>(field));
          }
        } while (0);

    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return retcode::FAIL;
    }
    const auto& party_info = task.party_access_info();
    auto it = party_info.find(PARTY_SERVER);
    if (it == party_info.end()) {
      LOG(ERROR) << "client can not found access info to server";
      return retcode::FAIL;
    }
    auto& pb_node = it->second;
    pbNode2Node(pb_node, &peer_node_);
    VLOG(5) << "peer_node: " << peer_node_.to_string();
    return retcode::SUCCESS;
}

KeywordPIRClientTask::DatasetDBPair KeywordPIRClientTask::_LoadDataFromDataset() {
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
        return std::make_pair(nullptr, orig_items);
    }
    return {std::make_unique<apsi::util::CSVReader::DBData>(std::move(db_data)), std::move(orig_items)};
}

KeywordPIRClientTask::DatasetDBPair KeywordPIRClientTask::_LoadDataFromRecvData() {
  if (recv_data_.empty()) {
    LOG(ERROR) << "query data is empty";
    return std::make_pair(nullptr, std::vector<std::string>());
  }
  // build db_data;
  // std::unqiue_ptr<apsi::util::CSVReader::DBData>
  apsi::util::CSVReader::DBData db_data = apsi::util::CSVReader::UnlabeledData();
  for(const auto& item_str : recv_data_) {
    apsi::Item db_item = item_str;
    std::get<apsi::util::CSVReader::UnlabeledData>(db_data).push_back(std::move(db_item));
  }
  return {std::make_unique<apsi::util::CSVReader::DBData>(std::move(db_data)), recv_data_};
  // return std::make_pair(std::move(db_data), std::move(orig_items));
}

KeywordPIRClientTask::DatasetDBPair KeywordPIRClientTask::_LoadDataset(void) {
    if (!recv_query_data_direct) {
        return _LoadDataFromDataset();
    } else {
        return _LoadDataFromRecvData();
    }
}

retcode KeywordPIRClientTask::saveResult(
        const std::vector<std::string>& orig_items,
        const std::vector<Item>& items,
        const std::vector<MatchRecord>& intersection) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    if (orig_items.size() != items.size()) {
        LOG(ERROR) << "Keyword PIR orig_items must have the same size as items, detail: "
            << "orig_items size: " << orig_items.size() << " items size: " << items.size();
        return retcode::FAIL;
    }

    std::vector<std::vector<std::string>> result_data;
    result_data.resize(2);
    for (auto& item : result_data) {
      item.reserve(orig_items.size());
    }
    auto& key = result_data[0];
    auto& result_value = result_data[1];
    for (size_t i = 0; i < orig_items.size(); i++) {
      if (!intersection[i].found) {
        VLOG(0) << "no match result found for query: [" << orig_items[i] << "]";
        continue;
      }
      if (intersection[i].label) {
        std::string label_info = intersection[i].label.to_string();
        std::vector<std::string> labels;
        std::string sep = DATA_RECORD_SEP;
        str_split(label_info, &labels, sep);
        for (const auto& lable_ : labels) {
          key.push_back(orig_items[i]);
          result_value.push_back(lable_);
        }
      } else {
        LOG(WARNING) << "no value found for query key: " << orig_items[i];
      }
    }
    VLOG(0) << "save query result to : " << result_file_path_;

    std::vector<std::shared_ptr<arrow::Field>> schema_vector;
    std::vector<std::string> tmp_colums{"key", "value"};
    for (const auto& col_name : tmp_colums) {
      schema_vector.push_back(arrow::field(col_name, arrow::int64()));
    }
    std::vector<std::shared_ptr<arrow::Array>> arrow_array;
    for (auto& item : result_data) {
      arrow::StringBuilder builder;
      builder.AppendValues(item);
      std::shared_ptr<arrow::Array> array;
      builder.Finish(&array);
      arrow_array.push_back(std::move(array));
    }

    auto schema = std::make_shared<arrow::Schema>(schema_vector);
    // std::shared_ptr<arrow::Table>
    auto table = arrow::Table::Make(schema, arrow_array);
    auto driver = DataDirverFactory::getDriver("CSV", "test address");
    auto csv_driver = std::dynamic_pointer_cast<CSVDriver>(driver);
    auto rtcode = csv_driver->Write(server_dataset_schema_, table, result_file_path_);
    if (rtcode != retcode::SUCCESS) {
      LOG(ERROR) << "save PIR data to file " << result_file_path_ << " failed.";
      return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

retcode KeywordPIRClientTask::requestPSIParams() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    RequestType type = RequestType::PsiParam;
    std::string request{reinterpret_cast<char*>(&type), sizeof(type)};
    VLOG(5) << "send_data length: " << request.length();
    std::string response_str;
    auto& link_ctx = this->getTaskContext().getLinkContext();
    CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
    auto channel = link_ctx->getChannel(peer_node_);
    auto ret = channel->sendRecv(this->key, request, &response_str);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send requestPSIParams to peer: [" << peer_node_.to_string()
            << "] failed";
        return ret;
    }
    if (VLOG_IS_ON(5)) {
        std::string tmp_str;
        for (const auto& chr : response_str) {
            tmp_str.append(std::to_string(static_cast<int>(chr))).append(" ");
        }
        VLOG(5) << "recv_data size: " << response_str.size() << " "
                << "data content: " << tmp_str;
    }

    // create psi params
    // static std::pair<PSIParams, std::size_t> Load(std::istream &in);
    std::istringstream stream_in(response_str);
    auto [parse_data, ret_size] = PSIParams::Load(stream_in);
    psi_params_ = std::make_unique<PSIParams>(parse_data);
    VLOG(5) << "parsed psi param, size: " << ret_size << " "
            << "content: " << psi_params_->to_string();
    return retcode::SUCCESS;
}

static std::string to_hexstring(const Item &item) {
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
    CHECK_TASK_STOPPED(retcode::FAIL);
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
    auto& link_ctx = this->getTaskContext().getLinkContext();
    CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
    auto channel = link_ctx->getChannel(peer_node_);

    // auto ret = channel->sendRecv(this->key, oprf_request_sv, &oprf_response);
    auto ret = this->send(this->key, peer_node_, oprf_request_sv);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "requestOprf to peer: [" << peer_node_.to_string()
            << "] failed";
        return ret;
    }
    ret = this->recv(this->key, &oprf_response);
    if (ret != retcode::SUCCESS || oprf_response.empty()) {
        LOG(ERROR) << "receive oprf_response from peer: [" << peer_node_.to_string()
            << "] failed";
        return retcode::FAIL;
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
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "Pir client load task params failed.";
        return -1;
    }
    VLOG(5) << "begin to request psi params";
    ret = requestPSIParams();
    CHECK_RETCODE_WITH_RETVALUE(ret, -1);

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
    ret = requestOprf(items_vec, &oprf_items, &label_keys);
    CHECK_RETCODE_WITH_RETVALUE(ret, -1);

    CHECK_TASK_STOPPED(-1);
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
        ret = this->send(this->key, peer_node_, query_data_str);
        CHECK_RETCODE_WITH_RETVALUE(ret, -1);

        // receive package count
        uint32_t package_count = 0;
        ret = this->recv("package_count", reinterpret_cast<char*>(&package_count), sizeof(package_count));
        CHECK_RETCODE_WITH_RETVALUE(ret, -1);

        VLOG(5) << "received package count: " << package_count;
        std::vector<apsi::ResultPart> result_packages;
        for (size_t i = 0; i < package_count; i++) {
            std::string recv_data;
            ret = this->recv(this->key, &recv_data);
            CHECK_RETCODE_WITH_RETVALUE(ret, -1);
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
    ret = this->saveResult(orig_items, items, query_result);
    CHECK_RETCODE_WITH_RETVALUE(ret, -1);
    return 0;
}

} // namespace primihub::task

