
#include <set>

#include "src/primihub/task/semantic/psi_ecdh_task.h"
#include "private_set_intersection/cpp/psi_client.h"
#include "private_set_intersection/cpp/psi_server.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/endian_util.h"
#include "fmt/format.h"

using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
// using arrow::Int64Builder;
using arrow::StringBuilder;

namespace primihub::task {

PSIEcdhTask::PSIEcdhTask(const TaskParam *task_param,
    std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {}

retcode PSIEcdhTask::LoadParams(Task &task) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    auto param_map = task.params().param_map();
    reveal_intersection_ = true;
    try {
        std::string party_name = task.party_name();
        VLOG(2) << "party_name: " << task.party_name();
        psi_type_ = param_map["psiType"].value_int32();
        result_file_path_ = param_map["outputFullFilename"].value_string();
        auto it = param_map.find("sync_result_to_server");
        if (it != param_map.end()) {
            sync_result_to_server = it->second.value_int32();
            VLOG(5) << "sync_result_to_server: " << sync_result_to_server;
        }
        it = param_map.find("server_outputFullFilname");
        if (it != param_map.end()) {
            server_result_path = it->second.value_string();
            VLOG(5) << "server_outputFullFilname: " << server_result_path;
        }
        const auto& party_dataset = task.party_datasets();
        auto dataset_iter = party_dataset.find(party_name);
        if (dataset_iter == party_dataset.end()) {
          LOG(ERROR) << "no dataset found for party_name: " << party_name;
          return retcode::FAIL;
        } else {
          auto& dataset_map = dataset_iter->second.data();
          auto it = dataset_map.find(party_name);
          if (it == dataset_map.end()) {
            LOG(ERROR) << "no dataset found for party: " << party_name;
            return retcode::FAIL;
          }
          dataset_id_ = it->second;
          VLOG(5) << "dataset_id_: " << dataset_id_;
        }

        const auto& party_access_info = task.party_access_info();
        std::string PsiIndexName;
        if (party_name == PARTY_CLIENT) {
          run_as_client_ = true;
          auto node_iter = party_access_info.find(PARTY_SERVER);
          if (node_iter == party_access_info.end()) {
            LOG(ERROR) << "server node access info is not found";
            return retcode::FAIL;
          }
          const auto& node_info = node_iter->second;
          pbNode2Node(node_info, &peer_node);
          PsiIndexName = "clientIndex";
        } else {
          auto node_iter = party_access_info.find(PARTY_CLIENT);
          if (node_iter == party_access_info.end()) {
            LOG(ERROR) << "client node access info is not found";
            return retcode::FAIL;
          }
          const auto& node_info = node_iter->second;
          pbNode2Node(node_info, &peer_node);
          PsiIndexName = "serverIndex";
        }
        auto index_it = param_map.find(PsiIndexName);
        if (index_it != param_map.end()) {
            const auto& client_index = index_it->second;
            if (client_index.is_array()) {
                const auto& array_values = client_index.value_int32_array().value_int32_array();
                for (const auto value : array_values) {
                     data_index_.push_back(value);
                }
            } else {
                data_index_.push_back(client_index.value_int32());
            }
        }
        VLOG(5) << "data_index_ size: " << data_index_.size();
        for (const auto& i : data_index_) {
            VLOG(5) << "i: " << i;
        }
    } catch (std::exception &e) {
        std::string err_msg = fmt::format("Failed to load params: {}", e.what());
        CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, err_msg);
    }
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::LoadDataset() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    auto driver = this->getDatasetService()->getDriver(this->dataset_id_);
    if (driver == nullptr) {
        std::string err_msg = fmt::format("get driver for dataset: {} failed", this->dataset_id_);
        CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, err_msg);
    }
    auto ret = LoadDatasetInternal(driver, data_index_, elements_);
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "Load dataset for psi client failed");
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::GetIntsection(const std::unique_ptr<openminded_psi::PsiClient>& client,
                              rpc::PsiResponse& response) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    SCopedTimer timer;
    psi_proto::Response entrpy_response;
    size_t num_response_elements = response.encrypted_elements().size();
    for (size_t i = 0; i < num_response_elements; i++) {
        entrpy_response.add_encrypted_elements(
            response.encrypted_elements()[i]);
    }
    psi_proto::ServerSetup server_setup;
    server_setup.set_bits(response.server_setup().bits());
    if (response.server_setup().data_structure_case() ==
        psi_proto::ServerSetup::DataStructureCase::kGcs) {
        auto *ptr_gcs = server_setup.mutable_gcs();
        ptr_gcs->set_div(response.server_setup().gcs().div());
        ptr_gcs->set_hash_range(response.server_setup().gcs().hash_range());
    } else if (response.server_setup().data_structure_case() ==
               psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
        auto *ptr_bloom_filter = server_setup.mutable_bloom_filter();
        ptr_bloom_filter->set_num_hash_functions(
            response.server_setup().bloom_filter().num_hash_functions()
        );
    } else {
        LOG(ERROR) << "Node psi client get intersection error!";
        return retcode::FAIL;
    }
    auto build_response_time_cost = timer.timeElapse();
    VLOG(5) << "build_response_time_cost(ms): " << build_response_time_cost;

    std::vector<int64_t> intersection =
        std::move(client->GetIntersection(server_setup, entrpy_response)).value();
    auto get_intersection_ts = timer.timeElapse();
    auto get_intersection_time_cost = get_intersection_ts - build_response_time_cost;
    VLOG(5) << "get_intersection_time_cost: " << get_intersection_time_cost;
    size_t num_intersection = intersection.size();

    if (psi_type_ == rpc::PsiType::DIFFERENCE) {
        std::unordered_map<int64_t, int> inter_map(num_intersection);
        size_t num_elements = elements_.size();
        result_.reserve(num_elements);
        for (size_t i = 0; i < num_intersection; i++) {
            inter_map[intersection[i]] = 1;
        }
        for (size_t i = 0; i < num_elements; i++) {
            if (inter_map.find(i) == inter_map.end()) {
                result_.push_back(elements_[i]);
            }
        }
    } else {
        for (size_t i = 0; i < num_intersection; i++) {
            result_.push_back(elements_[intersection[i]]);
        }
    }
    return retcode::SUCCESS;
}

