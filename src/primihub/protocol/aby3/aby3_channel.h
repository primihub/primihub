#ifndef ABY3_CHANNEL_H_
#define ABY3_CHANNEL_H_

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/threadsafe_queue.h"

#include <sstream>
#include <string_view>

namespace primihub {
class Aby3Channel {
public:
  Aby3Channel(const std::string &job_id, const std::string &task_id,
              const std::string &local_node_id) {
    local_node_ = local_node_id;
    task_id_ = task_id;
    job_id_ = job_id;
    counter_ = 0;
  }

  using GetRecvQueueFunc =
      std::function<ThreadSafeQueue<std::string> &(const std::string &key)>;

  Aby3Channel() = delete;

  void SetupCommChannel(const std::string &peer_node_id,
                        std::shared_ptr<network::IChannel> channel,
                        GetRecvQueueFunc get_func);

  // Send a single value.
  template <typename T> void asyncSendCopy(const T &val);
  template <typename T> void asyncSend(const T &val);

  // Send value in matrix.
  template <typename T> void asyncSendCopy(const eMatrix<T> &c);
  template <typename T> void asyncSend(const eMatrix<T> &c);

  // Send value in contiguous memory.
  template <typename T> void asyncSend(const T *c, uint64_t elem_num);
  template <typename T> void asyncSendCopy(const T *c, uint64_t elem_num);

  // Recv a single value.
  template <typename T> void asyncRecv(T &dest);

  // Recv value into matrix.
  template <typename T> void asyncRecv(eMatrix<T> &dest);

  // Recv value into contiguous memory.
  template <typename T> void asyncRecv(T *ptr, uint64_t elem_num);

private:
  template <typename T> inline void _channelSend(const T &val);

  template <typename T>
  inline void _channelSend(const T *ptr, uint64_t elem_num);

  inline int _channelRecv(ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                          uint64_t recv_size);

  inline uint64_t _GetCounterValue(void);

  std::string job_id_;
  std::string task_id_;
  std::string local_node_;
  std::string peer_node_;
  std::shared_ptr<network::IChannel> channel_;
  GetRecvQueueFunc get_queue_func_;
  std::mutex counter_mu_;
  uint64_t counter_;
};

uint64_t Aby3Channel::_GetCounterValue(void) {
  uint64_t counter_val = 0;
  counter_mu_.lock();
  counter_val = counter_++;
  counter_mu_.unlock();
  return counter_val;
}

int Aby3Channel::_channelRecv(ThreadSafeQueue<std::string> &queue,
                              char *recv_ptr, uint64_t recv_size) {
  std::string recv_str;
  queue.wait_and_pop(recv_str);

  if (recv_str.size() != recv_size) {
    LOG(ERROR) << "Recv buffer size mismatch, expect " << recv_str.size()
               << " bytes, gives " << recv_size << " bytes.";
    return -1;
  }

  memcpy(recv_ptr, recv_str.c_str(), recv_size);
  return 0;
}

template <typename T> void Aby3Channel::_channelSend(const T &val) {
  std::stringstream ss;
  ss << job_id_ << "_" << task_id_ << "_" << local_node_ << "_" << peer_node_;
  std::string key = ss.str();
  
  std::string_view send_str(reinterpret_cast<const char*>(&val), sizeof(T));

  auto ret = channel_->send(key, send_str);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send i64 val to " << peer_node_ << " failed, send key " << key
       << ".";

    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }

  VLOG(5) << "Issue send finish, send value " << val << ", send key " << key
          << ".";
}

template <typename T>
void Aby3Channel::_channelSend(const T *ptr, uint64_t elem_num) {
  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << elem_num << ".";
  }

  const char *raw_ptr = reinterpret_cast<const char *>(ptr);
  uint64_t raw_size = elem_num * sizeof(T);

  std::string_view str(raw_ptr, raw_size);

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << local_node_ << "_"
         << peer_node_;

  LOG(INFO) << "Send key is " << key_ss.str() << ", send length " << raw_size
            << " bytes.";

  auto ret = channel_->send(key_ss.str(), str);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send value in pointer 0x" << std::hex
       << reinterpret_cast<uint64_t>(raw_ptr) << " to " << peer_node_
       << " failed, send size " << raw_size;
    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }
}

template <typename T> void Aby3Channel::asyncSendCopy(const T &val) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t value now.");

  _channelSend(val);
}

template <typename T> void Aby3Channel::asyncSend(const T &val) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t value now.");

  _channelSend(val);
}

template <typename T> void Aby3Channel::asyncSend(const eMatrix<T> &c) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t matrix .");

  _channelSend(c.data(), c.size());
}

template <typename T> void Aby3Channel::asyncSendCopy(const eMatrix<T> &c) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t matrix .");

  _channelSend(c.data(), c.size());
}

template <typename T>
void Aby3Channel::asyncSend(const T *val, uint64_t elem_num) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t array.");

  _channelSend(val, elem_num);
}

template <typename T>
void Aby3Channel::asyncSendCopy(const T *val, uint64_t elem_num) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t array.");

  _channelSend(val, elem_num);
}

template <typename T> void Aby3Channel::asyncRecv(T &dest) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support recv int64_t value.");

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_"
         << local_node_;

  auto &recv_queue = get_queue_func_(key_ss.str());
  char *recv_ptr = reinterpret_cast<char *>(&dest);
  uint64_t recv_size = sizeof(T);

  VLOG(5) << "Recv start, recv key " << key_ss.str() << ".";

  int ret = _channelRecv(recv_queue, recv_ptr, recv_size);
  if (ret) {
    LOG(ERROR) << "Recv failed, buffer size mismatch.";
    throw std::runtime_error("Recv failed, buffer size mismatch.");
  }

  VLOG(5) << "Recv finish, recv key " << key_ss.str() << ".";
}

template <typename T> void Aby3Channel::asyncRecv(eMatrix<T> &dest) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support int64_t matrix.");

  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << dest.size() << ".";
  }

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_"
         << local_node_;

  auto &recv_queue = get_queue_func_(key_ss.str());
  char *recv_ptr = reinterpret_cast<char *>(dest.data());
  uint64_t recv_size = dest.size() * sizeof(T);

  VLOG(5) << "Recv start, recv key " << key_ss.str() << ".";

  int ret = _channelRecv(recv_queue, recv_ptr, recv_size);
  if (ret) {
    LOG(ERROR) << "Recv failed, buffer size mismatch.";
    throw std::runtime_error("Recv failed, buffer size mismatch.");
  }

  VLOG(5) << "Recv finish, recv key " << key_ss.str() << ".";
}

template <typename T> void Aby3Channel::asyncRecv(T *ptr, uint64_t elem_num) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support int64_t matrix.");

  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << elem_num << ".";
  }

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_"
         << local_node_;

  auto &recv_queue = get_queue_func_(key_ss.str());
  char *recv_ptr = reinterpret_cast<char *>(ptr);
  uint64_t recv_size = elem_num * sizeof(T);

  VLOG(5) << "Recv start, recv key " << key_ss.str() << ".";

  int ret = _channelRecv(recv_queue, recv_ptr, recv_size);
  if (ret) {
    LOG(ERROR) << "Recv failed, buffer size mismatch.";
    throw std::runtime_error("Recv failed, buffer size mismatch.");
  }

  VLOG(5) << "Recv finish, recv key " << key_ss.str() << ".";
}

}; // namespace primihub
#endif
