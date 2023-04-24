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
#include "src/primihub/common/defines.h"
#include "src/primihub/node/server_config.h"

namespace primihub {
Nodelet::Nodelet(const std::string& config_file_path) {
    auto& server_cfg = ServerConfig::getInstance();
    // Get p2p address from config file
    auto& service_cfg = server_cfg.getServiceConfig();
    auto& cfg = server_cfg.getNodeConfig();
    auto& p2p_cfg = cfg.p2p;
    auto& bootstrap_nodes = p2p_cfg.bootstrap_nodes;
    p2p_node_stub_ = std::make_shared<primihub::p2p::NodeStub>(std::move(bootstrap_nodes));
    nodelet_addr_ = service_cfg.to_string();
    auto& redis_cfg = server_cfg.getRedisConfig();
    if (redis_cfg.use_redis) {
        // This is a dummy stub, not used in redis meta service.
        p2p_node_stub_ = std::make_shared<primihub::p2p::NodeStub>(bootstrap_nodes);
        LOG(INFO) << "Use redis meta service instead of p2p network.";
    } else {
        p2p_node_stub_ =
            std::make_shared<primihub::p2p::NodeStub>(std::move(bootstrap_nodes));
        std::string addr = cfg.p2p.multi_addr;
        p2p_node_stub_->start(addr);
    }

    // Create and start notify service
    notify_server_addr_ = cfg.notify_server;
    std::vector<std::string> server_info;
    str_split(notify_server_addr_, &server_info, ':');
    if (server_info.size() > 1) {
        notify_server_info_.port_ = std::stoi(server_info[1]);
        notify_server_info_.ip_ = service_cfg.ip();
    }
    // notify_service_ = std::make_shared<primihub::service::NotifyService>(notify_server_addr_);
    // // std::thread notify_service_thread([this]() {
    // //     notify_service_->run();
    // // });
    // // notify_service_thread.detach();
    // notify_service_fut = std::async(std::launch::async,
    //     [this]() {
    //         SET_THREAD_NAME("notifyServer");
    //         notify_service_->run();
    //     });
    // Wait for p2p node to start
    // sleep(3);

    // Create local kv storage defined in config file
    auto& localkv = cfg.localkv;
    std::string localkv_c = localkv.model;
    VLOG(5) << "localkv_c_localkv_c_localkv_c_localkv_c: " << localkv_c ;
    if (localkv_c == "default") {
        local_kv_ = std::make_shared<primihub::service::StorageBackendDefault>();
    } else if (localkv_c == "leveldb") {
        local_kv_ = std::make_shared<primihub::service::StorageBackendLevelDB>(
            localkv.path);
    } else {
       local_kv_ = std::make_shared<primihub::service::StorageBackendDefault>();
    }

    if (redis_cfg.use_redis) {
        meta_service_ =
            std::make_shared<primihub::service::RedisDatasetMetaService>(
                redis_cfg.redis_addr, redis_cfg.redis_password, local_kv_, p2p_node_stub_);
        dataset_service_ = std::make_shared<primihub::service::DatasetService>(
            meta_service_, nodelet_addr_);

        LOG(INFO) << "Init redis meta service and dataset service finish.";
    } else {
        meta_service_ = std::make_shared<primihub::service::DatasetMetaService>(
            p2p_node_stub_, local_kv_);
        dataset_service_ = std::make_shared<primihub::service::DatasetService>(
            meta_service_, nodelet_addr_);

        LOG(INFO) << "Init p2p meta service and dataset service finish.";
    }

    dataset_service_->restoreDatasetFromLocalStorage();
    // auto timeout = config["p2p"]["dht_get_value_timeout"].as<unsigned int>();
    auto timeout = p2p_cfg.dht_get_value_timeout;
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
