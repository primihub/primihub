#include "src/primihub/task/semantic/psi_ecdh_task.h"
#include "private_set_intersection/cpp/psi_client.h"
#include "private_set_intersection/cpp/psi_server.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/endian_util.h"


using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
//using arrow::Int64Builder;
using arrow::StringBuilder;

namespace primihub::task {
PSIEcdhTask::PSIEcdhTask(const TaskParam *task_param,
    std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {}

int PSIEcdhTask::LoadParams(Task &task) {
    auto param_map = task.params().param_map();

    reveal_intersection_ = true;
    try {
        data_index_ = param_map["clientIndex"].value_int32();
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
        it = param_map.find("serverIndex");
        if (it != param_map.end()) {
            server_index_ = it->second;
        }
        it = param_map.find("serverAddress");
        if (it != param_map.end()) {
            server_address_ = it->second.value_string();
            run_as_client_ = true;
            dataset_path_ = param_map["clientData"].value_string();
        } else {
            dataset_path_ = param_map["serverData"].value_string();
        }
        it = param_map.find(server_address_);
        if (it != param_map.end()) {
            server_dataset_ = it->second.value_string();
        }
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    const auto& node_map = task.node_map();
    for (const auto& it : node_map) {
        std::string node_id = it.first;
        if (node_id == this->node_id_) {
            continue;
        }
        const auto& node_info = it.second;
        peer_node.ip_ = node_info.ip();
        peer_node.port_ = node_info.port();
        peer_node.use_tls_ = node_info.use_tls();
        VLOG(5) << "peer_node: " << peer_node.to_string();
        break;
    }
    return 0;
}

int PSIEcdhTask::LoadDatasetFromSQLite(
      const std::string &conn_str, int data_col, std::vector<std::string>& col_array) {
    std::string nodeaddr{"localhost"};
    // std::shared_ptr<DataDriver>
    auto driver = DataDirverFactory::getDriver("SQLITE", nodeaddr);
    auto& cursor = driver->read(conn_str);
    auto ds = cursor->read();
    auto table = std::get<std::shared_ptr<Table>>(ds->data);
    int col_count = table->num_columns();
    if(col_count < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col, "
            << "dataset total colum: " << col_count
            << "expected col index: " << data_col;
        return -1;
    }
    auto array = std::static_pointer_cast<StringArray>(table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        col_array.push_back(array->GetString(i));
    }
    VLOG(0) << "loaded records number: " << col_array.size();
    return col_array.size();
}

int PSIEcdhTask::LoadDatasetFromCSV(
      const std::string &filename, int data_col, std::vector <std::string> &col_array) {
    std::string nodeaddr("test address"); // TODO
    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", nodeaddr);
    std::shared_ptr<Cursor> &cursor = driver->read(filename);
    std::shared_ptr<Dataset> ds = cursor->read();
    std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col";
        return -1;
    }

    auto array = std::static_pointer_cast<StringArray>(
        table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        col_array.push_back(array->GetString(i));
    }
    return array->length();
}

int PSIEcdhTask::LoadDataset() {
    // TODO fixme trick method, search sqlite as keyword and if find then laod data from sqlite
    std::string match_word{"sqlite"};
    std::string driver_type;
    if (dataset_path_.size() > match_word.size()) {
        driver_type = dataset_path_.substr(0, match_word.size());
    } else {
        driver_type = dataset_path_;
    }
    // currently, we supportes only two kind of storage type [csv, sqlite] as dataset
    int ret = 0;
    if (match_word == driver_type) {
        ret = LoadDatasetFromSQLite(dataset_path_, data_index_, elements_);
    } else {
        ret = LoadDatasetFromCSV(dataset_path_, data_index_, elements_);
    }
    // load datasets encountes error or file empty
    if (ret <= 0) {
        LOG(ERROR) << "Load dataset for psi client failed. dataset size: " << ret;
        return -1;
    }
    return 0;
}

int PSIEcdhTask::GetIntsection(const std::unique_ptr<openminded_psi::PsiClient>& client,
                              rpc::PsiResponse& response) {
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
        return -1;
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
    return 0;
}

int PSIEcdhTask::execute() {
    SCopedTimer timer;
    int ret = LoadParams(task_param_);
    if (ret) {
        LOG(ERROR) << "Psi client load task params failed.";
        return ret;
    }
    auto load_param_time_cost = timer.timeElapse();
    VLOG(5) << "load params time cost(ms): " << load_param_time_cost;

    ret = LoadDataset();
    if (ret) {
        LOG(ERROR) << "Psi client load dataset failed.";
        return ret;
    }
    if (runAsClient()) {
        executeAsClient();
    } else {
        executeAsServer();
    }
    return 0;
}
retcode PSIEcdhTask::broadcastResultToServer() {
    VLOG(5) << "send_result_to_server";
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
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node);
    auto ret = channel->send(this->key, result_str);
    VLOG(5) << "send result to server success";
    return ret;
}

retcode PSIEcdhTask::buildInitParam(std::string* init_params_str) {
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
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node);
    channel->send(this->key, init_param);
    return retcode::SUCCESS;
}

