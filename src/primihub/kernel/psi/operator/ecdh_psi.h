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

#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_ECDH_PSI_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_ECDH_PSI_H_
#include <unordered_map>
#include <memory>
#include <string>
#include <set>
#include <vector>

#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "private_set_intersection/cpp/psi_client.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/psi.pb.h"
#include "src/primihub/protos/worker.pb.h"

namespace primihub::psi {
namespace openminded_psi = private_set_intersection;
class EcdhPsiOperator : public BasePsiOperator {
 public:
  explicit EcdhPsiOperator(const Options& options) : BasePsiOperator(options) {}
  retcode OnExecute(const std::vector<std::string>& input,
                    std::vector<std::string>* result) override;

 protected:
  retcode ExecuteAsClient(const std::vector<std::string>& input,
                          std::vector<std::string>* result);
  retcode SendRequetToServer(psi_proto::Request&& psi_request);
  retcode BuildInitParam(int64_t element_size, std::string* init_param);
  retcode SendInitParam(const std::string& init_param);
  retcode SendPSIRequestAndWaitResponse(const psi_proto::Request& request,
                                        rpc::PsiResponse* response);
  retcode SendPSIRequestAndWaitResponse(psi_proto::Request&& request,
                                        rpc::PsiResponse* response);
  retcode ParsePsiResponseFromeString(const std::string& res_str,
                                      rpc::PsiResponse* response);
  retcode GetIntersection(const std::vector<std::string> origin_data,
    const std::unique_ptr<openminded_psi::PsiClient>& client,
    rpc::PsiResponse& response,
    std::vector<std::string>* result);
  // server method
  retcode ExecuteAsServer(const std::vector<std::string>& input);
  retcode InitRequest(psi_proto::Request* psi_request);
  retcode PreparePSIResponse(psi_proto::Response&& psi_response,
                             psi_proto::ServerSetup&& setup);
  retcode RecvInitParam(size_t* client_dataset_size, bool* reveal_intersection);
  void SetFpr(double fpr) {fpr_ = fpr;}

 private:
  bool reveal_intersection_{true};
  double fpr_{0.0001};
};
}  // namespace primihub::psi

#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_ECDH_PSI_H_
