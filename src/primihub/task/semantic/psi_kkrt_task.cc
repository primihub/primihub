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

#ifndef __APPLE__
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Network/Endpoint.h"
#include "cryptoTools/Network/SocketAdapter.h"
#include "cryptoTools/Network/Session.h"
#include "cryptoTools/Common/config.h"
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Crypto/RandomOracle.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Common/Timer.h"
#endif

#include "src/primihub/task/semantic/psi_kkrt_task.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include <glog/logging.h>
#include <chrono>

#ifndef __APPLE__
#include "libPSI/PSI/Kkrt/KkrtPsiSender.h"

#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h"
#include "libOTe/NChooseOne/NcoOtExt.h"
#endif

#include <numeric>


#ifndef __APPLE__
using namespace osuCrypto;
#endif
using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
using arrow::Int64Builder;
using primihub::rpc::VarType;
using std::map;

namespace primihub::task {


PSIKkrtTask::PSIKkrtTask(const std::string &node_id,
                         const std::string &job_id,
                         const std::string &task_id,
                         const TaskParam *task_param,
                         std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int PSIKkrtTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();
    auto param_map_it = param_map.find("serverAddress");

    if (param_map_it != param_map.end()) {
        //role_tag_ = EpMode::Client;
        role_tag_ = 0;
        try {
            VLOG(5) << "parse paramerter";
            auto it = param_map.find("sync_result_to_server");
            if (it != param_map.end()) {
                sync_result_to_server = it->second.value_int32() > 0;
                V_VLOG(5) << "sync_result_to_server: " << sync_result_to_server;
            }
            it = param_map.find("server_outputFullFilname");
            if (it != param_map.end()) {
                server_result_path = it->second.value_string();
                V_VLOG(5) << "server_outputFullFilname: " << server_result_path;
            }
            data_index_ = param_map["clientIndex"].value_int32();
	        psi_type_ = param_map["psiType"].value_int32();
            dataset_path_ = param_map["clientData"].value_string();
            result_file_path_ = param_map["outputFullFilename"].value_string();
            host_address_ = param_map["serverAddress"].value_string();
            V_VLOG(5) << "serverAddress: " << host_address_;
            std::vector<std::string> server_info;
            str_split(host_address_, &server_info, ':');
            if (server_info.size() == 3) {
                host_address_ = server_info[0] + ":" + server_info[1];
                if (std::stoi(server_info[2])) {
                    this->set_use_tls(true);
                }
            }
        } catch (std::exception &e) {
            LOG_ERROR() << "Failed to load params: " << e.what();
            return -1;
        }
    } else {
        //role_tag_ = EpMode::Server;
        role_tag_ = 1;
        try {
            data_index_ = param_map["serverIndex"].value_int32();
            dataset_path_ = param_map["serverData"].value_string();
            host_address_ = param_map["clientAddress"].value_string();
        } catch (std::exception &e) {
            LOG_ERROR() << "Failed to load params: " << e.what();
            return -1;
        }
    }

    return 0;
}

int PSIKkrtTask::_LoadDatasetFromSQLite(std::string& conn_str, int data_col,
                                        std::vector<std::string>& col_array) {
    std::string nodeaddr("localhost");
    // std::shared_ptr<DataDriver>
    auto driver = DataDirverFactory::getDriver("SQLITE", nodeaddr);
    if (driver == nullptr) {
        LOG_ERROR() << "create sqlite db driver failed";
        return -1;
    }
    // std::shared_ptr<Cursor> &cursor
    auto& cursor = driver->read(conn_str);
    // std::shared_ptr<Dataset>
    auto ds = cursor->read();
    if (ds == nullptr) {
        return -1;
    }
    auto table = std::get<std::shared_ptr<Table>>(ds->data);
    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col";
        return -1;
    }

    auto array = std::static_pointer_cast<StringArray>(table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        col_array.push_back(array->GetString(i));
    }
    V_VLOG(5) << "psi server loaded data records: " << col_array.size();
    return 0;
}