int PSIEcdhTask::saveResult() {
    std::string col_title =
        psi_type_ == rpc::PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    saveDataToCSVFile(result_, result_file_path_, col_title);
    return 0;
}

retcode PSIEcdhTask::parsePsiResponseFromeString(const std::string& response_str, rpc::PsiResponse* response) {
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

    for (const auto& encrypted_element : psi_response.encrypted_elements()) {
        response->add_encrypted_elements(encrypted_element);
    }
    return retcode::SUCCESS;
}

retcode PSIEcdhTask::sendPSIRequestAndWaitResponse(
        const psi_proto::Request& client_request, rpc::PsiResponse* response) {
    SCopedTimer timer;
    std::string psi_req_str;
    client_request.SerializeToString(&psi_req_str);
    std::string recv_data_str;
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node);
    channel->sendRecv(this->key, psi_req_str, &recv_data_str);
    if (recv_data_str.empty()) {
        return retcode::FAIL;
    }
    auto ret = parsePsiResponseFromeString(recv_data_str, response);
    VLOG(5) << "end of merge task response";
    return ret;
}

int PSIEcdhTask::executeAsClient() {
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
    sendInitParam(init_param_str);
    VLOG(5) << "client begin to prepare psi request";
    // prepare psi data
    auto client = openminded_psi::PsiClient::CreateWithNewKey(reveal_intersection_).value();
    psi_proto::Request client_request = client->CreateRequest(elements_).value();
    psi_proto::Response server_response;
    auto build_request_ts = timer.timeElapse();
    auto build_request_time_cost = build_request_ts;
    VLOG(5) << "client build request time cost(ms): " << build_request_time_cost;
    // send psi data to server
    rpc::PsiResponse task_response;
    sendPSIRequestAndWaitResponse(client_request, &task_response);
    auto _start = timer.timeElapse();
    int ret = this->GetIntsection(client, task_response);
    if (ret) {
        LOG(ERROR) << "Node psi client get insection failed.";
        return -1;
    }
    auto _end =  timer.timeElapse();
    auto get_intersection_time_cost = _end - _start;
    VLOG(5) << "get intersection time cost(ms): " << get_intersection_time_cost;
    ret = saveResult();
    if (ret) {
        LOG(ERROR) << "Save psi result failed.";
        return -1;
    }
    if (this->reveal_intersection_ && this->sync_result_to_server) {
        broadcastResultToServer();
    }
}

int PSIEcdhTask::executeAsServer() {
    SCopedTimer timer;
    size_t num_client_elements{0};
    bool reveal_intersection_flag{false};
    recvInitParam(&num_client_elements, &reveal_intersection_flag);
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
    initRequest(&psi_request);
    auto init_req_ts = timer.timeElapse();
    auto init_req_time_cost = init_req_ts;
    VLOG(5) << "init_req_time_cost(ms): " << init_req_time_cost;
    psi_proto::Response server_response = std::move(server->ProcessRequest(psi_request)).value();
    VLOG(5) << "server end of process request, begin to build response";
    preparePSIResponse(std::move(server_response), std::move(server_setup));
    VLOG(5) << "server end of build psi response";
    if (this->reveal_intersection_ && this->sync_result_to_server) {
        recvPSIResult();
    }
}

