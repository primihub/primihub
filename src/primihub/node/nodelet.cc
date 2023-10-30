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

#include "src/primihub/node/nodelet.h"

namespace primihub {
Nodelet::Nodelet(const std::string &config_file_path,
                 std::shared_ptr<service::DatasetService> service) :
                 config_file_path_(config_file_path),
                 dataset_service_(std::move(service)) {
  auto& server_cfg = ServerConfig::getInstance();
  auto& service_cfg = server_cfg.getServiceConfig();
  nodelet_addr_ = service_cfg.to_string();
  loadConifg(config_file_path, 20);
#ifdef SGX
  auto& cfg = server_cfg.getNodeConfig();
  auto& sgx_config = cfg.tee_conf;
  bool tee_executor = sgx_config.executor;
  bool sgx_enable = sgx_config.sgx_enable;
  std::string RA_SERVER = sgx_config.ra_server_addr;
  std::string CN = service_cfg.id();
  std::string cert_path = sgx_config.cert_path;
  LOG(INFO) << "sgx config enable:" << sgx_enable
      << " ra_server:"<< RA_SERVER
      << " CN: " << CN << " cert_path:" << cert_path;
    // init services
  if (sgx_enable) {
    this->ra_service_ = std::make_shared<sgx::RaTlsService>();
    this->auth_ = std::make_unique<sgx::CertAuth>();
    if (tee_executor) {
      this->tee_executor_ =
          std::make_shared<sgx::TeeEngine>(sgx::TEE_EXECUTOR,
                                           sgx::max_enclave_slice_size, CN);
    } else {
      this->tee_executor_ =
          std::make_shared<sgx::TeeEngine>(sgx::TEE_DATA_SERVER,
                                           sgx::max_enclave_slice_size, CN);
    }
    if (!this->tee_executor_->init(this->auth_.get(), cert_path, RA_SERVER)) {
      LOG(ERROR) << "tee engine init error!";
      return;
    }
    // init handlers
    this->ra_handler_ = std::make_unique<sgx::RaTlsHandlerImpl>();
    this->ra_handler_->init(tee_executor_->get_channel());

    // init services
    this->ra_service_->set_channel(tee_executor_->get_channel());
    this->ra_service_->set_ra_handler(ra_handler_.get());
  } else {
    LOG(INFO) << "sgx enable is :" << sgx_enable;
  }
#endif
}

// Load config file and load default datasets
void Nodelet::loadConifg(const std::string &config_file_path,
                        unsigned int dht_get_value_timeout) {
  dataset_service_->restoreDatasetFromLocalStorage();
  dataset_service_->loadDefaultDatasets(config_file_path);
  dataset_service_->setMetaSearchTimeout(dht_get_value_timeout);
}

} // namespace primihub
