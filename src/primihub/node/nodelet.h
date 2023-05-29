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
#include <future>
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/common/common.h"
#include "src/primihub/node/server_config.h"

namespace primihub {
/**
 * @brief The Nodelet class
 * Provide protocols, services
 *
 */
class Nodelet {
 public:
  explicit Nodelet(const std::string &config_file_path,
                  std::shared_ptr<service::DatasetService> service) :
      config_file_path_(config_file_path), dataset_service_(std::move(service)) {
    auto& server_cfg = ServerConfig::getInstance();
    auto& service_cfg = server_cfg.getServiceConfig();
    nodelet_addr_ = service_cfg.to_string();
    loadConifg(config_file_path, 20);
  }

  ~Nodelet() = default;
  std::string getNodeletAddr() const {return nodelet_addr_;}
  std::shared_ptr<service::DatasetService>& getDataService() {return dataset_service_;}

 private:
  void loadConifg(const std::string &config_file_path, unsigned int timeout);
  std::string nodelet_addr_;
  std::string config_file_path_;
  std::shared_ptr<service::DatasetService> dataset_service_{nullptr};
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODELET_H_
