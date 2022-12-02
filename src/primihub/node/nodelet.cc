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

#include "src/primihub/node/nodelet.h"
#include <vector>
#include <string>

#include "src/primihub/service/dataset/localkv/storage_default.h"
#include "src/primihub/service/dataset/localkv/storage_leveldb.h"
#include "src/primihub/util/util.h"

namespace primihub {
Nodelet::Nodelet(const std::string& config_file_path) {

    // Get p2p address from config file
    YAML::Node config = YAML::LoadFile(config_file_path);
    auto bootstrap_nodes = config["p2p"]["bootstrap_nodes"].as<std::vector<std::string>>();
    p2p_node_stub_ = std::make_shared<primihub::p2p::NodeStub>(std::move(bootstrap_nodes));
    nodelet_addr_ = config["node"].as<std::string>() + ":"
        + config["location"].as<std::string>() + ":"
        + std::to_string(config["grpc_port"].as<uint64_t>());
    std::string addr = config["p2p"]["multi_addr"].as<std::string>();
    p2p_node_stub_->start(addr);

    // Create and start notify service
    notify_server_addr_ = config["notify_server"].as<std::string>();
    std::vector<std::string> server_info;
    str_split(notify_server_addr_, &server_info, ':');
    if (server_info.size() > 1) {
        notify_server_info_.port_ = std::stoi(server_info[1]);
        notify_server_info_.ip_ = config["location"].as<std::string>();
    }
    notify_service_ = std::make_shared<primihub::service::NotifyService>(notify_server_addr_);
    // std::thread notify_service_thread([this]() {
    //     notify_service_->run();
    // });
    // notify_service_thread.detach();
    notify_service_fut = std::async(std::launch::async,
        [this]() {
            notify_service_->run();
        });
    // Wait for p2p node to start
    sleep(3);

    // Create local kv storage defined in config file
    auto localkv_c = config["localkv"]["model"].as<std::string>();
    VLOG(5) << "localkv_c_localkv_c_localkv_c_localkv_c: " << localkv_c ;
    if (localkv_c == "default") {
        local_kv_ = std::make_shared<primihub::service::StorageBackendDefault>();
    } else if (localkv_c == "leveldb") {
        local_kv_ = std::make_shared<primihub::service::StorageBackendLevelDB>(
             config["localkv"]["path"].as<std::string>()
        );
    } else {
       local_kv_ = std::make_shared<primihub::service::StorageBackendDefault>();
    }

    // Init DatasetService with nodelet as stub
    dataset_service_ = std::make_shared<primihub::service::DatasetService>(
        p2p_node_stub_, local_kv_, nodelet_addr_);
    dataset_service_->restoreDatasetFromLocalStorage();

    auto timeout = config["p2p"]["dht_get_value_timeout"].as<unsigned int>();
    loadConifg(config_file_path, timeout);

}

Nodelet::~Nodelet() {
    notify_service_fut.get();
    // TODO stop node and release all protocol/service resources
    this->local_kv_.reset();
}

std::shared_ptr<primihub::service::DatasetService> &Nodelet::getDataService() {
    return dataset_service_;
}

std::string Nodelet::getNodeletAddr() {
    return nodelet_addr_;
}

std::string Nodelet::getNotifyServerAddr() {
    return notify_server_addr_;
}

// Load config file and load default datasets
void Nodelet::loadConifg(const std::string &config_file_path, unsigned int dht_get_value_timeout) {
    dataset_service_->loadDefaultDatasets(config_file_path);
    dataset_service_->setMetaSearchTimeout(dht_get_value_timeout);

    // TODO other service load config
}

} // namespace primihub
