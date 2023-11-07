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
#include "src/primihub/algorithm/base.h"
#include <map>

#include "src/primihub/util/network/mpc_channel.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/config/server_config.h"

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
oc::IOService g_ios_;
#endif  // MPC_SOCKET_CHANNEL
AlgorithmBase::AlgorithmBase(const PartyConfig& config,
                             std::shared_ptr<DatasetService> dataset_service)
                             : dataset_service_(std::move(dataset_service)) {
  party_config_.Init(config);
  this->set_party_name(config.party_name());
  this->set_party_id(config.party_id());

#ifdef MPC_SOCKET_CHANNEL
  auto &node_map = config.node_map;
  std::map<uint16_t, rpc::Node> party_id_node_map;
  for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
    auto& node = iter->second;
    uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
    party_id_node_map[party_id] = node;
  }

  auto iter = node_map.find(config.node_id);  // node_id
  if (iter == node_map.end()) {
    std::stringstream ss;
    ss << "Can't find " << config.node_id << " in node_map.";
    throw std::runtime_error(ss.str());
  }

  uint16_t local_id_ = iter->second.vm(0).party_id();
  LOG(INFO) << "Note party id of this node is " << local_id_ << ".";

  if (local_id_ == 0) {
    rpc::Node &node = party_id_node_map[0];
    uint16_t port = 0;

    // Two Local server addr.
    auto& next_ip = node.ip();
    uint16_t next_port = node.vm(0).next().port();
    next_addr_ = std::make_pair(next_ip, next_port);

    auto& prev_ip = node.ip();
    uint16_t prev_port = node.vm(0).prev().port();
    prev_addr_ = std::make_pair(prev_ip, prev_port);
  } else if (local_id_ == 1) {
    rpc::Node &node = party_id_node_map[1];

    // A local server addr.
    auto& next_ip = node.ip();
    uint16_t next_port = node.vm(0).next().port();
    next_addr_ = std::make_pair(next_ip, next_port);

    // A remote server addr.
    auto& prev_ip = node.vm(0).prev().ip();
    uint16_t prev_port = node.vm(0).prev().port();
    prev_addr_ = std::make_pair(prev_ip, prev_port);
  } else {
    rpc::Node &node = party_id_node_map[2];
    // Two remote server addr.
    auto& next_ip = node.vm(0).next().ip();
    uint16_t next_port = node.vm(0).next().port();
    next_addr_ = std::make_pair(next_ip, next_port);
    auto& prev_ip = node.vm(0).prev().ip();
    uint16_t prev_port = node.vm(0).prev().port();
    prev_addr_ = std::make_pair(prev_ip, prev_port);
  }
#endif
}

int AlgorithmBase::loadParams(primihub::rpc::Task &task) {
  const auto& task_info = task.task_info();
  this->SetTraceId(task_info.request_id());
}

retcode AlgorithmBase::ExtractProxyNode(const rpc::Task& task_config,
                                        Node* proxy_node) {
  const auto& auxiliary_server = task_config.auxiliary_server();
  auto it = auxiliary_server.find(PROXY_NODE);
  if (it == auxiliary_server.end()) {
    LOG(ERROR) << "no proxy node found";
    return retcode::FAIL;
  }
  const auto& pb_proxy_node = it->second;
  pbNode2Node(pb_proxy_node, proxy_node);
  return retcode::SUCCESS;
}

#ifdef MPC_SOCKET_CHANNEL
int AlgorithmBase::initPartyComm() {
  VLOG(3) << "Next addr: " << next_addr_.first << ":" << next_addr_.second << ".";
  VLOG(3) << "Prev addr: " << prev_addr_.first << ":" << prev_addr_.second << ".";
  if (party_id_ == 0) {
    std::ostringstream ss;
    ss << "sess_" << party_id_ << "_1";
    std::string sess_name_1 = ss.str();

    ss.str("");
    ss << "sess_" << party_id_ << "_2";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   oc::SessionMode::Server, sess_name_1);
    LOG(INFO) << "[Next] Init server session, party " << party_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   oc::SessionMode::Server, sess_name_2);
    LOG(INFO) << "[Prev] Init server session, party " << party_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  } else if (party_id_ == 1) {
    std::ostringstream ss;
    ss << "sess_" << party_id_ << "_1";
    std::string sess_name_1 = ss.str();
    ss.str("");

    ss << "sess_" << party_config_.PrevPartyId() << "_1";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   oc::SessionMode::Server, sess_name_1);
    LOG(INFO) << "[Next] Init server session, party " << party_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   oc::SessionMode::Client, sess_name_2);
    LOG(INFO) << "[Prev] Init client session, party " << party_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  } else {
    std::ostringstream ss;
    ss.str("");
    ss << "sess_" << party_config_.NextPartyId() << "_2";
    std::string sess_name_1 = ss.str();

    ss.str("");
    ss << "sess_" << party_config_.PrevPartyId() << "_1";
    std::string sess_name_2 = ss.str();

    ep_next_.start(ios_, next_addr_.first, next_addr_.second,
                   oc::SessionMode::Client, sess_name_1);
    LOG(INFO) << "[Next] Init client session, party " << party_id_ << ", "
              << "ip " << next_addr_.first << ", port " << next_addr_.second
              << ", name " << sess_name_1 << ".";

    ep_prev_.start(ios_, prev_addr_.first, prev_addr_.second,
                   oc::SessionMode::Client, sess_name_2);
    LOG(INFO) << "[Prev] Init client session, party " << party_id_ << ", "
              << "ip " << prev_addr_.first << ", port " << prev_addr_.second
              << ", name " << sess_name_2 << ".";
  }
  session_enabled = true;
  // init communication channel
  comm_pkg_ = std::make_unique<aby3::CommPkg>();
  comm_pkg_->mNext = ep_next_.addChannel();
  comm_pkg_->mPrev = ep_prev_.addChannel();
  this->mNext().waitForConnection();
  this->mPrev().waitForConnection();

  this->mNext().send(party_id_);
  this->mPrev().send(party_id_);

  uint16_t prev_party = 0;
  uint16_t next_party = 0;
  this->mNext().recv(next_party);
  this->mPrev().recv(prev_party);

  if (next_party != party_config_.NextPartyId()) {
    LOG(ERROR) << "Party " << party_id_ << ", expect next party id "
               << party_config_.NextPartyId() << ", but give " << next_party << ".";
    return -3;
  }

  if (prev_party != party_config_.PrevPartyId()) {
    LOG(ERROR) << "Party " << party_id_ << ", expect prev party id "
               << party_config_.PrevPartyId() << ", but give " << prev_party << ".";
    return -3;
  }
  return 0;
}

