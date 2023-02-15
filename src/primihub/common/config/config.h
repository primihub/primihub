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

#ifndef SRC_PRIMIHUB_COMMON_CONFIG_CONFIG_H_
#define SRC_PRIMIHUB_COMMON_CONFIG_CONFIG_H_
#include <yaml-cpp/yaml.h>
#include <glog/logging.h>
#include <vector>
#include <string>
#include "src/primihub/common/common.h"

namespace primihub::common {

struct DBInfo {
    std::string db_name;
    std::string table_name;
};

struct Dataset {
    std::string description;
    std::string model;
    std::string source;
    DBInfo db_info;
};

struct LocalKV {
    std::string model;
    std::string path;
};

struct P2P {
    std::vector<std::string> bootstrap_nodes;
    std::string multi_addr;
    uint32_t dht_get_value_timeout{20};
};

struct RedisConfig {
    bool use_redis{false};
    std::string redis_addr;
    std::string redis_password;
};

class CertificateConfig {
 public:
    CertificateConfig() = default;
    CertificateConfig(const std::string& root_ca_path,
        const std::string& key_path, const std::string& cert_path)
        :root_ca_path_(root_ca_path), key_path_(key_path), cert_path_(cert_path) {
        auto ret = initCertificateConfig();
        if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "initCertificationConfig failed, " << " "
                << "root_ca_path: " << root_ca_path_ << " "
                << "key_path: " << key_path_ << " "
                << "cert_path: " << cert_path_;
        }
    }

    retcode init(const std::string& root_ca_path,
            const std::string& key_path, const std::string& cert_path) {
        setRootCAPath(root_ca_path);
        setKeyPath(key_path);
        setCertPath(cert_path);
        return initCertificateConfig();
    }

    retcode init() {
        return initCertificateConfig();
    }

    inline std::string& rootCAContent() {return root_ca_content_;}
    inline std::string& keyContent() { return key_content_;}
    inline std::string& certContent() {return cert_content_;}
    inline std::string rootCAPath() const {return root_ca_path_;}
    inline std::string keyPath() const { return key_path_;}
    inline std::string certPath() const {return cert_path_;}
    inline std::string setRootCAPath(const std::string& file_path) {
        return root_ca_path_ = file_path;
    }
    inline std::string setKeyPath(const std::string& file_path) {
        return key_path_ = file_path;
    }
    inline std::string setCertPath(const std::string& file_path) {
        return cert_path_ = file_path;
    }

 protected:
    retcode initCertificateConfig();

 private:
    std::string root_ca_content_;
    std::string key_content_;
    std::string cert_content_;
    std::string root_ca_path_;
    std::string key_path_;
    std::string cert_path_;
};

struct NodeConfig {
    Node server_config;
    CertificateConfig cert_config;
    std::vector<Dataset> datasets;
    RedisConfig redis_cfg;
    P2P p2p;
    LocalKV localkv;
    std::string notify_server;
};

}  // namespace primihub::common

