/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SRC_PRIMIHUB_COMMON_CONFIG_SERVER_CONFIG_H_
#define SRC_PRIMIHUB_COMMON_CONFIG_SERVER_CONFIG_H_
#include <glog/logging.h>
#include <string>
#include <atomic>
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"

namespace primihub {
using primihub::common::CertificateConfig;
using primihub::common::RedisConfig;
using primihub::common::NodeConfig;
class ServerConfig {
 public:
  ServerConfig() = default;
  static ServerConfig& getInstance() {
    static ServerConfig ins;
    return ins;
  }
  retcode initServerConfig(const std::string& config_file);
  Node& getServiceConfig() { return config_.server_config;}
  bool PublicIpProxyEnabled() {return config_.public_ip_proxy_enable;}
  Node& PublicIpProxyConfig() {return config_.public_ip_proxy_config.host_info;}
  CertificateConfig& getCertificateConfig() {return config_.cert_config;}
  NodeConfig& getNodeConfig() {return config_;}
  std::string getConfigFile() {return config_file_;}
  Node& ProxyServerCfg() {return config_.proxy_server_cfg.host_info;}
  std::string& StoragePath() {return config_.storage_info.path;}
  Node& PublicServiceConfig();
  bool IsInitFlag() {return is_init_flag.load(std::memory_order::memory_order_relaxed);}

 protected:
  ServerConfig(const ServerConfig&) = default;
  ServerConfig(ServerConfig&&) = default;
  ServerConfig& operator=(const ServerConfig&) = default;
  ServerConfig& operator=(ServerConfig&&) = default;

 private:
  std::atomic<bool> is_init_flag{false};
  NodeConfig config_;
  std::string config_file_;
};
}  // namespace primihub

#endif  // SRC_PRIMIHUB_COMMON_CONFIG_SERVER_CONFIG_H_
