/*
 Copyright 2022 PrimiHub

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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_
#define SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_

#include <glog/logging.h>
#include <string>

#include "src/primihub/common/common.h"
#include "src/primihub/util/util.h"

namespace primihub::service {
using Key = std::string;
using Value = std::string;
using primihub::str_split;

[[deprecated("delete in future")]]
[[maybe_unused]] static retcode DataURLToDetail(const std::string &data_url,
                              std::string &node_id,
                              std::string &node_ip,
                              int &node_port,
                              std::string& dataset_path) {
    // node_id:node_ip:port:data_path
    std::vector<std::string> v;
    primihub::str_split(data_url, &v);
    if ( v.size() != 4 ) {
        LOG(ERROR) << "DataURLToDetail: data_url is invalid: " << data_url;
        return retcode::FAIL;
    }
    node_id = v[0];
    node_ip = v[1];
    node_port = std::stoi(v[2]);
    dataset_path = v[3];
    DLOG(INFO) << "node_id:" << node_id
               << " ip:"<< node_ip
               << " port:"<< node_port << std::endl;
     return retcode::SUCCESS;
}

[[maybe_unused]]
static retcode DataURLToDetail(const std::string &data_url,
                           Node& node_info,
                           std::string& dataset_path) {
    // node_id:node_ip:port:use_tls:data_path
    std::vector<std::string> v;
    primihub::str_split(data_url, &v);
    if (v.size() < 5) {
        LOG(ERROR) << "DataURLToDetail: data_url is invalid: " << data_url;
        return retcode::SUCCESS;
    }
    auto& node_id = v[0];
    auto& node_ip = v[1];
    uint32_t node_port = std::stoi(v[2]);
    bool use_tls = (v[3] == "1" ? true : false);
    node_info = Node(node_id, node_ip, node_port, use_tls);
    if (v.size() == 5) {
        dataset_path = v[4];
    } else {
        dataset_path = v[5];
    }
    VLOG(5) << "DataURLToDetail: " << node_info.to_string();
    return retcode::SUCCESS;
}

// static  std::string Key2Str(const Key &key) {
//      auto s = libp2p::multi::ContentIdentifierCodec::toString(
//           libp2p::multi::ContentIdentifierCodec::decode(key.data).value());
//      return s.value();
// }

// static Key Str2Key(const std::string &str) {
//      auto k = libp2p::multi::ContentIdentifierCodec::encode(
//           libp2p::multi::ContentIdentifierCodec::fromString(str).value());
//      return Key(k.value());
// }

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_
