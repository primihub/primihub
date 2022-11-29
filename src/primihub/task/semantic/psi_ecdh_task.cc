#include "src/primihub/task/semantic/psi_ecdh_task.h"
#include "private_set_intersection/cpp/psi_client.h"
#include "private_set_intersection/cpp/psi_server.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"


using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
//using arrow::Int64Builder;
using arrow::StringBuilder;
using primihub::rpc::VarType;

namespace primihub::task {
PSIECDHTask::PSIECDHTask(const TaskParam *task_param,
    std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {}

void PSIECDHTask::setTaskInfo(const std::string& node_id,
        const std::string& job_id,
        const std::string& task_id,
        const std::string& submit_client_id) {
    node_id_ = node_id;
    task_id_ = task_id;
    job_id_ = job_id;
    submit_client_id_ = submit_client_id;
}

int PSIECDHTask::LoadParams(Task &task) {
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
    return 0;
}

int PSIECDHTask::LoadDatasetFromSQLite(
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

int PSIECDHTask::LoadDatasetFromCSV(
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

int PSIECDHTask::LoadDataset() {
    // TODO fixme trick method, search sqlite as keyword and if find then laod data from sqlite
    std::string match_word{"sqlite"};
    std::string driver_type;
    if (dataset_path_.size() > match_word.size()) {
        driver_type = dataset_path_.substr(0, match_word.size());
    } else {
        driver_type = dataset_path_;
    }
    // current we supportes only two type of strage type [csv, sqlite] as dataset
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

int PSIECDHTask::GetIntsection(const std::unique_ptr<openminded_psi::PsiClient>& client,
                              TaskResponse& taskResponse) {
    SCopedTimer timer;
    psi_proto::Response entrpy_response;
    size_t num_response_elements = taskResponse.psi_response().encrypted_elements().size();
    for (size_t i = 0; i < num_response_elements; i++) {
        entrpy_response.add_encrypted_elements(
            taskResponse.psi_response().encrypted_elements()[i]);
    }
    psi_proto::ServerSetup server_setup;
    server_setup.set_bits(taskResponse.psi_response().server_setup().bits());
    if (taskResponse.psi_response().server_setup().data_structure_case() ==
        psi_proto::ServerSetup::DataStructureCase::kGcs) {
        auto *ptr_gcs = server_setup.mutable_gcs();
        ptr_gcs->set_div(taskResponse.psi_response().server_setup().gcs().div());
        ptr_gcs->set_hash_range(taskResponse.psi_response().server_setup().gcs().hash_range());
    } else if (taskResponse.psi_response().server_setup().data_structure_case() ==
               psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
        auto *ptr_bloom_filter = server_setup.mutable_bloom_filter();
        ptr_bloom_filter->set_num_hash_functions(
            taskResponse.psi_response().server_setup().bloom_filter().num_hash_functions()
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

    if (psi_type_ == PsiType::DIFFERENCE) {
        std::unordered_map<int64_t, int> inter_map(num_intersection);
        size_t num_elements = elements_.size();
        for (size_t i = 0; i < num_intersection; i++) {
            inter_map[intersection[i]] = 1;
        }
        for (size_t i = 0; i < num_elements; i++) {
            if (inter_map.find(i) == inter_map.end()) {
                // outFile << i << std::endl;
                result_.push_back(elements_[i]);
            }
        }
    } else {
        for (size_t i = 0; i < num_intersection; i++) {
            // outFile << intersection[i] << std::endl;
            result_.push_back(elements_[intersection[i]]);
        }
    }
    return 0;
}

int PSIECDHTask::execute() {
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

int PSIECDHTask::send_result_to_server() {
    grpc::ClientContext context;
    VLOG(5) << "send_result_to_server";
     // std::unique_ptr<VMNode::Stub>
    auto channel = grpc::InsecureChannelCredentials();
    auto stub = VMNode::NewStub(grpc::CreateChannel(server_address_, channel));
    primihub::rpc::TaskResponse task_response;
    std::unique_ptr<grpc::ClientWriter<primihub::rpc::TaskRequest>> writer(stub->Send(&context, &task_response));
    constexpr size_t limited_size = 1 << 22;  // limit data size 4M
    size_t sended_size = 0;
    size_t sended_index = 0;
    bool add_head_flag = false;
    do {
        primihub::rpc::TaskRequest task_request;
        task_request.set_job_id(this->job_id_);
        task_request.set_task_id(this->task_id_);
        task_request.set_storage_type(primihub::rpc::TaskRequest::FILE);
        task_request.set_storage_info(server_result_path);
        auto data_ptr = task_request.mutable_data();
        data_ptr->reserve(limited_size);
        size_t pack_size = 0;
        size_t item_len = 0;
        for (size_t i = sended_index; i < this->result_.size(); i++) {
            auto& data_item = this->result_[i];
            item_len = data_item.size();
            if (pack_size + item_len + sizeof(size_t) > limited_size) {
                break;
            }
            data_ptr->append(reinterpret_cast<char*>(&item_len), sizeof(item_len));
            pack_size += sizeof(item_len);
            data_ptr->append(data_item);
            pack_size += item_len;
            sended_index++;
        }
        writer->Write(task_request);
        sended_size += pack_size;
        VLOG(5) << "sended_size: " << sended_size << " "
                << "sended_index: " << sended_index << " "
                << "result size: " << this->result_.size();
        if (sended_index >= this->result_.size()) {
            break;
        }
    } while(true);
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        auto ret_code = task_response.ret_code();
        if (ret_code) {
            LOG(ERROR) << "client Node send result data to server return failed error code: " << ret_code;
            return -1;
        }
    } else {
        LOG(ERROR) << "client Node send result data to server failed. error_code: "
                   << status.error_code() << ": " << status.error_message();
        return -1;
    }
    VLOG(5) << "send result to server success";
}

int PSIECDHTask::saveResult() {
    std::string col_title =
        psi_type_ == PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    saveDataToCSVFile(result_, result_file_path_, col_title);
    return 0;
}

int PSIECDHTask::executeAsClient() {
    SCopedTimer timer;
    auto client = openminded_psi::PsiClient::CreateWithNewKey(reveal_intersection_).value();
    psi_proto::Request client_request = client->CreateRequest(elements_).value();
    psi_proto::Response server_response;
    auto build_request_ts = timer.timeElapse();
    auto build_request_time_cost = build_request_ts;
    VLOG(5) << "client build request time cost(ms): " << build_request_time_cost;
    grpc::ClientContext context;
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(128*1024*1024);
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateCustomChannel(server_address_, grpc::InsecureChannelCredentials(), channel_args);
    std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(channel);
    // std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(grpc::CreateChannel(
    //     server_address_, grpc::InsecureChannelCredentials()));
    using stream_t = std::shared_ptr<grpc::ClientReaderWriter<TaskRequest, TaskResponse>>;
    stream_t client_stream(stub->SendRecv(&context));

    size_t limited_size = 1 << 21;
    size_t num_elements = client_request.encrypted_elements().size();
    const auto& encrypted_elements = client_request.encrypted_elements();
    VLOG(5) << "num_elements_num_elements: " << num_elements;
    size_t sended_index{0};
    std::vector<TaskRequest> send_requests;
    do {
        TaskRequest taskRequest;
        taskRequest.set_job_id(job_id_);
        taskRequest.set_task_id(task_id_);
        auto ptr_request = taskRequest.mutable_psi_request();
        ptr_request->set_reveal_intersection(client_request.reveal_intersection());
        ptr_request->set_job_id(job_id_);
        ptr_request->set_task_id(task_id_);
        size_t pack_size = 0;
        for (size_t i = sended_index; i < num_elements; i++) {
            const auto& element = encrypted_elements[i];
            auto element_len = element.size();
            if (pack_size + element_len > limited_size) {
                break;
            }
            ptr_request->add_encrypted_elements(element);
            pack_size += element_len;
            sended_index++;
        }
        // auto *ptr_params = taskRequest.mutable_params()->mutable_param_map();
        // ParamValue pv;
        // pv.set_var_type(VarType::STRING);
        // pv.set_value_string(server_dataset_);
        // (*ptr_params)["serverData"] = pv;
        // (*ptr_params)["serverIndex"] = server_index_;
        send_requests.push_back(std::move(taskRequest));
        if (sended_index >= num_elements) {
            break;
        }
    } while (true);
    VLOG(5) << "send_requests size: " << send_requests.size();
    // send request to server
    for (const auto& request : send_requests) {
        client_stream->Write(request);
    }
    client_stream->WritesDone();
    auto send_data_ts = timer.timeElapse();
    auto send_data_time_cost = send_data_ts - build_request_ts;
    VLOG(5) << "client send request to server time cost(ms): " << send_data_time_cost;
    TaskResponse taskResponse;
    TaskResponse recv_response;
    auto psi_response = taskResponse.mutable_psi_response();
    bool is_initialized{false};
    while (client_stream->Read(&recv_response)) {
        const auto& _psi_response = recv_response.psi_response();
        if (!is_initialized) {
            const auto& res_server_setup = _psi_response.server_setup();
            auto server_setup = psi_response->mutable_server_setup();
            server_setup->CopyFrom(res_server_setup);
            is_initialized = true;
        }
        for (const auto& encrypted_element : _psi_response.encrypted_elements()) {
            psi_response->add_encrypted_elements(encrypted_element);
        }
    }
    Status status = client_stream->Finish();
    if (status.ok()) {
        if (taskResponse.psi_response().ret_code()) {
            LOG(ERROR) << "Node psi server process request error.";
            return -1;
        }
        auto _start = timer.timeElapse();
        int ret = this->GetIntsection(client, taskResponse);
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
    } else {
        LOG(ERROR) << "Node push psi server task rpc failed. "
                   << status.error_code() << ": " << status.error_message();
        return -1;
    }
    if (this->reveal_intersection_ && this->sync_result_to_server) {
        send_result_to_server();
    }
}

int PSIECDHTask::executeAsServer() {
    SCopedTimer timer;
    psi_proto::Request psi_request;
    initRequest(&psi_request);
    auto init_req_ts = timer.timeElapse();
    auto init_req_time_cost = init_req_ts;
    VLOG(5) << "init_req_time_cost(ms): " << init_req_time_cost;
    std::unique_ptr<openminded_psi::PsiServer> server =
        std::move(openminded_psi::PsiServer::CreateWithNewKey(psi_request.reveal_intersection())).value();
    std::int64_t num_client_elements =
        static_cast<std::int64_t>(psi_request.encrypted_elements().size());
    psi_proto::ServerSetup server_setup =
        std::move(server->CreateSetupMessage(fpr_, num_client_elements, elements_)).value();

    psi_proto::Response server_response = std::move(server->ProcessRequest(psi_request)).value();

    preparePSIResponse(std::move(server_response), std::move(server_setup));

    if (this->reveal_intersection_ && this->sync_result_to_server) {
        recvPSIResult();
    }
}

int PSIECDHTask::preparePSIResponse(psi_proto::Response&& psi_response,
                                    psi_proto::ServerSetup&& setup_info) {
    SCopedTimer timer;
    VLOG(5) << "preparePSIResponse";
    // prepare response to server
    auto server_response = std::move(psi_response);
    auto server_setup = std::move(setup_info);
    auto& send_queue = this->getTaskContext().getSendQueue();
    // auto proceess_request_ts = timer.timeElapse();
    // auto proceess_request_time_cost = proceess_request_ts - init_req_ts;
    // VLOG(5) << "proceess_request_time_cost(ms): " << proceess_request_time_cost;
    // std::int64_t num_response_elements =
    //     static_cast<std::int64_t>(server_response.encrypted_elements().size());

    // for (std::int64_t i = 0; i < num_response_elements; i++) {
    //     response_->add_encrypted_elements(server_response.encrypted_elements()[i]);
    // }

    // auto ptr_server_setup = response_->mutable_server_setup();
    // ptr_server_setup->set_bits(server_setup.bits());
    // if (server_setup.data_structure_case() ==
    //     psi_proto::ServerSetup::DataStructureCase::kGcs) {
    //     ptr_server_setup->mutable_gcs()->set_div(server_setup.gcs().div());
    //     ptr_server_setup->mutable_gcs()->set_hash_range(server_setup.gcs().hash_range());
    // } else if (server_setup.data_structure_case() ==
    //            psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
    //     ptr_server_setup->mutable_bloom_filter()->
    //         set_num_hash_functions(server_setup.bloom_filter().num_hash_functions());
    // }
    size_t limited_size = 1 << 21; // 4M
    size_t encrypted_elements_num = server_response.encrypted_elements().size();
    const auto& encrypted_elements = server_response.encrypted_elements();
    size_t sended_size = 0;
    size_t sended_index = 0;
    do {
        TaskResponse sub_resp;
        sub_resp.set_last_frame(false);
        auto sub_psi_res = sub_resp.mutable_psi_response();
        sub_psi_res->set_ret_code(0);
        size_t pack_size = 0;
        // auto sub_server_setup = sub_psi_res->mutable_server_setup();
        // sub_server_setup->CopyFrom(server_setup);
        auto ptr_server_setup = sub_psi_res->mutable_server_setup();
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
        for (size_t i = sended_index; i < encrypted_elements_num; i++) {
            const auto& data_item = encrypted_elements[i];
            size_t item_len = data_item.size();
            if (pack_size + item_len > limited_size) {
                break;
            }
            sub_psi_res->add_encrypted_elements(data_item);
            pack_size += item_len;
            sended_index++;
        }
        send_queue.push(std::move(sub_resp));
        VLOG(5) << "pack_size+pack_size: " << pack_size;
        if (sended_index >= encrypted_elements_num) {
            break;
        }
    } while(true);
    TaskResponse sub_resp;
    sub_resp.set_last_frame(true);
    send_queue.push(std::move(sub_resp));
    auto build_response_ts = timer.timeElapse();
    auto build_response_time_cost = build_response_ts;
    VLOG(5) << "build_response_time_cost(ms): " << build_response_time_cost;
    return 0;
}

int PSIECDHTask::initRequest(psi_proto::Request* psi_request) {
    auto& recv_queue = this->getTaskContext().getRecvQueue();
    do {
        TaskRequest request;
        recv_queue.wait_and_pop(request);
        if (request.last_frame()) {
            break;
        }
        const auto& req = request.psi_request();
        psi_request->set_reveal_intersection(req.reveal_intersection());
        for (const auto& encrypted_element : req.encrypted_elements()) {
            psi_request->add_encrypted_elements(encrypted_element);
        }
    } while (true);
    return 0;
}

int PSIECDHTask::recvPSIResult() {
    VLOG(5) << "recvPSIResult from client";
    std::vector<std::string> psi_result;
    auto& recv_queue = this->getTaskContext().getRecvQueue();
    do {
        TaskRequest request;
        recv_queue.wait_and_pop(request);
        if (request.last_frame()) {
            break;
        }
        size_t offset = 0;
        size_t data_len = request.data().length();
        VLOG(5) << "data_len_data_len: " << data_len;
        auto data_ptr = const_cast<char*>(request.data().c_str());
        // format length: value
        while (offset < data_len) {
            auto len_ptr = reinterpret_cast<size_t*>(data_ptr+offset);
            size_t len = *len_ptr;
            offset += sizeof(size_t);
            psi_result.push_back(std::string(data_ptr+offset, len));
            offset += len;
        }
    } while (true);

    VLOG(5) << "psi_result size: " << psi_result.size();
    std::string col_title{"intersection_row"};
    saveDataToCSVFile(psi_result, server_result_path, col_title);
    return 0;
}

int PSIECDHTask::saveDataToCSVFile(const std::vector<std::string>& data,
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
