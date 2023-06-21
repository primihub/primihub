// "Copyright [2023] <Primihub>"
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_MPC_CHANNEL_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_MPC_CHANNEL_H_

#include <glog/logging.h>

#include <future>
#include <string_view>
#include <atomic>
#include <sstream>
#include <memory>
#include <vector>
#include <string>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/util/util.h"

namespace primihub {
std::string BinToHex(const std::string_view &strBin, bool bIsUpper = false);
std::string HexToBin(const std::string &strHex);

class MpcChannel {
 public:
  MpcChannel() {}

  MpcChannel(const std::string &job_id, const std::string &task_id,
             const std::string &local_party_name,
             network::LinkContext *link_context) {
    local_party_name_ = local_party_name;
    task_id_ = task_id;
    job_id_ = job_id;
    link_context_ = link_context;
  }

  MpcChannel(const std::string &local_party_name,
             network::LinkContext *link_context) {
    local_party_name_ = local_party_name;
    task_id_ = link_context->task_id();
    job_id_ = link_context->job_id();
    request_id_ = link_context->request_id();
    link_context_ = link_context;
  }

  ~MpcChannel() {}

  MpcChannel& operator=(const MpcChannel &another) {
    this->local_party_name_ = another.local_party_name_;
    this->peer_party_name_ = another.peer_party_name_;
    this->job_id_ = another.job_id_;
    this->task_id_ = another.task_id_;
    this->request_id_ = another.request_id_;
    this->channel_ = another.channel_;
    this->link_context_ = another.link_context_;

    return *this;
  }

  MpcChannel(const MpcChannel& another) {
    this->local_party_name_ = another.local_party_name_;
    this->peer_party_name_ = another.peer_party_name_;
    this->job_id_ = another.job_id_;
    this->task_id_ = another.task_id_;
    this->request_id_ = another.request_id_;
    this->channel_ = another.channel_;
    this->link_context_ = another.link_context_;
  }

  void SetupBaseChannel(const std::string& peer_party_name_,
                        std::shared_ptr<network::IChannel> channel);

  std::string KeyPrefix() {
    std::stringstream ss;
    ss << job_id_ << "_" << task_id_ << "_" << request_id_ << "_";
    return ss.str();
  }

  std::string SendKey() {
    std::stringstream ss;
    ss << KeyPrefix() << local_party_name_ << "_" << peer_party_name_;
    return ss.str();
  }

  std::string RecvKey() {
    std::stringstream ss;
    ss << KeyPrefix() << peer_party_name_  << "_" << local_party_name_;
    return ss.str();
  }

  template <typename T>
  void asyncSendCopy(const T &val);

  template <typename T>
  void asyncSend(const T &val);

  template <typename T>
  void asyncSendCopy(const eMatrix<T> &c);

  template <typename T>
  void asyncSend(const eMatrix<T> &c);

  template <typename T>
  void asyncSendCopy(span<T> &c);

  template <typename T>
  void asyncSend(const T *c, uint64_t elem_num);

  template <typename T>
  void asyncSendCopy(const T *c, uint64_t elem_num);

  template <typename T>
  void asyncSend(const std::vector<T> &c);

  template <typename T>
  void send(const T *c, uint64_t elem_num);

  template <typename T>
  void send(const T &val);

  template <typename T>
  std::future<void> asyncRecv(std::vector<T> &dest);

  template <typename T>
  std::future<void> asyncRecv(T &dest);

  template <typename T>
  std::future<void> asyncRecv(eMatrix<T> &dest);

  template <typename T>
  std::future<void> asyncRecv(T *ptr, uint64_t elem_num);

  template <typename T>
  std::future<void> asyncRecv(T *ptr, uint64_t elem_num, std::function<void()> cb);

  template <typename T>
  std::future<void> asyncRecv(span<T> &c);

  template <typename T>
  void recv(T *ptr, uint64_t size);

  template <typename T>
  void recv(T &val);

  const std::string& peer_party_name() const { return peer_party_name_; }
  const std::string& self_party_name() const { return local_party_name_; }

 private:
  template <typename T> void _channelSend(const T &val);

  template <typename T> void _channelSend(const T *ptr, uint64_t elem_num);

  int _channelRecvNonBlock(ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                           uint64_t recv_size, const std::string &recv_key);

  int _channelRecvNonBlock(ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                           uint64_t recv_size, const std::string &recv_key,
                           std::function<void()> done);

  std::string job_id_;
  std::string task_id_;
  std::string request_id_;

  std::string local_party_name_;
  std::string peer_party_name_;

  network::LinkContext* link_context_{nullptr};
  std::shared_ptr<network::IChannel> channel_{nullptr};
};

template <typename T> void MpcChannel::_channelSend(const T &val) {
  std::string send_key = SendKey();
  std::string_view send_str(reinterpret_cast<const char *>(&val), sizeof(T));
  auto ret = channel_->send(send_key, send_str);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send val to " << peer_party_name_ << " failed, send key " << send_key << ".";
    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }
}