int PSIKkrtTask::_LoadDatasetFromCSV(std::string &filename, int data_col,
                                     std::vector <std::string> &col_array) {
    std::string nodeaddr("test address"); // TODO
    std::shared_ptr<DataDriver> driver =  DataDirverFactory::getDriver("CSV", nodeaddr);
    std::shared_ptr<Cursor> &cursor = driver->read(filename);
    std::shared_ptr<Dataset> ds = cursor->read();
    std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG_ERROR() << "psi dataset colunum number is smaller than data_col";
        return -1;
    }

    auto array = std::static_pointer_cast<StringArray>(
        table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        col_array.push_back(array->GetString(i));
    }
    return 0;
}

int PSIKkrtTask::_LoadDataset(void) {
    std::string match_word{"sqlite"};
    std::string driver_type;
    if (dataset_path_.size() > match_word.size()) {
        driver_type = dataset_path_.substr(0, match_word.size());
    } else {
        driver_type = dataset_path_;
    }
    // current we supportes [csv, sqlite] as dataset
    int ret = -1;
    if (match_word == driver_type) {
        ret = _LoadDatasetFromSQLite(dataset_path_, data_index_, elements_);
    } else {
        ret = _LoadDatasetFromCSV(dataset_path_, data_index_, elements_);
    }
     // file reading error or file empty
    if (ret) {
        LOG_ERROR() << "Load dataset for psi server failed. dataset size: " << ret;
        return -1;
    }
    return 0;
}

#ifndef __APPLE__
void PSIKkrtTask::_kkrtRecv(Channel& chl) {
    u8 dummy[1];
    PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 sendSize;
    //u64 recvSize = 10;
    u64 recvSize = elements_.size();

    std::vector<u64> data{recvSize};
    chl.asyncSend(std::move(data));
    std::vector<u64> dest;
    chl.recv(dest);

    //LOG_INFO() << "send size:" << dest[0];
    sendSize = dest[0];
    std::vector<block> sendSet(sendSize), recvSet(recvSize);

    u8 block_size = sizeof(block);
    RandomOracle  sha1(block_size);
    u8 hash_dest[block_size];
    for (u64 i = 0; i < recvSize; ++i) {
        sha1.Update((u8 *)elements_[i].data(), elements_[i].size());
        sha1.Final((u8 *)hash_dest);
        recvSet[i] = toBlock(hash_dest);
        sha1.Reset();
    }
    KkrtNcoOtReceiver otRecv;
    KkrtPsiReceiver recvPSIs;
    //LOG_INFO() << "client step 1";

    //recvPSIs.setTimer(gTimer);
    chl.recv(dummy, 1);
    //LOG_INFO() << "client step 2";
    //gTimer.reset();
    chl.asyncSend(dummy, 1);
    //LOG_INFO() << "client step 3";
    //Timer timer;
    //auto start = timer.setTimePoint("start");
    recvPSIs.init(sendSize, recvSize, 40, chl, otRecv, prng.get<block>());
    //LOG_INFO() << "client step 4";
    //auto mid = timer.setTimePoint("init");
    recvPSIs.sendInput(recvSet, chl);
    //LOG_INFO() << "client step 5";
    //auto end = timer.setTimePoint("done");

    _GetIntsection(recvPSIs);
}

