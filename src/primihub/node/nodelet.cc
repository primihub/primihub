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
#include "src/primihub/service/dataset/localkv/storage_default.h"

namespace primihub {
Nodelet::Nodelet(const std::string& config_file_path) {
    
    // Get p2p address from config file
    YAML::Node config = YAML::LoadFile(config_file_path);
    auto bootstrap_nodes = config["p2p"]["bootstrap_nodes"].as<std::vector<std::string>>();
    p2p_node_stub_ = std::make_shared<primihub::p2p::NodeStub>(std::move(bootstrap_nodes));
    std::string addr = config["p2p"]["multi_addr"].as<std::string>();
    p2p_node_stub_->start(addr);
    sleep(10);
    local_kv_ = std::make_shared<primihub::service::StorageBackendDefault>();

    // Init DatasetService with nodelet as stub
    dataset_service_ = std::make_shared<primihub::service::DatasetService>(
        p2p_node_stub_, local_kv_);

    loadConifg(config_file_path);
    
}

Nodelet::~Nodelet() {
    // TODO stop node and release all protocol/service resources
}

std::shared_ptr<primihub::service::DatasetService> &Nodelet::getDataService() {
    return dataset_service_;
}

// Load config file and load default datasets
void Nodelet::loadConifg(const std::string &config_file_path) {
    dataset_service_->loadDefaultDatasets(config_file_path);
    // TODO other service load config
}

} // namespace primihub
