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



#ifndef SRC_PRIMIHUB_NODE_NODELET_H_
#define SRC_PRIMIHUB_NODE_NODELET_H_

#include <string>
#include <future>
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/server_config.h"
#ifdef SGX
#include "sgx/ra/service.h"
#include "sgx/engine/sgx_engine.h"
#endif

namespace primihub {
/**
 * @brief The Nodelet class
 * Provide protocols, services
 *
 */
class Nodelet {
 public:
  explicit Nodelet(const std::string &config_file_path,
                  std::shared_ptr<service::DatasetService> service);
  ~Nodelet() = default;
  std::string getNodeletAddr() const {
    return nodelet_addr_;
  }
  std::shared_ptr<service::DatasetService>& getDataService() {
    return dataset_service_;
  }

#ifdef SGX
  std::shared_ptr<sgx::RaTlsService>& GetRaService() {return ra_service_;}
  std::shared_ptr<sgx::TeeEngine>& GetTeeExecutor() {return tee_executor_;}
#endif

 protected:
  void loadConifg(const std::string &config_file_path, unsigned int timeout);

 private:
  std::string nodelet_addr_;
  std::string config_file_path_;
  std::shared_ptr<service::DatasetService> dataset_service_{nullptr};
#ifdef SGX
  std::shared_ptr<sgx::RaTlsService> ra_service_{nullptr};
  std::shared_ptr<sgx::TeeEngine> tee_executor_{nullptr};
  std::unique_ptr<sgx::CertAuth> auth_{nullptr};
  std::unique_ptr<sgx::RaTlsHandlerImpl> ra_handler_{nullptr};
#endif
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODELET_H_
