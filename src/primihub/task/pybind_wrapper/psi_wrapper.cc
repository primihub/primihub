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
#include "src/primihub/task/pybind_wrapper/psi_wrapper.h"
#include <glog/logging.h>

#include "src/primihub/common/common.h"
#include "src/primihub/common/value_check_util.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/kernel/psi/util.h"

namespace primihub::task {
namespace rpc = primihub::rpc;
PsiExecutor::PsiExecutor(const std::string& task_req_str,
                         const std::string& root_ca_path,
                         const std::string& key_path,
                         const std::string& cert_path) :
    root_ca_path_(root_ca_path), key_path_(key_path), cert_path_(cert_path) {
  task_req_ptr_ = std::make_unique<rpc::PushTaskRequest>();
  bool succ_flag = task_req_ptr_->ParseFromString(task_req_str);
  if (!succ_flag) {
    RaiseException("parser task request encountes error");
  }
  auto& task_config = task_req_ptr_->task();

  task_ptr_ = std::make_unique<PsiTask>(&task_config);
  if (!root_ca_path.empty() && !key_path.empty() && !cert_path.empty()) {
    LOG(INFO) << "link_ctx->initCertificate";
    auto link_ctx = task_ptr_->getTaskContext().getLinkContext().get();
    link_ctx->initCertificate(root_ca_path, key_path, cert_path);
  }
}

auto PsiExecutor::RunPsi(const std::vector<std::string>& input,
                         const std::vector<std::string>& parties,
                         const std::string& receiver,
                         bool broadcast_result,
                         const std::string& protocol) ->
    std::vector<std::string> {
  if (parties.size() != 2) {
    std::stringstream ss;
    ss << "Need 2 parties, but get " << parties.size();
    RaiseException(ss.str());
  }
  std::vector<std::string> result;
  auto req_ptr = BuildPsiTaskRequest(parties, receiver,
                                    broadcast_result, protocol, *task_req_ptr_);
  auto task_config_ptr = req_ptr->mutable_task();
  auto task_ptr = std::make_unique<PsiTask>(task_config_ptr);
  if (!root_ca_path_.empty() && !key_path_.empty() && !cert_path_.empty()) {
    LOG(INFO) << "link_ctx->initCertificate XXXXXXXX";
    auto link_ctx = task_ptr_->getTaskContext().getLinkContext().get();
    link_ctx->initCertificate(root_ca_path_, key_path_, cert_path_);
    auto& options = task_ptr->PsiOptions();
    options.link_ctx_ref = link_ctx;
  }
  auto ret = task_ptr->ExecuteTask(input, &result);
  if (ret != retcode::SUCCESS) {
    RaiseException("run psi task failed");
  }
  LOG(INFO) << "PsiExecutor::RunPsi: " << result.size();
  return result;
}
// to support multithread mode in future, first Negotiate a unique key
retcode PsiExecutor::NegotiateSubTaskId(std::string* sub_task_id, Role role) {
  return retcode::SUCCESS;
}
auto PsiExecutor::BuildPsiTaskRequest(const std::vector<std::string>& parties,
    const std::string& receiver,
    bool broadcast_result,
    const std::string& protocol,
    const primihub::rpc::PushTaskRequest& request) ->
      std::unique_ptr<primihub::rpc::PushTaskRequest> {
  auto req_ptr = std::make_unique<primihub::rpc::PushTaskRequest>(request);
  auto task_config_ptr = req_ptr->mutable_task();
  // build access info
  const auto& party_access_info = request.task().party_access_info();
  auto it = party_access_info.find(receiver);
  if (it == party_access_info.end()) {
    std::stringstream ss;
    ss << "No access info found for result receiver: " << receiver;
    RaiseException(ss.str());
  }
  task_config_ptr->clear_party_access_info();
  auto ptr = task_config_ptr->mutable_party_access_info();
  (*ptr)[PARTY_CLIENT].CopyFrom(it->second);
  for (const auto& party_name : parties) {
    if (party_name == receiver) {
      continue;
    }
    auto it = party_access_info.find(party_name);
    if (it == party_access_info.end()) {
      std::stringstream ss;
      ss << "No access info found for party: " << party_name;
      RaiseException(ss.str());
    }
    (*ptr)[PARTY_SERVER].CopyFrom(it->second);
  }
  // set party name
  auto& party_name = task_config_ptr->party_name();
  if (party_name == receiver) {
    task_config_ptr->set_party_name(PARTY_CLIENT);
  } else {
    task_config_ptr->set_party_name(PARTY_SERVER);
  }
  // broadcast result
  auto param_map = task_config_ptr->mutable_params()->mutable_param_map();
  rpc::ParamValue pv;
  pv.set_is_array(false);
  pv.set_var_type(rpc::INT32);
  if (broadcast_result) {
    pv.set_value_int32(1);
  } else {
    pv.set_value_int32(0);
  }
  (*param_map)["sync_result_to_server"] = std::move(pv);
  // configure psi protocol
  rpc::ParamValue psi_type;
  psi_type.set_is_array(false);
  psi_type.set_var_type(rpc::INT32);
  if (protocol == std::string("KKRT")) {
    psi_type.set_value_int32(static_cast<int>(rpc::KKRT));
  } else if (protocol == std::string("ECDH")) {
    psi_type.set_value_int32(static_cast<int>(rpc::ECDH));
  } else {
    std::stringstream ss;
    ss << "Unknown PSI protocol: " << protocol;
    RaiseException(ss.str());
  }
  (*param_map)["psiTag"] = std::move(psi_type);
  return req_ptr;
}
}  // namespace primihub::task
