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

#include <vector>
#include <string>

#include <yaml-cpp/yaml.h>

namespace primihub::common {

typedef struct Dataset {
    std::string description;
    std::string model;
    std::string source;
} Dataset;

typedef struct LocalKV {
    std::string model;
    std::string path;
} LocalKV;

typedef struct P2P {
    std::vector<std::string> bootstrap_nodes;
    std::string multi_addr;
};
typedef struct NodeConfig {
    std::string node;
    std::string location;
    uint64_t grpc_port;
    std::vector<Dataset> datasets;
    P2P p2p;
    LocalKV localkv;
} NodeConfig;
} // namespace primihub::common

namespace YAML {
    
using primihub::common::Dataset;
using primihub::common::LocalKV;
using primihub::common::P2P;
using primihub::common::NodeConfig;

template <> struct convert<Dataset> {
    static Node encode(const Dataset &ds) {
        Node node;
        node["description"] = ds.description;
        node["model"] = ds.model;
        node["source"] = ds.source;
        return node;
    }

    static bool decode(const Node &node, Dataset &ds) {
        ds.description  = node["description"].as<std::string>();
        ds.model = node["model"].as<std::string>();
        ds.source = node["source"].as<std::string>();
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

    static bool decode(const Node &node, LocalKV &lkv) {
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

    static bool decode(const Node &node, P2P &p2p) {
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
        node["node"] = nc.node;
        node["location"] = nc.location;
        node["grpc_port"] = nc.grpc_port;
        node["datasets"] = nc.datasets;
        node["localkv"] = nc.localkv;
        node["p2p"] = nc.p2p;

        return node;
    }

    static bool decode(const Node &node, NodeConfig &nc) {
        nc.node = node["node"].as<std::string>();
        nc.location = node["location"].as<std::string>();
        nc.grpc_port = node["grpc_port"].as<uint64_t>();
        auto datasets = node["datasets"].as<YAML::Node>();
        for (int i = 0; i < datasets.size(); i++) {
            auto dataset = datasets[i].as<Dataset>();
            nc.datasets.push_back(dataset);
        }
        nc.localkv = node["localkv"].as<LocalKV>();
        nc.p2p = node["p2p"].as<P2P>();

        return true;
    }
};
} // namespace YAML

#endif // SRC_PRIMIHUB_COMMON_CONFIG_CONFIG_H_
