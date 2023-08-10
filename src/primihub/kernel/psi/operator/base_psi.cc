// "Copyright [2023] <PrimiHub>"
#include "src/primihub/kernel/psi/operator/base_psi.h"
#include <utility>

#include "src/primihub/util/endian_util.h"
#include "src/primihub/util/util.h"

namespace primihub::psi {
retcode BasePsiOperator::Execute(const std::vector<std::string>& input,
                                 bool sync_result,
                                 std::vector<std::string>* result) {
  auto ret = this->OnExecute(input, result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "Execut PSI failed";
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
  return ret;
}

retcode BasePsiOperator::BroadcastPsiResult(std::vector<std::string>* result) {
  if (IgnoreResult(options_.self_party)) {
    return retcode::SUCCESS;
  }
  auto ret{retcode::SUCCESS};
  if (IsClient(options_.self_party)) {
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
    auto ret = this->Send(party_info, result_str);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "Send result data to: "
                 << party_info.to_string() << " failed";
    }
  }
  return retcode::SUCCESS;
}

retcode BasePsiOperator::ReceiveResult(std::vector<std::string>* result) {
  std::string recv_data_str;
  auto ret = Recv(&recv_data_str);
  if (ret != retcode::SUCCESS) {
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

retcode BasePsiOperator::Send(const Node& dest_node,
                              const std::string& send_buff) {
  return Send(this->key_, dest_node, send_buff);
}

retcode BasePsiOperator::Send(const std::string& key,
                              const Node& dest_node,
                              const std::string& send_buff) {
  auto channel = options_.link_ctx_ref->getChannel(dest_node);
  auto ret = channel->send(key, send_buff);
  VLOG(5) << "send date to " << dest_node.to_string() << " success";
  return ret;
}

retcode BasePsiOperator::Send(const std::string& key,
                              const Node& dest_node,
                              const std::string_view send_buff_sv) {
  auto channel = options_.link_ctx_ref->getChannel(dest_node);
  auto ret = channel->send(key, send_buff_sv);
  VLOG(5) << "send data to " << dest_node.to_string() << " success";
  return ret;
}

retcode BasePsiOperator::Recv(std::string* recv_buff) {
  return Recv(this->key_, recv_buff);
}

retcode BasePsiOperator::Recv(const std::string& key, std::string* recv_buff) {
  std::string recv_data_str;
  auto link_ctx = options_.link_ctx_ref;
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  recv_queue.wait_and_pop(recv_data_str);
  *recv_buff = std::move(recv_data_str);
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
  if (IsTeeCompute(party_name)) {
    return true;
  }
  return false;
}

Node BasePsiOperator::PeerNode() {
  std::string peer_node_name;
  Node peer_node;
  if (IsClient(PartyName())) {
    peer_node_name = PARTY_SERVER;
  } else if (IsServer(PartyName())) {
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

bool BasePsiOperator::IsClient(const std::string& party_name) {
  return party_name == PARTY_CLIENT;
}
bool BasePsiOperator::IsServer(const std::string& party_name) {
  return party_name == PARTY_SERVER;
}
bool BasePsiOperator::IsTeeCompute(const std::string& party_name) {
  return party_name == PARTY_TEE_COMPUTE;
}
}  // namespace primihub::psi
