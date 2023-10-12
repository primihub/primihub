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
#include "src/primihub/task/language/parser.h"

namespace primihub::task {
LanguageParser::LanguageParser(const rpc::PushTaskRequest& task_request) {
  task_request_.CopyFrom(task_request);
}

retcode LanguageParser::MergePartyAccessInfo(
    const std::map<std::string, Node>& party_access_info) {
  std::map<std::string, Node> filtered_party;
  auto party_access_info_ptr =
      this->task_request_.mutable_task()->mutable_party_access_info();
  // fetch all party from mutable_party_access_info
  for (const auto& [party_name, pb_node] : *party_access_info_ptr) {
    Node node;
    pbNode2Node(pb_node, &node);
    auto iter = filtered_party.find(party_name);
    if (iter == filtered_party.end()) {
      filtered_party[party_name] = std::move(node);
    }
  }
  // need_merged
  for (const auto& [party_name, node] : party_access_info) {
    auto iter = filtered_party.find(party_name);
    if (iter == filtered_party.end()) {
      filtered_party[party_name] = node;
    }
  }
  // refill all
  party_access_info_ptr->clear();
  int32_t party_id{0};
  for (auto& [party_name, node] : filtered_party) {
    auto& party_node = (*party_access_info_ptr)[party_name];
    node2PbNode(node, &party_node);
    party_node.set_party_id(party_id);
    party_id++;
  }
  return retcode::SUCCESS;
}
}  // namespace primihub::task {