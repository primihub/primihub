//
#include "src/primihub/util/network/link_context.h"
namespace primihub::network {
void LinkContext::Clean() {
  stop_.store(true);
  LOG(WARNING) << "stop all in data queue";
  {
    std::lock_guard<std::mutex> lck(in_queue_mtx);
    for(auto it = in_data_queue.begin(); it != in_data_queue.end(); ++it) {
        it->second.shutdown();
    }
  }
  LOG(WARNING) << "stop all out data queue";
  {
    std::lock_guard<std::mutex> lck(out_queue_mtx);
    for(auto it = out_data_queue.begin(); it != out_data_queue.end(); ++it) {
      it->second.shutdown();
    }
  }
  LOG(WARNING) << "stop all complete queue";
  {
    std::lock_guard<std::mutex> lck(complete_queue_mtx);
    for(auto it = complete_queue.begin(); it != complete_queue.end(); ++it) {
      it->second.shutdown();
    }
  }
}

LinkContext::StringDataQueue& LinkContext::GetRecvQueue(
    const std::string& key) {
  std::unique_lock<std::mutex> lck(this->in_queue_mtx);
  auto it = in_data_queue.find(key);
  if (it != in_data_queue.end()) {
    return it->second;
  } else {
    in_data_queue[key];
    if (stop_.load(std::memory_order::memory_order_relaxed)) {
      in_data_queue[key].shutdown();
    }
    return in_data_queue[key];
  }
}

LinkContext::StringDataQueue& LinkContext::GetSendQueue(
    const std::string& key) {
  std::unique_lock<std::mutex> lck(this->out_queue_mtx);
  auto it = out_data_queue.find(key);
  if (it != out_data_queue.end()) {
    return it->second;
  } else {
    return out_data_queue[key];
  }
}

LinkContext::StatusDataQueue& LinkContext::GetCompleteQueue(
    const std::string& key) {
  std::unique_lock<std::mutex> lck(this->complete_queue_mtx);
  auto it = complete_queue.find(key);
  if (it != complete_queue.end()) {
    return it->second;
  } else {
    return complete_queue[key];
  }
}

retcode LinkContext::Send(const std::string& key,
                          const Node& dest_node,
                          const std::string& send_buf) {
  std::string_view send_data_sv{send_buf.data(), send_buf.size()};
  return Send(key, dest_node, send_data_sv);
}

retcode LinkContext::Send(const std::string& key,
                          const Node& dest_node,
                          std::string_view send_buf_sv) {
  auto ch = getChannel(dest_node);
  return ch->send(key, send_buf_sv);
}

retcode LinkContext::Send(const std::string& key,
                          const Node& dest_node,
                          char* send_buf, size_t send_size) {
  std::string_view send_data_sv{send_buf, send_size};
  return Send(key, dest_node, send_data_sv);
}

retcode LinkContext::Recv(const std::string& key, std::string* recv_buf) {
  std::string recv_buf_tmp;
  auto& recv_queue = GetRecvQueue(key);
  recv_queue.wait_and_pop(recv_buf_tmp);
  *recv_buf = std::move(recv_buf_tmp);
  return retcode::SUCCESS;
}

retcode LinkContext::Recv(const std::string& key,
                          char* recv_buf, size_t recv_size) {
  std::string recv_buf_tmp;
  auto& recv_queue = GetRecvQueue(key);
  recv_queue.wait_and_pop(recv_buf_tmp);
  if (recv_size != recv_buf_tmp.size()) {
    LOG(ERROR) << "recv data does not match, expected: " << recv_size
        << " but get: " << recv_buf_tmp.size();
    return retcode::FAIL;
  }
  memcpy(recv_buf, recv_buf_tmp.data(), recv_size);
  return retcode::SUCCESS;
}

retcode LinkContext::SendRecv(const std::string& key,
                              const Node& dest_node,
                              std::string_view send_buf,
                              std::string* recv_buf) {
  auto channel = getChannel(dest_node);
  auto ret = channel->sendRecv(key, send_buf, recv_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "send data to peer: [" << dest_node.to_string()
        << "] failed";
    return ret;
  }
  return retcode::SUCCESS;
}

retcode LinkContext::SendRecv(const std::string& key,
                              const Node& dest_node,
                              const std::string& send_buf,
                              std::string* recv_buf) {
  auto send_buf_sv = std::string_view(send_buf.data(), send_buf.size());
  return SendRecv(key, dest_node, send_buf_sv, recv_buf);
}

retcode LinkContext::SendRecv(const std::string& key,
                              const Node& dest_node,
                              const char* send_buf,
                              size_t length,
                              std::string* recv_buf) {
  auto send_buf_sv = std::string_view(send_buf, length);
  return SendRecv(key, dest_node, send_buf_sv, recv_buf);
}

retcode LinkContext::SendRecv(const std::string& key,
                              const std::string& send_buf,
                              std::string* recv_buf) {
  std::string recv_buf_tmp;
  auto& recv_queue = this->GetRecvQueue(key);
  recv_queue.wait_and_pop(recv_buf_tmp);
  *recv_buf = std::move(recv_buf_tmp);
  if (HasStopped()) {
    LOG(ERROR) << "link context has been closed";
    return retcode::FAIL;
  }
  auto& send_queue = this->GetSendQueue(key);
  send_queue.push(send_buf);
  auto& complete_queue = this->GetCompleteQueue(key);
  retcode complete_flag;
  complete_queue.wait_and_pop(complete_flag);
  return retcode::SUCCESS;
}

} // namespace primihub::network