int AlgorithmBase::finishPartyComm() {
  if (comm_pkg_ == nullptr) {
    return 0;
  }
  this->mNext().close();
  this->mPrev().close();
  if (session_enabled) {
    ep_next_.stop();
    ep_prev_.stop();
  }

  return 0;
}

#else  // GRPC MPC_SOCKET_CHANNEL
int AlgorithmBase::initPartyComm(
    const std::vector<ph_link::Channel>& channels) {
  if (channels.size() < 2) {
    LOG(ERROR) << "channel size at least 2";
    return -1;
  }
  // 0: prev   1: next
  comm_pkg_ = std::make_unique<aby3::CommPkg>();
  comm_pkg_->mPrev = channels[0];
  comm_pkg_->mNext = channels[1];
  return 0;
}

int AlgorithmBase::initPartyComm() {
  uint16_t prev_party_id = this->party_config_.PrevPartyId();
  uint16_t next_party_id = this->party_config_.NextPartyId();
  auto link_ctx = this->GetLinkContext();
  if (link_ctx == nullptr) {
    LOG(ERROR) << "link context is not available";
    return -1;
  }

  // construct channel for communication to next party
  std::string party_name_next = this->party_config_.NextPartyName();
  auto party_node_next = this->party_config_.NextPartyInfo();
  auto base_channel_next = link_ctx->getChannel(party_node_next);

  // construct channel for communication to prev party
  std::string party_name_prev = this->party_config_.PrevPartyName();
  auto party_node_prev = this->party_config_.PrevPartyInfo();
  auto base_channel_prev = link_ctx->getChannel(party_node_prev);

  std::string job_id = link_ctx->job_id();
  std::string task_id = link_ctx->task_id();
  std::string request_id = link_ctx->request_id();
  LOG(INFO) << "local_id_local_id_: " << this->party_id();
  LOG(INFO) << "next_party: " << party_name_next
            << " detail: " << party_node_next.to_string();
  LOG(INFO) << "prev_party: " << party_name_prev
            << " detail: " << party_node_prev.to_string();
  LOG(INFO) << "job_id: " << job_id << " task_id: "
            << task_id << " request id: " << request_id;

  auto recv_channel = link_ctx->getChannel(this->proxy_node_);
  // The 'osuCrypto::Channel' will consider it to be a unique_ptr and will
  // reset the unique_ptr, so the 'osuCrypto::Channel' will delete it.
  auto channel_impl_prev =
      std::make_shared<network::MPCTaskChannel>(
          this->party_name(), party_name_prev,
          link_ctx, base_channel_prev, recv_channel);

  auto channel_impl_next =
      std::make_shared<network::MPCTaskChannel>(
          this->party_name(), party_name_next,
          link_ctx, base_channel_next, recv_channel);

  ph_link::Channel chl_prev(channel_impl_prev);
  ph_link::Channel chl_next(channel_impl_next);
  comm_pkg_ = std::make_unique<aby3::CommPkg>();
  comm_pkg_->mPrev = chl_prev;
  comm_pkg_->mNext = chl_next;
  return 0;
}

int AlgorithmBase::finishPartyComm() {
  if (comm_pkg_ == nullptr) {
    return 0;
  }
  VLOG(5) << "stop next channel.";
  this->mNext().close();
  VLOG(5) << "stop prev channel.";
  this->mPrev().close();
  return 0;
}
#endif  // MPC_SOCKET_CHANNEL

}  // namespace primihub
