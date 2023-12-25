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
#ifndef SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_BASE_PIR_H_
#define SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_BASE_PIR_H_
#include <map>
#include <string>
#include "src/primihub/common/common.h"
#include "src/primihub/kernel/pir/common.h"
#include "src/primihub/util/network/link_context.h"
namespace primihub::pir {
using LinkContext = network::LinkContext;
struct Options {
  LinkContext* link_ctx_ref;
  std::map<std::string, Node> party_info;
  std::string self_party;
  std::string code;
  Role role;
  // online
  bool use_cache{false};
  // offline task
  bool generate_db{false};
  std::string db_path;
  Node peer_node;
  Node proxy_node;
};

class BasePirOperator {
 public:
  explicit BasePirOperator(const Options& options) : options_(options) {}
  virtual ~BasePirOperator() = default;
  /**
   * PSI protocol
  */
  retcode Execute(const PirDataType& input, PirDataType* result);
  virtual retcode OnExecute(const PirDataType& input, PirDataType* result) = 0;
  void set_stop() {stop_.store(true);}

 protected:
  bool has_stopped() {
    return stop_.load(std::memory_order::memory_order_relaxed);
  }
  std::string PartyName() {return options_.self_party;}
  Role role() {return options_.role;}
  LinkContext* GetLinkContext() {return options_.link_ctx_ref;}
  Node& PeerNode() {return options_.peer_node;}
  Node& ProxyNode() {return options_.proxy_node;}

 protected:
  std::atomic<bool> stop_{false};
  Options options_;
  std::string key_{"pir_key"};
};
}  // namespace primihub::pir
#endif  // SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_BASE_PIR_H_