namespace YAML {
using primihub::common::Dataset;
using primihub::common::LocalKV;
using primihub::common::P2P;
using primihub::common::NodeConfig;
using primihub::common::DBInfo;
using ServerConfig = primihub::Node;
using primihub::common::CertificateConfig;
using primihub::common::RedisConfig;

template <> struct convert<RedisConfig> {
    static Node encode(const RedisConfig &redis_cfg) {
        Node node;
        node["redis_addr"] = redis_cfg.redis_addr;
        node["use_redis"] = redis_cfg.use_redis;
        node["redis_password"] = redis_cfg.redis_password;
        return node;
    }

    static bool decode(const Node &node, RedisConfig &redis_cfg) {   // NOLINT
        redis_cfg.redis_addr = node["redis_addr"].as<std::string>();
        redis_cfg.redis_password = node["redis_password"].as<std::string>();
        redis_cfg.use_redis = node["use_redis"].as<bool>();
        return true;
    }
};

template <> struct convert<CertificateConfig> {
    static Node encode(const CertificateConfig &cert_cfg) {
        Node node;
        node["root_ca"] = cert_cfg.rootCAPath();
        node["key"] = cert_cfg.keyPath();
        node["cert"] = cert_cfg.certPath();
        return node;
    }

    static bool decode(const Node &node, CertificateConfig &cert_cfg) {   // NOLINT
        cert_cfg.setRootCAPath(node["root_ca"].as<std::string>());
        cert_cfg.setKeyPath(node["key"].as<std::string>());
        cert_cfg.setCertPath(node["cert"].as<std::string>());
        auto ret = cert_cfg.init();
        if (ret != primihub::retcode::SUCCESS) {
            return false;
        }
        return true;
    }
};

template <> struct convert<DBInfo> {
    static Node encode(const DBInfo &db_info) {
        Node node;
        node["db_name"] = db_info.db_name;
        node["table_name"] = db_info.table_name;
        return node;
    }

    static bool decode(const Node &node, DBInfo &ds) {   // NOLINT
        ds.db_name  = node["db_name"].as<std::string>();
        ds.table_name = node["table_name"].as<std::string>();
        return true;
    }
};

template <> struct convert<Dataset> {
    static Node encode(const Dataset &ds) {
        Node node;
        node["description"] = ds.description;
        node["model"] = ds.model;
        node["source"] = ds.source;
        if (ds.model == "sqlite") {
            node["db_info"] = ds.db_info;
        }
        return node;
    }

    static bool decode(const Node &node, Dataset &ds) {     // NOLINT
        ds.description  = node["description"].as<std::string>();
        ds.model = node["model"].as<std::string>();
        ds.source = node["source"].as<std::string>();
        if (ds.model == "sqlite") {
            ds.db_info = node["db_info"].as<DBInfo>();
        }
        return true;
    }
};


template <> struct convert<LocalKV> {
    static Node encode(const LocalKV &lkv) {
        Node node;
        node["model"] = lkv.model;
        node["path"] = lkv.path;
        return node;
    }

    static bool decode(const Node &node, LocalKV &lkv) {    // NOLINT
        lkv.model = node["model"].as<std::string>();
        lkv.path = node["path"].as<std::string>();
        return true;
    }
};


template <> struct convert<P2P> {
    static Node encode(const P2P &p2p) {
        Node node;
        node["multi_addr"] = p2p.multi_addr;
        node["bootstrap_nodes"] = p2p.bootstrap_nodes;
        return node;
    }

    static bool decode(const Node &node, P2P &p2p) {            // NOLINT
        for (auto &bootstrap_node : node["bootstrap_nodes"]) {
            p2p.bootstrap_nodes.push_back(bootstrap_node.as<std::string>());
        }
        p2p.multi_addr  = node["multi_addr"].as<std::string>();
        return true;
    }
};

template <> struct convert<NodeConfig> {
    static Node encode(const NodeConfig &nc) {
        Node node;
        node["node"] = nc.server_config.id();
        node["location"] = nc.server_config.ip();
        node["grpc_port"] = nc.server_config.port();
        node["use_tls"] = nc.server_config.use_tls();
        node["redis_meta_service"] = nc.redis_cfg;
        // node["datasets"] = nc.datasets;
        node["localkv"] = nc.localkv;
        node["p2p"] = nc.p2p;
        node["notify_server"] = nc.notify_server;
        return node;
    }

    static bool decode(const Node &node, NodeConfig &nc) {      // NOLINT
        nc.server_config.id_ = node["node"].as<std::string>();
        nc.server_config.ip_ = node["location"].as<std::string>();
        nc.server_config.port_ = node["grpc_port"].as<uint32_t>();
        if (node["use_tls"]) {
            nc.server_config.use_tls_ = node["use_tls"].as<bool>();
        }
        if (node["certificate"]) {
            nc.cert_config = node["certificate"].as<CertificateConfig>();
        }
        if (node["redis_meta_service"]) {
            nc.redis_cfg = node["redis_meta_service"].as<RedisConfig>();
        }
        // datasets may be too much, so do not parse from here
        // auto datasets = node["datasets"].as<YAML::Node>();
        // for (size_t i = 0; i < datasets.size(); i++) {
        //     auto dataset = datasets[i].as<Dataset>();
        //     nc.datasets.push_back(dataset);
        // }
        nc.localkv = node["localkv"].as<LocalKV>();
        nc.p2p = node["p2p"].as<P2P>();
        nc.notify_server = node["notify_server"].as<std::string>();

        return true;
    }
};
}  // namespace YAML

#endif  // SRC_PRIMIHUB_COMMON_CONFIG_CONFIG_H_
