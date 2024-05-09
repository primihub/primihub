/*
 Copyright 2023 PrimiHub

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
#include "src/primihub/common/config/server_config.h"
#include <unistd.h>
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace primihub {
retcode ServerConfig::initServerConfig(const std::string& config_file) {
  if (is_init_flag.load(std::memory_order::memory_order_relaxed)) {
    return retcode::SUCCESS;
  }
  config_file_ = config_file;
  try {
    YAML::Node root_config = YAML::LoadFile(config_file);
    config_ = root_config.as<primihub::common::NodeConfig>();
    is_init_flag.store(true);
  } catch (std::exception& e) {
    LOG(ERROR) << "load config file: " << config_file << " failed. "
               << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}
Node& ServerConfig::PublicServiceConfig() {
  if (PublicIpProxyEnabled()) {
    return PublicIpProxyConfig();
  } else {
    return getServiceConfig();
  }
}
}  // namespace primihub
