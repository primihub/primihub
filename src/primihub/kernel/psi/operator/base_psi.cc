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

#include "src/primihub/kernel/psi/operator/base_psi.h"
#include <utility>
#include <future>

#include "src/primihub/util/endian_util.h"
#include "src/primihub/util/util.h"

namespace primihub::psi {
retcode BasePsiOperator::Execute(const std::vector<std::string>& input,
                                 bool sync_result,
                                 std::vector<std::string>* result) {
  auto ret = this->OnExecute(input, result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "Execute PSI failed";
    return retcode::FAIL;
  }
  // broadcast result from party who get result during the protocol
  // to the other parties who participate
  if (sync_result) {
    SCopedTimer timer;
    ret = BroadcastPsiResult(result);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "Broadcast Psi Result failed";
    }
    auto time_cost = timer.timeElapse();
    VLOG(5) << "BroadcastPsiResult time cost(ms): " << time_cost;
  }
  // calculate DIFFERENCE
  if (options_.psi_result_type == PsiResultType::DIFFERENCE) {
    if (input.size() == result->size()) {
      result->clear();
      *result = std::vector<std::string>();
      return retcode::SUCCESS;
    }
    if (result->empty()) {
      result->assign(input.begin(), input.end());
      return retcode::SUCCESS;
    }
    std::set<std::string> duplicate(result->begin(), result->end());
    result->clear();
    for (const auto& item : input ) {
      if (duplicate.find(item) != duplicate.end()) {
        continue;
      }
      result->push_back(item);
    }
  }
  return ret;
}

retcode BasePsiOperator::BroadcastPsiResult(std::vector<std::string>* result) {
  if (IgnoreResult(options_.self_party)) {
    return retcode::SUCCESS;
  }
  auto ret{retcode::SUCCESS};
  if (RoleValidation::IsClient(options_.self_party)) {
    ret = BroadcastResult(*result);
  } else {
    ret = ReceiveResult(result);
  }
  return ret;
}

retcode BasePsiOperator::BroadcastResult(
    const std::vector<std::string>& result) {
  retcode ret{retcode::SUCCESS};
  VLOG(5) << "broadcast result to server";
  std::string result_str;
  size_t total_size{0};
  for (const auto& item : result) {
    total_size += item.size();
  }
  total_size += result.size() * sizeof(uint64_t);
  result_str.reserve(total_size);
  for (const auto& item : result) {
    uint64_t item_len = item.size();
    uint64_t be_item_len = htonll(item_len);
    result_str.append(reinterpret_cast<char*>(&be_item_len),
                      sizeof(be_item_len));
    result_str.append(item);
  }
  std::vector<Node> party_list;
  BroadcastPartyList(&party_list);
  for (const auto& party_info : party_list) {
    auto ret = this->GetLinkContext()->Send(this->key_, party_info, result_str);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "Send result data to: "
                 << party_info.to_string() << " failed";
    }
  }
  return retcode::SUCCESS;
}

retcode BasePsiOperator::ReceiveResult(std::vector<std::string>* result) {
  std::string recv_data_str;
  auto ret = this->GetLinkContext()->Recv(this->key_,
                                          this->ProxyServerNode(),
                                          &recv_data_str);
  if (ret != retcode::SUCCESS && !recv_data_str.empty()) {
    LOG(ERROR) << "ReceiveResult failed for party name: "
               << options_.self_party;
    return retcode::FAIL;
  }
  uint64_t offset = 0;
  uint64_t data_len = recv_data_str.length();
  VLOG(5) << "data_len_data_len: " << data_len;
  auto data_ptr = const_cast<char*>(recv_data_str.c_str());
  // format length: value
  while (offset < data_len) {
    auto len_ptr = reinterpret_cast<uint64_t*>(data_ptr+offset);
    uint64_t be_len = *len_ptr;
    uint64_t len = ntohll(be_len);
    offset += sizeof(uint64_t);
    result->push_back(std::string(data_ptr+offset, len));
    offset += len;
  }
  return retcode::SUCCESS;
}

retcode BasePsiOperator::BroadcastPartyList(std::vector<Node>* party_list) {
  party_list->clear();
  for (const auto& [party_name, node] : options_.party_info) {
    if (IgnoreParty(party_name)) {
      continue;
    }
    party_list->emplace_back(node);
  }
  return retcode::SUCCESS;
}

bool BasePsiOperator::IgnoreParty(const std::string& party_name) {
  if (party_name == options_.self_party) {
    return true;
  } else if (IgnoreResult(party_name)) {
    return true;
  } else {
    return false;
  }
}

bool BasePsiOperator::IgnoreResult(const std::string& party_name) {
  if (RoleValidation::IsTeeCompute(party_name)) {
    return true;
  }
  return false;
}

Node BasePsiOperator::PeerNode() {
  std::string peer_node_name;
  Node peer_node;
  if (RoleValidation::IsClient(PartyName())) {
    peer_node_name = PARTY_SERVER;
  } else if (RoleValidation::IsServer(PartyName())) {
    peer_node_name = PARTY_CLIENT;
  } else {
    LOG(ERROR) << "Invalid party name: " << PartyName();
    return Node();
  }
  auto ret = GetNodeByName(peer_node_name, &peer_node);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "no party info for party name: " << peer_node_name;
    return Node();
  }
  return peer_node;
}

Node& BasePsiOperator::ProxyServerNode() {
  return options_.proxy_node;
}

retcode BasePsiOperator::GetNodeByName(const std::string& party_name,
                                       Node* node_info) {
  auto it = options_.party_info.find(party_name);
  if (it != options_.party_info.end()) {
    *node_info = it->second;
  } else {
    LOG(ERROR) << "no party info for party name: " << party_name;
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}
retcode BasePsiOperator::GetResult(const std::vector<std::string>& input,
    const std::vector<uint64_t>& intersection_index,
    std::vector<std::string>* result) {
  SCopedTimer timer;
  size_t result_size = intersection_index.size();
  result->resize(result_size);
  size_t block_size = 10000000;
  size_t block_num = result_size / block_size;
  size_t rem_num = result_size % block_size;
  std::vector<std::future<void>> futs;
  for (size_t i = 0; i < block_num; i++) {
    futs.push_back(
      std::async(
        std::launch::async,
        [&, i]() -> void {
          size_t index = i*block_size;
          auto& result_ref = *result;
          for (size_t j = 0; j < block_size; j++) {
            uint64_t pos = intersection_index[index];
            result_ref[index] = input[pos];
            index++;
          }
        }));
  }
  if (rem_num) {
    futs.push_back(
      std::async(
        std::launch::async,
        [&]() -> void {
          size_t index = block_num*block_size;
          auto& result_ref = *result;
          for (size_t j = 0; j < rem_num; j++) {
            uint64_t pos = intersection_index[index];
            result_ref[index] = input[pos];
            index++;
          }
        }));
  }
  for (auto&& fut : futs) {
    fut.get();
  }
  auto time_cost = timer.timeElapse();
  VLOG(3) << "Get Intersection Result time cost: " << time_cost;
  return retcode::SUCCESS;
}
}  // namespace primihub::psi
