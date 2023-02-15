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

#include "src/primihub/cli/reg_cli.h"
#include <fstream>  // std::ifstream
#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <future>
#include <thread>
#include "uuid.h"

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");
ABSL_FLAG(std::string, fid, "test", "fid");
ABSL_FLAG(std::string, data_type, "csv", "data_type");
ABSL_FLAG(std::string, meta_info, "data/server_e.csv", "meta_info");
namespace primihub {

int RegClient::RegDataSet(const std::string& dataset_id,
                const std::string& data_type,
                const std::string& meta_info) {
    NewDatasetRequest request;
    NewDatasetResponse response;
    grpc::ClientContext context;


    LOG(INFO) << "dataset_id: " << dataset_id;
    request.set_fid(dataset_id);
    request.set_driver(data_type);
    request.set_path(meta_info);

    LOG(INFO) << " Register Dataset for dataset_id: " << dataset_id << " "
            << "data_type: " <<  data_type << " "
            << "meta_info: " << meta_info;

    auto status = stub_->NewDataset(&context, request, &response);
    if (status.ok()) {
        LOG(INFO) << "SubmitTask rpc succeeded.";
        if (response.ret_code() == 0) {
            LOG(INFO) << "dataset_url: " << response.dataset_url() << " success";
        } else if (response.ret_code() == 1) {
            LOG(INFO) << "dataset_url: " << response.dataset_url() << " doing";
        } else {
            LOG(INFO) << "dataset_url: " << response.dataset_url() << " error";
        }
    } else {
        LOG(INFO) << "ERROR: " << status.error_message() << "NewDataset rpc failed.";
        return -1;
    }
    return 0;
}

}  // namespace primihub

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    // Set Color logging on
    FLAGS_colorlogtostderr = true;
    // Set stderr logging on
    FLAGS_alsologtostderr = true;
    // Set log output directory
    FLAGS_log_dir = "./log/";
    // Set log max size (MB)
    FLAGS_max_log_size = 10;
    // Set stop logging if disk full on
    FLAGS_stop_logging_if_full_disk = true;

    absl::ParseCommandLine(argc, argv);

    std::vector<std::string> peers;
    peers.push_back(absl::GetFlag(FLAGS_server));
    // std::random_device rd;
    // auto seed_data = std::array<int, std::mt19937::state_size> {};
    // std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    // std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    // std::mt19937 generator(seq);
    // uuids::uuid_random_generator gen{generator};
    // uuids::uuid const id = gen();
    // std::string dataset_id = uuids::to_string(id);
    std::vector<std::tuple<std::string, std::string, std::string>> meta_info = {
        {"psi_client_data", "csv", "data/client_e.csv"},
        {"psi_client_data_db", "sqlite", R"""({"tableName": "psi_client_data", "db_path": "data/client_e.db3"})"""},
        {"psi_client_data_mysql", "mysql",
            R"""({
                "password": "primihub@123",
                "database": "privacy_test1",
                "port": 30306,
                "dbName": "privacy_test1",
                "host": "192.168.99.13",
                "type": "mysql",
                "username": "primihub",
                "tableName": "sys_user"}
                )"""},
    };
    for (auto peer : peers) {
        LOG(INFO) << "SDK SubmitTask to: " << peer;
        auto channel = grpc::CreateChannel(peer, grpc::InsecureChannelCredentials());
        primihub::RegClient client(channel);
        auto _start = std::chrono::high_resolution_clock::now();
        for (const auto meta :  meta_info) {
            auto& fid = std::get<0>(meta);
            auto& data_type = std::get<1>(meta);
            auto& data_path = std::get<2>(meta);
            auto ret = client.RegDataSet(fid, data_type, data_path);
            if (ret) {
                LOG(ERROR) << "RegDataSet: " << fid << " data type: " <<  data_type << " "
                        << "meta_info: " << data_path << " failed";
            }
        }

        auto _end = std::chrono::high_resolution_clock::now();
        auto time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start).count();
        LOG(INFO) << "SubmitTask time cost(ms): " << time_cost;
        break;

    }

    return 0;
}