int PSIEcdhTask::recvInitParam(size_t* client_dataset_size, bool* reveal_intersection) {
    auto& client_dataset_size_ = *client_dataset_size;
    auto& reveal_flag = *reveal_intersection;
    auto& recv_queue = this->getTaskContext().getRecvQueue(this->key);
    VLOG(5) << "begin to recvInitParam ";
    std::string init_param_str;
    recv_queue.wait_and_pop(init_param_str);
    rpc::Params init_params;
    init_params.ParseFromString(init_param_str);
    const auto& parm_map = init_params.param_map();
    auto it = parm_map.find("dataset_size");
    if (it != parm_map.end()) {
        client_dataset_size_ = it->second.value_int64();
        VLOG(5) << "client_dataset_size_: " << client_dataset_size_;
    }
    it = parm_map.find("reveal_intersection");
    if (it != parm_map.end()) {
        reveal_flag = it->second.value_int32() > 0;
        VLOG(5) << "reveal_intersection_: " << reveal_flag;
    }
    VLOG(5) << "end of recvInitParam ";
    return 0;
}

int PSIEcdhTask::preparePSIResponse(psi_proto::Response&& psi_response,
                                    psi_proto::ServerSetup&& setup_info) {
    SCopedTimer timer;
    VLOG(5) << "preparePSIResponse";
    // prepare response to server
    auto server_response = std::move(psi_response);
    auto server_setup = std::move(setup_info);
    auto& send_queue = this->getTaskContext().getSendQueue(this->key);
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
    send_queue.push(std::move(psi_res_str));
    auto build_response_ts = timer.timeElapse();
    auto build_response_time_cost = build_response_ts;
    VLOG(5) << "build_response_time_cost(ms): " << build_response_time_cost;
    return 0;
}

int PSIEcdhTask::initRequest(psi_proto::Request* psi_request) {
    auto& recv_queue = this->getTaskContext().getRecvQueue(this->key);
    // rpc::TaskRequest request;
    std::string request_str;
    recv_queue.wait_and_pop(request_str);
    psi_proto::Request recv_psi_req;
    recv_psi_req.ParseFromString(request_str);
    *psi_request = std::move(recv_psi_req);
    return 0;
}

int PSIEcdhTask::recvPSIResult() {
    VLOG(5) << "recvPSIResult from client";
    std::vector<std::string> psi_result;
    auto& recv_queue = this->getTaskContext().getRecvQueue(this->key);
    std::string recv_data_str;
    recv_queue.wait_and_pop(recv_data_str);
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
    saveDataToCSVFile(psi_result, server_result_path, col_title);
    return 0;
}

int PSIEcdhTask::saveDataToCSVFile(const std::vector<std::string>& data,
        const std::string& file_path, const std::string& col_title) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::StringBuilder builder(pool);
    builder.AppendValues(data);
    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);
    std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
        arrow::field(col_title, arrow::utf8())};
    auto schema = std::make_shared<arrow::Schema>(schema_vector);
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});
    auto driver = DataDirverFactory::getDriver("CSV", "test address");
    auto csv_driver = std::dynamic_pointer_cast<CSVDriver>(driver);
    if (ValidateDir(file_path)) {
        LOG(ERROR) << "can't access file path: " << file_path;
        return -1;
    }
    int ret = csv_driver->write(table, file_path);
    if (ret != 0) {
        LOG(ERROR) << "Save PSI result to file " << file_path << " failed.";
        return -1;
    }
    LOG(INFO) << "Save PSI result to " << file_path << ".";
    return 0;
}

}   // namespace primihub::task