int PSIEcdhTask::execute() {
    SCopedTimer timer;
    auto ret = LoadParams(task_param_);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "Psi client load task params failed.";
        return -1;
    }
    auto load_param_time_cost = timer.timeElapse();
    VLOG(5) << "load params time cost(ms): " << load_param_time_cost;
    ret = LoadDataset();
    CHECK_RETCODE_WITH_RETVALUE(ret, -1);

    if (runAsClient()) {
        ret = executeAsClient();
    } else {
        ret = executeAsServer();
    }
    CHECK_RETCODE_WITH_RETVALUE(ret, -1);
    return 0;
}

retcode PSIEcdhTask::broadcastResultToServer() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    VLOG(5) << "sync intersection result to server";
    std::string result_str;
    size_t total_size{0};
    for (const auto& item : this->result_) {
        total_size += item.size();
    }
    total_size += this->result_.size() * sizeof(uint64_t);
    result_str.reserve(total_size);
    for (const auto& item : this->result_) {
        uint64_t item_len = item.size();
        uint64_t be_item_len = htonll(item_len);
        result_str.append(reinterpret_cast<char*>(&be_item_len), sizeof(be_item_len));
        result_str.append(item);
    }
    return this->send(this->key, peer_node, result_str);
}

retcode PSIEcdhTask::buildInitParam(std::string* init_params_str) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    rpc::Params init_params;
    auto param_map = init_params.mutable_param_map();
    // dataset size
    rpc::ParamValue pv_datasize;
    pv_datasize.set_var_type(rpc::VarType::INT64);
    pv_datasize.set_value_int64(elements_.size());
    pv_datasize.set_is_array(false);
    (*param_map)["dataset_size"] = pv_datasize;
    // reveal intersection
    rpc::ParamValue pv_intersection;
    pv_intersection.set_var_type(rpc::VarType::INT32);
    pv_intersection.set_value_int32(this->reveal_intersection_ ? 1 : 0);
    pv_intersection.set_is_array(false);
    (*param_map)["reveal_intersection"] = pv_intersection;
    bool success = init_params.SerializeToString(init_params_str);
    if (success) {
        return retcode::SUCCESS;
    } else {
        LOG(ERROR) << "serialize init param failed";
        return retcode::FAIL;
    }
}

