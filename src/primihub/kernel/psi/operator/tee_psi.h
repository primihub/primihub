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

#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_TEE_PSI_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_TEE_PSI_H_
#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "src/primihub/common/common.h"
#include "sgx/ra/service.h"
#include "sgx/engine/sgx_engine.h"

namespace primihub::psi {
class TeePsiOperator : public BasePsiOperator {
 public:
  TeePsiOperator(const Options& options, sgx::TeeEngine* executor) :
      BasePsiOperator(options), executor_(executor) {
    extra_info_.emplace_back("1");
    auto& party_info = this->options_.party_info;
    auto it = party_info.find(PartyName());
    if (it != party_info.end()) {
      this->options_.proxy_node = it->second;
    }
  }
  retcode OnExecute(const std::vector<std::string>& input,
                    std::vector<std::string>* result) override;

 protected:
  // method for data provider
  retcode ExecuteAsDataProvider(const std::vector<std::string>& input,
                                std::vector<std::string>* result);
  retcode GetComputeNode(Node* compute_node);
  retcode HashData(const std::vector<std::string>& input,
                   std::unordered_map<std::string, std::string>* hashed_data);
  retcode EncryptDataWithShareKey(const std::string& key_id,
                                  const std::string& plain_data,
                                  std::vector<char>* cipher_data);

  retcode DecryptDataWithShareKey(const std::string& cipher_data,
                                  std::vector<char>* plain_data);

  retcode GetIntersection(std::string_view result_buf,
      std::unordered_map<std::string, std::string>& hashed_data,
      std::vector<std::string>* result);
  bool IsResultReceiver();

  // method for TEE compute
  retcode ExecuteAsCompute();
  retcode ReceiveDataFromDataProvider();
  retcode GetDataProviderNode(std::vector<Node>* data_provider_list);
  retcode DoCompute(const std::string& code);
  retcode SendEncryptedResult();
  retcode GetResultReceiver(Node* receiver);

 protected:
  std::string DataIdPrefix() {
    return GetLinkContext()->request_id();
  }
  std::string GetKey(const std::string& key_value) {
    return TeeExecutor()->get_key(DataIdPrefix(), key_value);
  }
  sgx::TeeEngine* TeeExecutor() {return executor_;}
  std::string GenerateDataId(const std::string& request_id,
                             const std::string& party_name) {
    return request_id + "_" + party_name;
  }
  std::string GenerateResultId() {
    return DataIdPrefix() + "_result";
  }
  std::string SetDataFile(const std::string& file_name) {
    files_.push_back(file_name);
  }
  std::vector<std::string>& DataFiles() {return files_;}
  std::vector<std::string>& ExtraInfo() {return extra_info_;}
  /**
   * clean the share data file generated on protocol execute
  */
  retcode CleanTempFile();

 private:
  sgx::RaTlsService* ra_service_{nullptr};
  sgx::TeeEngine* executor_{nullptr};
  std::vector<std::string> files_;
  std::vector<std::string> extra_info_;

  // compute
  size_t result_size_{0};
};

}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_TEE_PSI_H_