template <typename T>
void MpcChannel::_channelSend(const T *ptr, uint64_t elem_num) {
  std::string send_key = SendKey();
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << elem_num << ". send key: " << send_key;
  }

  const char *raw_ptr = reinterpret_cast<const char *>(ptr);
  uint64_t raw_size = elem_num * sizeof(T);

  std::string_view send_str(reinterpret_cast<const char *>(raw_ptr), raw_size);

  VLOG(5) << "Send key is " << send_key << ", send length " << raw_size
          << " bytes.";

  auto ret = channel_->send(send_key, send_str);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send value in pointer 0x" << std::hex
       << reinterpret_cast<uint64_t>(raw_ptr) << " to " << peer_party_name_
       << " failed, send size " << raw_size;
    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }
}

template <typename T> void MpcChannel::asyncSendCopy(const T &val) {
  VLOG(5) << "Type of send value is " << typeid(val).name() << ".";
  _channelSend(val);
}

template <typename T> void MpcChannel::asyncSend(const T &val) {
  VLOG(5) << "Type of send value is " << typeid(val).name() << ".";
  _channelSend(val);
}

template <typename T> void MpcChannel::asyncSend(const eMatrix<T> &c) {
  _channelSend(c.data(), c.size());
}

template <typename T> void MpcChannel::asyncSendCopy(const eMatrix<T> &c) {
  _channelSend(c.data(), c.size());
}

template <typename T>
void MpcChannel::asyncSend(const T *val, uint64_t elem_num) {
  VLOG(5) << "Type of send value is " << typeid(T).name() << ".";
  _channelSend(val, elem_num);
}

template <typename T> void MpcChannel::asyncSend(const std::vector<T> &c) {
  _channelSend(c.data(), c.size());
}

template <typename T>
void MpcChannel::asyncSendCopy(const T *val, uint64_t elem_num) {
  VLOG(5) << "Type of send value is " << typeid(val).name() << ".";
  _channelSend(val, elem_num);
}

template <typename T> void MpcChannel::asyncSendCopy(span<T> &c) {
  VLOG(5) << "Type of send value is " << typeid(c).name() << ".";
  _channelSend(c.data(), c.size());
}

template <typename T> void MpcChannel::send(const T *ptr, uint64_t elem_num) {
  _channelSend(ptr, elem_num);
}

template <typename T> void MpcChannel::send(const T &val) {
  _channelSend(&val, 1);
}

template <typename T>
std::future<void> MpcChannel::asyncRecv(std::vector<T> &dest) {
  std::string recv_key = RecvKey();
  if (VLOG_IS_ON(5)) {
    VLOG(5) << "Recv type of T is " << typeid(dest).name() << ", recv size "
            << dest.size() << ". recv key: " << recv_key;
  }

  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(dest.data());
  uint64_t recv_size = dest.size();

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key);
}

template <typename T> std::future<void> MpcChannel::asyncRecv(T &dest) {
  std::string recv_key = RecvKey();
  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(&dest);
  uint64_t recv_size = sizeof(T);

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key);
}

template <typename T>
std::future<void> MpcChannel::asyncRecv(eMatrix<T> &dest) {
  std::string recv_key = RecvKey();
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << dest.size() << ". recv key: " << recv_key;
  }

  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(dest.data());
  uint64_t recv_size = dest.size() * sizeof(T);

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key);
}

template <typename T>
std::future<void> MpcChannel::asyncRecv(T *ptr, uint64_t elem_num) {
  std::string recv_key = RecvKey();
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << elem_num << ". recv key: " << recv_key;
  }

  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(ptr);
  uint64_t recv_size = elem_num * sizeof(T);

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key);
}

template <typename T>
std::future<void> MpcChannel::asyncRecv(T *ptr, uint64_t elem_num,
                                        std::function<void()> done) {
  std::string recv_key = RecvKey();
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << elem_num << ". recv key: " << recv_key;
  }

  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(ptr);
  uint64_t recv_size = elem_num * sizeof(T);

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key,
                     std::function<void()> done) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key, done);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key,
                    done);
}

template <typename T> std::future<void> MpcChannel::asyncRecv(span<T> &c) {
  std::string recv_key = RecvKey();
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << c.size() << ". recv key: " << recv_key;
  }
  auto &recv_queue = link_context_->GetRecvQueue(recv_key);
  char *recv_ptr = reinterpret_cast<char*>(c.data());
  uint64_t recv_size = c.size() * sizeof(T);

  auto recv_fn = [&](ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                     uint64_t recv_size, const std::string &recv_key) {
    _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  };

  return std::async(std::launch::async,
                    recv_fn,
                    std::ref(recv_queue),
                    recv_ptr,
                    recv_size,
                    recv_key);
}

template <typename T> void MpcChannel::recv(T *ptr, uint64_t size) {
  asyncRecv(ptr, size).get();
}

template <typename T> void MpcChannel::recv(T &val) {
  asyncRecv(&val, 1).get();
}

};  // namespace primihub
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_MPC_CHANNEL_H_