retcode PSIEcdhTask::sendInitParam(const std::string& init_param) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    auto& link_ctx = this->getTaskContext().getLinkContext();
    CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
    auto channel = link_ctx->getChannel(peer_node);
    return channel->send(this->key, init_param);
}

retcode PSIEcdhTask::saveResult() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    std::string col_title =
        psi_type_ == rpc::PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    return saveDataToCSVFile(result_, result_file_path_, col_title);
}

retcode PSIEcdhTask::parsePsiResponseFromeString(
        const std::string& response_str, rpc::PsiResponse* response) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    psi_proto::Response psi_response;
    psi_proto::ServerSetup setup_info;
    char* recv_buf = const_cast<char*>(response_str.c_str());
    uint64_t* be_psi_res_len = reinterpret_cast<uint64_t*>(recv_buf);
    uint64_t psi_res_len = ntohll(*be_psi_res_len);
    recv_buf += sizeof(uint64_t);
    psi_response.ParseFromArray(recv_buf, psi_res_len);
    recv_buf += psi_res_len;
    uint64_t* be_setup_info_len = reinterpret_cast<uint64_t*>(recv_buf);
    uint64_t setup_info_len = ntohll(*be_setup_info_len);
    recv_buf += sizeof(uint64_t);
    setup_info.ParseFromArray(recv_buf, setup_info_len);
    VLOG(5) << "begin to merge task response";

    auto server_setup = response->mutable_server_setup();
    server_setup->set_bits(setup_info.bits());
    if (setup_info.data_structure_case() ==
        psi_proto::ServerSetup::DataStructureCase::kGcs) {
        auto ptr_gcs = server_setup->mutable_gcs();
        ptr_gcs->set_div(setup_info.gcs().div());
        ptr_gcs->set_hash_range(setup_info.gcs().hash_range());
    } else if (setup_info.data_structure_case() ==
               psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
        auto ptr_bloom_filter = server_setup->mutable_bloom_filter();
        ptr_bloom_filter->set_num_hash_functions(
            setup_info.bloom_filter().num_hash_functions()
        );
    } else {
        LOG(ERROR) << "Node psi client get intersection setup info error!";
        return retcode::FAIL;
    }
    size_t elements_size = psi_response.encrypted_elements_size();
    auto encrypted_elements_ptr = response->mutable_encrypted_elements();
    encrypted_elements_ptr->Reserve(elements_size);
    for (auto& encrypted_element : *(psi_response.mutable_encrypted_elements())) {
        response->add_encrypted_elements(std::move(encrypted_element));
    }
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::sendPSIRequestAndWaitResponse(
        psi_proto::Request&& request, rpc::PsiResponse* response) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    SCopedTimer timer;
    // send process
    {
        psi_proto::Request client_request = std::move(request);
        std::string psi_req_str;
        client_request.SerializeToString(&psi_req_str);
        VLOG(5) << "begin to send psi request to server";
        auto ret = this->send(this->key, peer_node, psi_req_str);
        if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "send psi reuqest to [" << peer_node.to_string() << "] failed";
            return retcode::FAIL;
        }
        VLOG(5) << "send psi request to server success";
    }

    VLOG(5) << "begin to recv psi response from server";
    std::string recv_data_str;
    auto ret = this->recv(this->key, &recv_data_str);
    if (ret != retcode::SUCCESS || recv_data_str.empty()) {
        LOG(ERROR) << "recv data is empty";
        return retcode::FAIL;
    }
    VLOG(5) << "successfully recv psi response from server";
    ret = parsePsiResponseFromeString(recv_data_str, response);
    VLOG(5) << "end of merge task response";
    return ret;
}

