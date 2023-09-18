// "Copyright [2023] <PrimiHub>"

#include "src/primihub/util/network/mpc_channel.h"
#include <string_view>
#include <atomic>
#include <functional>
#include <utility>
#include <cstring>

namespace primihub::network {

ph_link::retcode MPCTaskChannel::SendImpl(const std::string& send_buf) {
  auto send_sv = std::string_view(send_buf.data(), send_buf.size());
  return SendImpl(send_sv);
}

ph_link::retcode MPCTaskChannel::SendImpl(const char* buff, size_t size) {
  auto send_sv = std::string_view(buff, size);
  return SendImpl(send_sv);
}

ph_link::retcode MPCTaskChannel::SendImpl(std::string_view send_buff_sv) {
  auto ret = send_channel_->send(send_key_, send_buff_sv);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send message to " << peer_node_id_ << " failed, size "
       << send_buff_sv.size() << ", send key " << send_key_ << ".";
    LOG(ERROR) << ss.str();
    return ph_link::retcode::FAIL;
  }
  if (VLOG_IS_ON(8)) {
    std::string send_data;
    for (const auto& ch : send_buff_sv) {
      send_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(INFO) << "MPCTaskChannel::SendImpl "
              << "send_key: " << send_key_ << " "
              << "data size: " << send_buff_sv.size() << " "
              << "send data: [" << send_data << "]";
  }

  return ph_link::retcode::SUCCESS;
}

ph_link::retcode MPCTaskChannel::RecvImpl(std::string* recv_buf) {
  *recv_buf = recv_channel_->forwardRecv(recv_key_);
  if (VLOG_IS_ON(8)) {
    std::string recv_data;
    for (const auto& ch : *recv_buf) {
      recv_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(INFO) << "MPCTaskChannel::RecvImpl "
              << "recv_key: " << recv_key_
              << "data size: " << recv_buf->size() << " "
              << "recv data: " << recv_data;
  }
  return ph_link::retcode::SUCCESS;
}

ph_link::retcode MPCTaskChannel::RecvImpl(char* recv_buf, size_t recv_size) {
  std::string tmp_recv_buf = recv_channel_->forwardRecv(recv_key_);
  if (tmp_recv_buf.size() != recv_size) {
    LOG(ERROR) << "data length does not match: " << " "
               << "expected: " << recv_size << " "
              << "actually: " << tmp_recv_buf.size();
    return ph_link::retcode::FAIL;
  }
  memcpy(recv_buf, tmp_recv_buf.data(), recv_size);
  if (VLOG_IS_ON(8)) {
    std::string recv_data;
    for (const auto& ch : tmp_recv_buf) {
      recv_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(ERROR) << "MPCTaskChannel::RecvImpl "
              << "recv_key: " << recv_key_ << " "
              << "data size: " << recv_size << " "
              << "recv data: [" << recv_data << "] ";
  }

  return ph_link::retcode::SUCCESS;
}

void MPCTaskChannel::close() {
}

void MPCTaskChannel::cancel() {
  cancel_.store(true);
}
}  // namespace primihub::network
