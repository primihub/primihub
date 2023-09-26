// "Copyright [2023] <PrimiHub>"

#include "src/primihub/util/network/message_interface.h"
#include <string_view>
#include <atomic>
#include <functional>
#include <utility>

#include "boost/system/system_error.hpp"

namespace primihub::network {
void TaskMessagePassInterface::_channelSend(
    const std::string send_key,
    osuCrypto::span<boost::asio::mutable_buffer> buffers,
    io_completion_handle &&fn) {
  if (send_count_.fetch_add(1) > 0)
    LOG(WARNING) << "Parallel send detected, current send count "
                 << send_count_.load();

  u64 num = buffers.size();
  u64 index = 0;
  boost::system::error_code errcode;
  u64 bytes_send = 0;
  bool errors = false;

  for (u64 i = 0; i < buffers.size(); i++) {
    uint8_t *ptr = boost::asio::buffer_cast<u8 *>(buffers[i]);
    size_t size = boost::asio::buffer_size(buffers[i]);

    if (cancel_) {
      LOG(WARNING) << "Cancel send message, send key " << send_key
                   << ", already send " << index + 1 << ", total " << num
                   << ".";
      break;
    }

    std::string_view send_str(reinterpret_cast<const char *>(ptr), size);
    auto ret = send_channel_->send(send_key, send_str);
    if (ret != retcode::SUCCESS) {
      std::stringstream ss;
      ss << "Send message to " << peer_node_id_ << " failed, size " << size
         << ", send key " << send_key << ".";
      LOG(ERROR) << ss.str();

      errors = true;
      break;
    }

    bytes_send += size;
    index += 1;

    VLOG(6) << "Send " << index << "th message finish, total " << num
            << ", send size " << size << ".";

    if (VLOG_IS_ON(7)) {
      // std::string send_data;
      // for (const auto& ch : send_str) {
      //   send_data.append(std::to_string(static_cast<int>(ch))).append(" ");
      // }
      LOG(INFO) << "send data using key: " << send_key << " "
                << "data size: " << send_str.size() << "]";
    }
  }

  if (cancel_) {
    errcode = boost::system::errc::make_error_code(
        boost::system::errc::operation_canceled);
    fn(errcode, bytes_send);
    return;
  }

  if (errors) {
    errcode =
        boost::system::errc::make_error_code(boost::system::errc::broken_pipe);
    fn(errcode, bytes_send);
    return;
  }

  fn(errcode, bytes_send);

  send_count_.fetch_sub(1);
  return;
}

void TaskMessagePassInterface::_channelRecv(
    const std::string recv_key, ThreadSafeQueue<std::string> &queue,
    osuCrypto::span<boost::asio::mutable_buffer> buffers,
    io_completion_handle &&fn) {
  std::shared_ptr<WaitLock> wait_lock(new WaitLock());
  {
    std::lock_guard<std::mutex> lock(recv_queue_mu_);
    if (!recv_wait_queue_.empty()) {
      recv_wait_queue_.push(wait_lock);
      wait_lock->wait();
    }
  }

  if (recv_count_.fetch_add(1) > 1)
    LOG(WARNING) << "Parallel recv detected, current recv count is "
                 << recv_count_.load() << ".";

  u64 num = buffers.size();
  u64 index = 0;
  boost::system::error_code errcode;
  u64 bytes_recv = 0;
  bool errors = false;

  for (u64 i = 0; i < buffers.size(); i++) {
    auto ptr = boost::asio::buffer_cast<u8*>(buffers[i]);
    size_t size = boost::asio::buffer_size(buffers[i]);

    if (cancel_) {
      LOG(WARNING) << "Cancel recv message, recv key " << recv_key
                   << ", already recv " << index + 1 << ", total " << num
                   << ".";
      errcode = boost::system::errc::make_error_code(
          boost::system::errc::operation_canceled);
      break;
    }

    std::string recv_str = recv_channel_->forwardRecv(recv_key);

    if (!recv_str.size()) {
      LOG(ERROR) << "Recv queue has shutdown, recv failed, already recv "
                 << index + 1 << ", total " << num << ".";
      errcode = boost::system::errc::make_error_code(
          boost::system::errc::interrupted);

      errors = true;
      break;
    }

    if (recv_str.size() != size) {
      LOG(ERROR) << "Recv buffer size mismatch, expect " << recv_str.size()
                 << " bytes, gives " << size << " bytes, recv key " << recv_key
                 << ", already recv " << index + 1 << ", total " << num << ".";
      errcode = boost::system::errc::make_error_code(
          boost::system::errc::message_size);

      errors = true;
      break;
    }

    memcpy(ptr, recv_str.c_str(), recv_str.size());

    index += 1;
    bytes_recv += size;

    VLOG(6) << "Recv " << index << "th message, total " << num << ", recv size "
            << size << ".";
    if (VLOG_IS_ON(7)) {
      // std::string recv_data;
      // for (const auto& ch : recv_str) {
      //   recv_data.append(std::to_string(static_cast<int>(ch))).append(" ");
      // }
      LOG(INFO) << "recv data using key: " << recv_key << " "
                << "data size: " << recv_str.size() << "]";
    }

  }

  fn(errcode, bytes_recv);

  recv_count_.fetch_sub(1);
  {
    std::shared_ptr<WaitLock> wait_lock;
    std::lock_guard<std::mutex> lock(recv_queue_mu_);
    if (!recv_wait_queue_.empty()) {
      wait_lock = recv_wait_queue_.front();
      recv_wait_queue_.pop();
      wait_lock->notify();
    }
  }

  return;
}

void TaskMessagePassInterface::async_send(
    osuCrypto::span<boost::asio::mutable_buffer> buffers,
    io_completion_handle &&fn) {
  std::string send_key = SendKey();
  auto send_fn = std::bind(&TaskMessagePassInterface::_channelSend, this,
                           std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3);

  auto send_thread = std::thread(send_fn, send_key, buffers, std::move(fn));
  send_thread.detach();
  // _channelSend(send_key, buffers, std::move(fn));
}

void TaskMessagePassInterface::async_recv(
    osuCrypto::span<boost::asio::mutable_buffer> buffers,
    io_completion_handle &&fn) {
  std::string recv_key = RecvKey();
  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  // _channelRecv(recv_key, recv_queue, buffers, std::move(fn));
  auto recv_fn = std::bind(&TaskMessagePassInterface::_channelRecv, this,
                           std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4);

  auto recv_thread = std::thread(recv_fn, recv_key,
                                 std::ref(recv_queue), buffers, std::move(fn));
  recv_thread.detach();
}

void TaskMessagePassInterface::cancel() {
  this->cancel_.store(true);
}
}  // namespace primihub::network
