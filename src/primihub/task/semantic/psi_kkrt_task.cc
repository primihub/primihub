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

#if defined(__linux__) && defined(__x86_64__)
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
#include "src/primihub/util/endian_util.h"
#include <glog/logging.h>
#include <chrono>

#if defined(__linux__) && defined(__x86_64__)
#include "libPSI/PSI/Kkrt/KkrtPsiSender.h"

#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h"
#include "libOTe/NChooseOne/NcoOtExt.h"
#endif

#include <numeric>


#if defined(__linux__) && defined(__x86_64__)
using namespace osuCrypto;
#endif
using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
using arrow::Int64Builder;
using primihub::rpc::VarType;
using std::map;

namespace primihub::task {

PSIKkrtTask::PSIKkrtTask(const TaskParam *task_param,
                         std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {}

retcode PSIKkrtTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();
    auto param_map_it = param_map.find("serverAddress");
    std::string role = task.role();
    int32_t rank = task.rank();
    const auto& party_datasets = task.party_datasets();
    auto it = party_datasets.find(role);
    if (it == party_datasets.end()) {
      LOG(ERROR) << "no dataset is found for role: " << role;
      return retcode::FAIL;
    }
    auto datasets = it->second;
    dataset_id_ = datasets.item(0);
    if (role == ROLE_CLIENT) {
        //role_tag_ = EpMode::Client;
        role_tag_ = 0;
        try {
            VLOG(5) << "parse paramerter";
            auto it = param_map.find("sync_result_to_server");
            if (it != param_map.end()) {
                sync_result_to_server = it->second.value_int32() > 0;
                VLOG(5) << "sync_result_to_server: " << sync_result_to_server;
            }
            it = param_map.find("server_outputFullFilname");
            if (it != param_map.end()) {
                server_result_path = it->second.value_string();
                VLOG(5) << "server_outputFullFilname: " << server_result_path;
            }
	          psi_type_ = param_map["psiType"].value_int32();
            result_file_path_ = param_map["outputFullFilename"].value_string();
            auto index_it = param_map.find("clientIndex");
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
        } catch (std::exception &e) {
            LOG(ERROR) << "Failed to load params: " << e.what();
            return retcode::FAIL;
        }
    } else {
        //role_tag_ = EpMode::Server;
        role_tag_ = 1;
        try {
            // data_index_ = param_map["serverIndex"].value_int32();
            auto index_it = param_map.find("serverIndex");
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
            auto it = param_map.find("sync_result_to_server");
            if (it != param_map.end()) {
                sync_result_to_server = it->second.value_int32() > 0;
                VLOG(5) << "sync_result_to_server: " << sync_result_to_server;
            }
            it = param_map.find("server_outputFullFilname");
            if (it != param_map.end()) {
                server_result_path = it->second.value_string();
                VLOG(5) << "server_outputFullFilname: " << server_result_path;
            }
        } catch (std::exception &e) {
            LOG(ERROR) << "Failed to load params: " << e.what();
            return retcode::FAIL;
        }
    }
    const auto& party_info = task.party_access_info();
    std::string peer_role = role == ROLE_CLIENT ? ROLE_SERVER : ROLE_CLIENT;
    auto party_it = party_info.find(peer_role);
    if (party_it == party_info.end()) {
      LOG(ERROR) << "get peer node access info failed for role: " << role;
      return retcode::FAIL;
    }
    auto& peer_node_list = party_it->second;
    auto& pb_peer_node = peer_node_list.node(0);
    pbNode2Node(pb_peer_node, &peer_node);
    VLOG(5) << "peer_address_: " << peer_node.to_string();
    return retcode::SUCCESS;
}

retcode PSIKkrtTask::_LoadDataset(void) {
    auto driver = this->getDatasetService()->getDriver(this->dataset_id_);
    if (driver == nullptr) {
        LOG(ERROR) << "get driver for data set: " << this->dataset_id_ << " failed";
        return retcode::FAIL;
    }
    auto ret = LoadDatasetInternal(driver, data_index_, elements_);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "Load dataset for psi server failed.";
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

#if defined(__linux__) && defined(__x86_64__)
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

    //LOG(INFO) << "send size:" << dest[0];
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
    //LOG(INFO) << "client step 1";

    //recvPSIs.setTimer(gTimer);
    chl.recv(dummy, 1);
    //LOG(INFO) << "client step 2";
    //gTimer.reset();
    chl.asyncSend(dummy, 1);
    //LOG(INFO) << "client step 3";
    //Timer timer;
    //auto start = timer.setTimePoint("start");
    recvPSIs.init(sendSize, recvSize, 40, chl, otRecv, prng.get<block>());
    //LOG(INFO) << "client step 4";
    //auto mid = timer.setTimePoint("init");
    recvPSIs.sendInput(recvSet, chl);
    //LOG(INFO) << "client step 5";
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

    //LOG(INFO) << "recv size:" << dest[0];
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
    //LOG(INFO) << "server step 1";

    sendPSIs.setTimer(gTimer);
    chl.asyncSend(dummy, 1);
    //LOG(INFO) << "server step 2";
    chl.recv(dummy, 1);
    //LOG(INFO) << "server step 3";
    sendPSIs.init(sendSize, recvSize, 40, chl, otSend, prng.get<block>());
    //LOG(INFO) << "server step 4";
    sendPSIs.sendInput(set, chl);
    //LOG(INFO) << "server step 5";

    u64 dataSent = chl.getTotalDataSent();
    //LOG(INFO) << "server step 6";

    chl.resetStats();
    //LOG(INFO) << "server step 7";
}

retcode PSIKkrtTask::_GetIntsection(KkrtPsiReceiver &receiver) {
    /*for (auto pos : receiver.mIntersection) {
        LOG(INFO) << pos;
    }*/

    if (psi_type_ == rpc::PsiType::DIFFERENCE) {
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
    return retcode::SUCCESS;
}
#endif

retcode PSIKkrtTask::broadcastResultToServer() {
    retcode ret{retcode::SUCCESS};
#if defined(__linux__) && defined(__x86_64__)
    VLOG(5) << "broadcast_result_to_server";
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
    ret = this->send(this->key, peer_node, result_str);
    VLOG(5) << "send result to server success";
#endif
    return ret;
}

retcode PSIKkrtTask::saveResult() {
    std::string col_title =
        psi_type_ == rpc::PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    auto ret = saveDataToCSVFile(result_, result_file_path_, col_title);
    if (ret != retcode::SUCCESS) { return ret;}
    if (this->sync_result_to_server) {
        ret = broadcastResultToServer();
    }
    return ret;
}

int PSIKkrtTask::execute() {
    SCopedTimer timer;
    auto ret = _LoadParams(task_param_);
    if (ret != retcode::SUCCESS) {
        if (role_tag_ == 0) {
            LOG(ERROR) << "Kkrt psi client load task params failed.";
        } else {
            LOG(ERROR) << "Kkrt psi server load task params failed.";
        }
        return -1;
    }
    auto load_params_ts = timer.timeElapse();
    VLOG(5) << "load_params time cost(ms): " << load_params_ts;
    ret = _LoadDataset();
    if (ret != retcode::SUCCESS) {
        if (role_tag_ == 0) {
            LOG(ERROR) << "Psi client load dataset failed.";
        } else {
            LOG(ERROR) << "Psi server load dataset failed.";
        }
        return -1;
    }
    auto load_dataset_ts = timer.timeElapse();
    auto load_dataset_time_cost = load_dataset_ts - load_params_ts;
    VLOG(5) << "LoadDataset time cost(ms): " << load_dataset_time_cost;
#if defined(__linux__) && defined(__x86_64__)
    osuCrypto::IOService ios;
    auto mode = role_tag_ ? EpMode::Server : EpMode::Client;
    getAvailablePort(&data_port);
    VLOG(5) << "getAvailablePort: " << data_port;
    exchangeDataPort();
    std::string server_addr;
    if (mode == EpMode::Client) {
        server_addr = peer_node.ip() + ":" + std::to_string(peer_data_port);
    } else {
        server_addr = peer_node.ip() + ":" + std::to_string(data_port);
    }
    VLOG(5) << "server_addr: " << server_addr;
    Endpoint ep(ios, server_addr, mode);
    Channel chl = ep.addChannel();

    if (mode == EpMode::Client) {
        LOG(INFO) << "start recv.";
        auto recv_data_start = timer.timeElapse();
        try {
            _kkrtRecv(chl);
        } catch (std::exception &e) {
            LOG(ERROR) << "Kkrt psi client node task failed:"
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
        LOG(INFO) << "start send";
        auto recv_data_start = timer.timeElapse();
        try {
            _kkrtSend(chl);
        } catch (std::exception &e) {
            LOG(ERROR) << "Kkrt psi server node task failed:"
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
    LOG(INFO) << "kkrt psi run success";

    if (mode == EpMode::Client) {
        auto _start = timer.timeElapse();
        ret = saveResult();
        if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "Save psi result failed.";
            return -1;
        }
        auto _end = timer.timeElapse();
        auto time_cost = _end - _start;
        VLOG(5) << "kkrt client save result data time cost(ms): " << time_cost;
    } else if (mode == EpMode::Server) {
        if (sync_result_to_server) {
            recvIntersectionData();
        }
    }
#endif
    return 0;
}

retcode PSIKkrtTask::recvIntersectionData() {
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
    return saveDataToCSVFile(psi_result, server_result_path, col_title);
}

retcode PSIKkrtTask::exchangeDataPort() {
    std::string data_port_info_str;
    rpc::Params data_port_params;
    auto param_map = data_port_params.mutable_param_map();
    // dataset size
    rpc::ParamValue pv_data_port;
    pv_data_port.set_var_type(rpc::VarType::INT32);
    pv_data_port.set_value_int32(this->data_port);
    pv_data_port.set_is_array(false);
    (*param_map)["data_port"] = std::move(pv_data_port);
    bool success = data_port_params.SerializeToString(&data_port_info_str);
    if (!success) {
        LOG(ERROR) << "serialize data port info failed";
        return retcode::FAIL;
    }
    auto channel = this->getTaskContext().getLinkContext()->getChannel(peer_node);
    auto ret = channel->send(this->key, data_port_info_str);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send data port info to peer: [" << peer_node.to_string()
            << "] failed";
        return ret;
    }
    // // wait for peer data port info
    VLOG(5) << "begin to recv data port info from peer.........";
    std::string recv_data_port_info_str;
    auto& recv_queue = this->getTaskContext().getRecvQueue(this->key);
    recv_queue.wait_and_pop(recv_data_port_info_str);
    rpc::Params recv_data_port_info;
    recv_data_port_info.ParseFromString(recv_data_port_info_str);
    const auto& recv_param_map = recv_data_port_info.param_map();
    auto it = recv_param_map.find("data_port");
    if (it == recv_param_map.end()) {
        CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, "data_port info does not find");
    }
    peer_data_port = it->second.value_int32();
    VLOG(5) << "peer_data_port: " << peer_data_port;
    return retcode::SUCCESS;
}

} // namespace primihub::task