void PSIKkrtTask::_kkrtSend(Channel& chl) {
    u8 dummy[1];
    PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 sendSize = elements_.size();
    u64 recvSize;

    std::vector<u64> data{sendSize};
    chl.asyncSend(std::move(data));
    std::vector<u64> dest;
    chl.recv(dest);

    //LOG_INFO() << "recv size:" << dest[0];
    recvSize = dest[0];
    std::vector<block> set(sendSize);
    u8 block_size = sizeof(block);
    RandomOracle  sha1(block_size);
    u8 hash_dest[block_size];
    //prng.get(set.data(), set.size());
    for (u64 i = 0; i < sendSize; ++i) {
        sha1.Update((u8 *)elements_[i].data(), elements_[i].size());
        sha1.Final((u8 *)hash_dest);
        set[i] = toBlock(hash_dest);
        sha1.Reset();
    }

    KkrtNcoOtSender otSend;
    KkrtPsiSender sendPSIs;
    //LOG_INFO() << "server step 1";

    sendPSIs.setTimer(gTimer);
    chl.asyncSend(dummy, 1);
    //LOG_INFO() << "server step 2";
    chl.recv(dummy, 1);
    //LOG_INFO() << "server step 3";
    sendPSIs.init(sendSize, recvSize, 40, chl, otSend, prng.get<block>());
    //LOG_INFO() << "server step 4";
    sendPSIs.sendInput(set, chl);
    //LOG_INFO() << "server step 5";

    u64 dataSent = chl.getTotalDataSent();
    //LOG_INFO() << "server step 6";

    chl.resetStats();
    //LOG_INFO() << "server step 7";
}

int PSIKkrtTask::_GetIntsection(KkrtPsiReceiver &receiver) {
    /*for (auto pos : receiver.mIntersection) {
        LOG_INFO() << pos;
    }*/

    if (psi_type_ == PsiType::DIFFERENCE) {
        map<u64, int> inter_map;
        for (auto pos : receiver.mIntersection) {
            inter_map[pos] = 1;
        }
        u64 num_elements = elements_.size();
        for (u64 i = 0; i < num_elements; i++) {
            if (inter_map.find(i) == inter_map.end()) {
                result_.push_back(elements_[i]);
            }
        }
    } else {
        for (auto pos : receiver.mIntersection) {
            result_.push_back(elements_[pos]);
        }
    }
    return 0;
}
#endif


int PSIKkrtTask::send_result_to_server() {
#ifndef __APPLE__
    // cause grpc port is alive along with node life duration,
    // so send result data to server by grpc
    grpc::ClientContext context;
    V_VLOG(5) << "send_result_to_server";
    // auto channel = grpc::InsecureChannelCredentials();
    auto channel = buildChannel(host_address_, node_id(), use_tls());
    // std::unique_ptr<VMNode::Stub>
    auto stub = primihub::rpc::VMNode::NewStub(channel);
    primihub::rpc::TaskResponse task_response;
    std::unique_ptr<grpc::ClientWriter<primihub::rpc::TaskRequest>>
        writer(stub->Send(&context, &task_response));
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
        if (!add_head_flag) {
            task_request.add_data("\"intersection_row\"");
            add_head_flag = true;
        }
        size_t pack_size = 0;
        for (size_t i = sended_index; i < this->result_.size(); i++) {
            auto& data_item = this->result_[i];
            size_t item_len = data_item.size();
            if (pack_size + item_len > limited_size) {
                break;
            }
            task_request.add_data(data_item);
            pack_size += item_len;
            sended_index++;
        }

        writer->Write(task_request);
        sended_size += pack_size;
        V_VLOG(5) << "sended_size: " << sended_size 
                  << "sended_index: " << sended_index << " "
                  << "result size: " << this->result_.size();
        if (sended_index >= this->result_.size()) {
            V_VLOG(5) << " sended_index: " << sended_index
                    << " result size: " << this->result_.size() << " end of send";
            break;
        }
    } while(true);
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    V_VLOG(0) << "writer->Finish";
    if (status.ok()) {
        auto ret_code = task_response.ret_code();
        if (ret_code) {
            LOG_ERROR() << "client Node send result data to server return failed error code: " << ret_code;
            return -1;
        }
    } else {
        LOG_ERROR() << "client Node send result data to server failed. error_code: "
                   << status.error_code() << ": " << status.error_message();
        return -1;
    }
    V_VLOG(0) << "send result to server success";
#endif
    return 0;
}

