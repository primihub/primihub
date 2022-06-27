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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_
#define SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_

#include <glog/logging.h>
#include <string>
#include <glog/logging.h>

#include <libp2p/multi/content_identifier_codec.hpp>

#include "src/primihub/util/util.h"
#include "src/primihub/service/dataset/storage_backend.h"

namespace primihub::service {

using primihub::str_split;

static void DataURLToDetail(const std::string &data_url, 
                              std::string &node_id,
                              std::string &node_ip,
                              int &node_port,
                              std::string& dataset_path) {
    std::vector<std::string> v;
    primihub::str_split(data_url, &v);
    node_id = v[0];
    node_ip = v[1];
    node_port = std::stoi(v[2]);
    dataset_path = v[3];
    DLOG(INFO) << "node_id:" << node_id 
               << " ip:"<< node_ip   
               << " port:"<< node_port << std::endl;
}

static  std::string Key2Str(const Key &key) {
     auto s = libp2p::multi::ContentIdentifierCodec::toString(
          libp2p::multi::ContentIdentifierCodec::decode(key.data).value());
     return s.value();
}

static Key Str2Key(const std::string &str) {
     auto k = libp2p::multi::ContentIdentifierCodec::encode(
          libp2p::multi::ContentIdentifierCodec::fromString(str).value());
     return Key(k.value());
}

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_DATASET_UTIL_HPP_
