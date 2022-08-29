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
            data_index_ = param_map["clientIndex"].value_int32();
	    psi_type_ = param_map["psiType"].value_int32();
            dataset_path_ = param_map["clientData"].value_string();
            result_file_path_ = param_map["outputFullFilename"].value_string();
            host_address_ = param_map["serverAddress"].value_string();
        } catch (std::exception &e) {
            LOG(ERROR) << "Failed to load params: " << e.what();
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
            LOG(ERROR) << "Failed to load params: " << e.what();
            return -1;
        }
    }

    return 0;
}

int PSIKkrtTask::_LoadDatasetFromCSV(std::string &filename, int data_col,
                                        std::vector <std::string> &col_array) {
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
    return 0;
}

int PSIKkrtTask::_LoadDataset(void) {
    int ret = _LoadDatasetFromCSV(dataset_path_, data_index_, elements_);
    if (ret) {
        LOG(ERROR) << "Load dataset for psi client failed.";
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

    //LOG(INFO) << "send size:" << dest[0];
    sendSize = dest[0];
    std::vector<block> sendSet(sendSize), recvSet(recvSize);

    u8 block_size = sizeof(block);
    RandomOracle  sha1(block_size);
    u8 hash_dest[block_size];
    for (u64 i = 0; i < recvSize; ++i) {
        sha1.Update((u8 *)elements_[i].data(), block_size);
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
        sha1.Update((u8 *)elements_[i].data(), block_size);
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

int PSIKkrtTask::_GetIntsection(KkrtPsiReceiver &receiver) {
    /*for (auto pos : receiver.mIntersection) {
        LOG(INFO) << pos;
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
        LOG(ERROR) << "can't access file path: "
                   << result_file_path_;
        return -1;
    }
    int ret = csv_driver->write(table, result_file_path_);

    if (ret != 0) {
        LOG(ERROR) << "Save PSI result to file " << result_file_path_ << " failed.";
        return -1;
    }

    LOG(INFO) << "Save PSI result to " << result_file_path_ << "."; 
    return 0;
}


int PSIKkrtTask::execute() {
    int ret = _LoadParams(task_param_);
    if (ret) {
        if (role_tag_ == 0) {
            LOG(ERROR) << "Kkrt psi client load task params failed.";
        } else {
            LOG(ERROR) << "Kkrt psi server load task params failed.";
        }
        return ret;
    }

    ret = _LoadDataset();
    if (ret) {
        if (role_tag_ == 0) {
            LOG(ERROR) << "Psi client load dataset failed.";
        } else {
            LOG(ERROR) << "Psi server load dataset failed.";
        }
    }

#ifndef __APPLE__
    osuCrypto::IOService ios;
    auto mode = role_tag_ ? EpMode::Server : EpMode::Client;
    std::vector<std::string> addr_info;
    str_split(host_address_, &addr_info, ':');
    std::string server_addr = addr_info[0] + ":1212";
    Endpoint ep(ios, server_addr, mode);
    Channel chl = ep.addChannel();
    
    if (mode == EpMode::Client) {
        LOG(INFO) << "start recv.";
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
    } else {
        LOG(INFO) << "start send";
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
    }
    chl.close();
    ep.stop();
    ios.stop();
    LOG(INFO) << "kkrt psi run success";

    if (mode == EpMode::Client) {
        ret = saveResult();
        if (ret) {
            LOG(ERROR) << "Save psi result failed.";
            return -1;
        }
    }
#endif
    return 0;
}

} // namespace primihub::task