int PSIKkrtTask::saveResult(void) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::StringBuilder builder(pool);

    for (std::int64_t i = 0; i < result_.size(); i++) {
        builder.Append(result_[i]);
    }

    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);

    std::string col_title =
        psi_type_ == PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
        arrow::field(col_title, arrow::int64())};
    auto schema = std::make_shared<arrow::Schema>(schema_vector);
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});

    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", "psi result");
    std::shared_ptr<CSVDriver> csv_driver =
        std::dynamic_pointer_cast<CSVDriver>(driver);


    if (ValidateDir(result_file_path_)) {
        LOG_ERROR() << "can't access file path: "
                   << result_file_path_;
        return -1;
    }
    int ret = csv_driver->write(table, result_file_path_);

    if (ret != 0) {
        LOG_ERROR() << "Save PSI result to file " << result_file_path_ << " failed.";
        return -1;
    }
    if (this->sync_result_to_server) {
        send_result_to_server();
    }

    LOG_INFO() << "Save PSI result to " << result_file_path_ << ".";
    return 0;
}


int PSIKkrtTask::execute() {
    SCopedTimer timer;
    int ret = _LoadParams(task_param_);
    if (ret) {
        if (role_tag_ == 0) {
            LOG_ERROR() << "Kkrt psi client load task params failed.";
        } else {
            LOG_ERROR() << "Kkrt psi server load task params failed.";
        }
        return ret;
    }
    auto load_params_ts = timer.timeElapse();
    VLOG(5) << "load_params time cost(ms): " << load_params_ts;
    ret = _LoadDataset();
    if (ret) {
        if (role_tag_ == 0) {
            LOG_ERROR() << "Psi client load dataset failed.";
        } else {
            LOG_ERROR() << "Psi server load dataset failed.";
        }
    }
    auto load_dataset_ts = timer.timeElapse();
    auto load_dataset_time_cost = load_dataset_ts - load_params_ts;
    VLOG(5) << "LoadDataset time cost(ms): " << load_dataset_time_cost;
#ifndef __APPLE__
    osuCrypto::IOService ios;
    auto mode = role_tag_ ? EpMode::Server : EpMode::Client;
    std::vector<std::string> addr_info;
    str_split(host_address_, &addr_info, ':');
    std::string server_addr = addr_info[0] + ":1212";
    Endpoint ep(ios, server_addr, mode);
    Channel chl = ep.addChannel();

    if (mode == EpMode::Client) {
        LOG_INFO() << "start recv.";
        auto recv_data_start = timer.timeElapse();
        try {
            _kkrtRecv(chl);
        } catch (std::exception &e) {
            LOG_ERROR() << "Kkrt psi client node task failed:"
	               << e.what();
            chl.close();
            ep.stop();
            ios.stop();
	    return -1;
        }
        auto recv_data_end = timer.timeElapse();
        auto time_cost = recv_data_end - recv_data_start;
        VLOG(5) << "kkrt client process data time cost(ms): " << time_cost;
    } else {
        LOG_INFO() << "start send";
        auto recv_data_start = timer.timeElapse();
        try {
            _kkrtSend(chl);
        } catch (std::exception &e) {
            LOG_ERROR() << "Kkrt psi server node task failed:"
		       << e.what();
            chl.close();
            ep.stop();
            ios.stop();
            return -1;
        }
        auto recv_data_end = timer.timeElapse();
        auto time_cost = recv_data_end - recv_data_start;
        VLOG(5) << "kkrt server process data time cost(ms): " << time_cost;
    }
    chl.close();
    ep.stop();
    ios.stop();
    LOG_INFO() << "kkrt psi run success";

    if (mode == EpMode::Client) {
        auto _start = timer.timeElapse();
        ret = saveResult();
        if (ret) {
            LOG_ERROR() << "Save psi result failed.";
            return -1;
        }
        auto _end = timer.timeElapse();
        auto time_cost = _end - _start;
        VLOG(5) << "kkrt client save result data time cost(ms): " << time_cost;
    }
#endif
    return 0;
}

} // namespace primihub::task