retcode PSIEcdhTask::sendPSIRequestAndWaitResponse(
        const psi_proto::Request& client_request, rpc::PsiResponse* response) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    SCopedTimer timer;
    std::string psi_req_str;
    client_request.SerializeToString(&psi_req_str);
    VLOG(5) << "begin to send psi request to server";
    auto ret = this->send(this->key, peer_node, psi_req_str);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send psi reuqest to [" << peer_node.to_string() << "] failed";
        return retcode::FAIL;
    }
    VLOG(5) << "send psi request to server success";
    VLOG(5) << "begin to recv psi response from server";
    std::string recv_data_str;
    ret = this->recv(this->key, &recv_data_str);
    if (ret != retcode::SUCCESS || recv_data_str.empty()) {
        LOG(ERROR) << "recv data is empty";
        return retcode::FAIL;
    }
    VLOG(5) << "successfully recv psi response from server";
    ret = parsePsiResponseFromeString(recv_data_str, response);
    VLOG(5) << "end of merge task response";
    return ret;
}

retcode PSIEcdhTask::executeAsClient() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    SCopedTimer timer;
    rpc::TaskRequest request;
    std::string init_param_str;
    VLOG(5) << "begin to build init param";
    buildInitParam(&init_param_str);
    VLOG(5) << "build init param finished";
    auto init_param_ts = timer.timeElapse();
    auto init_param_time_cost = init_param_ts;
    VLOG(5) << "buildInitParam time cost(ms): " << init_param_time_cost << " "
            << "content length: " << init_param_str.size();
    auto ret = sendInitParam(init_param_str);
    CHECK_RETCODE(ret);
    VLOG(5) << "client begin to prepare psi request";
    // prepare psi data
    auto client = openminded_psi::PsiClient::CreateWithNewKey(reveal_intersection_).value();
    psi_proto::Request client_request = client->CreateRequest(elements_).value();
    // psi_proto::Response server_response;
    auto build_request_ts = timer.timeElapse();
    auto build_request_time_cost = build_request_ts;
    VLOG(5) << "client build request time cost(ms): " << build_request_time_cost;
    // send psi data to server
    rpc::PsiResponse task_response;
    ret = sendPSIRequestAndWaitResponse(std::move(client_request), &task_response);
    CHECK_RETCODE(ret);
    auto _start = timer.timeElapse();
    ret = this->GetIntsection(client, task_response);
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "Node psi client get insection failed.");
    auto _end =  timer.timeElapse();
    auto get_intersection_time_cost = _end - _start;
    VLOG(5) << "get intersection time cost(ms): " << get_intersection_time_cost;
    ret = saveResult();
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "Save psi result failed.");
    if (this->reveal_intersection_ && this->sync_result_to_server) {
        ret = broadcastResultToServer();
        CHECK_RETCODE_WITH_ERROR_MSG(ret, "broadcastResultToServer failed");
    }
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::executeAsServer() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    SCopedTimer timer;
    size_t num_client_elements{0};
    bool reveal_intersection_flag{false};
    auto ret = recvInitParam(&num_client_elements, &reveal_intersection_flag);
    CHECK_RETCODE(ret);
    // prepare for local compuation
    VLOG(5) << "sever begin to SetupMessage";
    std::unique_ptr<openminded_psi::PsiServer> server =
        std::move(openminded_psi::PsiServer::CreateWithNewKey(reveal_intersection_flag)).value();
    // std::int64_t num_client_elements =
    //     static_cast<std::int64_t>(psi_request.encrypted_elements().size());
    psi_proto::ServerSetup server_setup =
        std::move(server->CreateSetupMessage(fpr_, num_client_elements, elements_)).value();
    VLOG(5) << "sever end of SetupMessage";
    // recv request from clinet
    VLOG(5) << "server begin to init reauest according to recv data from client";
    psi_proto::Request psi_request;
    ret = initRequest(&psi_request);
    CHECK_RETCODE(ret);
    auto init_req_ts = timer.timeElapse();
    auto init_req_time_cost = init_req_ts;
    VLOG(5) << "init_req_time_cost(ms): " << init_req_time_cost;
    psi_proto::Response server_response = std::move(server->ProcessRequest(psi_request)).value();
    VLOG(5) << "server end of process request, begin to build response";
    preparePSIResponse(std::move(server_response), std::move(server_setup));
    VLOG(5) << "server end of build psi response";
    if (this->reveal_intersection_ && this->sync_result_to_server) {
        ret = recvPSIResult();
        CHECK_RETCODE_WITH_ERROR_MSG(ret, "recv psi result failed");
    }
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::recvInitParam(size_t* client_dataset_size, bool* reveal_intersection) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    VLOG(5) << "begin to recvInitParam ";
    std::string init_param_str;
    auto ret = this->recv(this->key, &init_param_str);
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "receive init param from client failed");
    auto& client_dataset_size_ = *client_dataset_size;
    auto& reveal_flag = *reveal_intersection;
    rpc::Params init_params;
    init_params.ParseFromString(init_param_str);
    const auto& parm_map = init_params.param_map();
    auto it = parm_map.find("dataset_size");
    if (it == parm_map.end()) {
        CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, "parse dataset_size is empty");
    }
    client_dataset_size_ = it->second.value_int64();
    VLOG(5) << "client_dataset_size_: " << client_dataset_size_;
    it = parm_map.find("reveal_intersection");
    if (it == parm_map.end()) {
        CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, "parse reveal_intersection is empty");
    }
    reveal_flag = it->second.value_int32() > 0;
    VLOG(5) << "reveal_intersection_: " << reveal_flag;
    VLOG(5) << "end of recvInitParam ";
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::preparePSIResponse(psi_proto::Response&& psi_response,
                                    psi_proto::ServerSetup&& setup_info) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    VLOG(5) << "preparePSIResponse";
    // prepare response to server
    auto server_response = std::move(psi_response);
    auto server_setup = std::move(setup_info);
    std::string response_str;
    server_response.SerializeToString(&response_str);
    std::string setup_info_str;
    server_setup.SerializeToString(&setup_info_str);
    uint64_t response_len = response_str.size();
    uint64_t setup_info_len = setup_info_str.size();
    std::string psi_res_str;
    psi_res_str.reserve(response_len+setup_info_len+2*sizeof(uint64_t));
    uint64_t be_response_len = htonll(response_len);
    psi_res_str.append(reinterpret_cast<char*>(&be_response_len), sizeof(uint64_t));
    psi_res_str.append(response_str);
    uint64_t be_setup_info_len = htonll(setup_info_len);
    psi_res_str.append(reinterpret_cast<char*>(&be_setup_info_len), sizeof(uint64_t));
    psi_res_str.append(setup_info_str);
    VLOG(5) << "preparePSIResponse data length: " << psi_res_str.size();
    // pushDataToSendQueue(this->key, std::move(psi_res_str));
    VLOG(5) << "begin to send psi response to client";
    auto ret = this->send(this->key, peer_node, psi_res_str);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send psi response data to [" << peer_node.to_string() << "] failed";
        return retcode::FAIL;
    }
    VLOG(5) << "successfully send psi response to cleint";
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::initRequest(psi_proto::Request* psi_request) {
    CHECK_TASK_STOPPED(retcode::FAIL);
    std::string request_str;
    auto ret = this->recv(this->key, &request_str);
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "receive request from client failed");
    psi_proto::Request recv_psi_req;
    recv_psi_req.ParseFromString(request_str);
    *psi_request = std::move(recv_psi_req);
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::recvPSIResult() {
    CHECK_TASK_STOPPED(retcode::FAIL);
    VLOG(5) << "recvPSIResult from client";
    std::vector<std::string> psi_result;
    std::string recv_data_str;
    auto ret = this->recv(this->key, &recv_data_str);
    CHECK_RETCODE_WITH_ERROR_MSG(ret, "receive psi result from client failed");
    uint64_t offset = 0;
    uint64_t data_len = recv_data_str.length();
    VLOG(5) << "data_len_data_len: " << data_len;
    auto data_ptr = const_cast<char*>(recv_data_str.c_str());
    // format length: value
    while (offset < data_len) {
        auto len_ptr = reinterpret_cast<uint64_t*>(data_ptr+offset);
        uint64_t be_len = *len_ptr;
        uint64_t len = ntohll(be_len);
        offset += sizeof(uint64_t);
        psi_result.push_back(std::string(data_ptr+offset, len));
        offset += len;
    }
    VLOG(5) << "psi_result size: " << psi_result.size();
    std::string col_title{"intersection_row"};
    return saveDataToCSVFile(psi_result, server_result_path, col_title);
}

}   // namespace primihub::task

