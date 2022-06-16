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



#ifndef SRC_PRIMIHUB_NODE_NODELET_H_
#define SRC_PRIMIHUB_NODE_NODELET_H_

#include <string>

#include "src/primihub/p2p/node_stub.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/storage_backend.h"

namespace primihub {

/**
 * @brief The Nodelet class
 * Provide protocols, services
 *
 */
class Nodelet {
  public:
    explicit Nodelet(const std::string &config_file_path);
    ~Nodelet();
    std::shared_ptr<primihub::service::DatasetService> &getDataService();
    std::string getNodeletAddr();

  private:
    void loadConifg(const std::string &config_file_path, unsigned int timeout);

    std::shared_ptr<primihub::p2p::NodeStub> p2p_node_stub_;
    std::shared_ptr<primihub::service::StorageBackend> local_kv_;
    // protocols, servcies, etc.
    std::shared_ptr<primihub::service::DatasetService> dataset_service_;
    std::string nodelet_addr_;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODELET_H_
