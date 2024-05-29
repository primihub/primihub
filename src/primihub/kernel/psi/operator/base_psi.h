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
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
#include <map>
#include <vector>
#include <string>

#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/common.h"
#include "src/primihub/kernel/psi/operator/common.h"
namespace primihub::psi {
using LinkContext = network::LinkContext;
struct Options {
  LinkContext* link_ctx_ref{nullptr};
  std::map<std::string, Node> party_info;
  std::string self_party;
  PsiResultType psi_result_type{PsiResultType::INTERSECTION};
  std::string code;
  Node proxy_node;      // location to fecth recv data
};

class BasePsiOperator {
 public:
  explicit BasePsiOperator(const Options& options) : options_(options) {
    peer_node_ = PeerNode();
  }
  virtual ~BasePsiOperator() = default;
  /**
   * PSI protocol
  */
  retcode Execute(const std::vector<std::string>& input,
                  bool sync_result,
                  std::vector<std::string>* result);
  virtual retcode OnExecute(const std::vector<std::string>& input,
                            std::vector<std::string>* result) = 0;
  /**
   * broadcast from the party who get the result to the others who participate
   * in the protocol
   * result: input or output
   *  for party who finally get the result after protocol, result is input
   *  for party who get result after broadcast step, result is output
  */
  virtual retcode BroadcastPsiResult(std::vector<std::string>* result);

 protected:
  /**
   * get partylist who need receive result from
  */
  retcode BroadcastPartyList(std::vector<Node>* party_list);
  /**
   * party who does not belong to broadcast party list,
   * such as the party tho has already get result after execute protocol,
   * in TEE mode, the executor which does not care about the result
  */
  bool IgnoreParty(const std::string& party_name);
  /**
   * party who doesn't care about the result
  */
  bool IgnoreResult(const std::string& party_name);
  retcode BroadcastResult(const std::vector<std::string>& result);
  retcode ReceiveResult(std::vector<std::string>* result);

  void set_stop() {stop_.store(true);}
  retcode GetResult(const std::vector<std::string>& input,
                    const std::vector<uint64_t>& intersection_index,
                    std::vector<std::string>* result);
 protected:
  bool has_stopped() {
    return stop_.load(std::memory_order::memory_order_relaxed);
  }
  std::string PartyName() {return options_.self_party;}
  LinkContext* GetLinkContext() {return options_.link_ctx_ref;}
  PsiResultType GetPsiResultType() {return options_.psi_result_type;}
  Node PeerNode();
  Node& ProxyServerNode();
  retcode GetNodeByName(const std::string& party_name, Node* node_info);

 protected:
  std::atomic<bool> stop_{false};
  Options options_;
  std::string key_{"default"};
  Node peer_node_;
};
}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
