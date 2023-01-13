#ifndef ABY3_CHANNEL_H_
#define ABY3_CHANNEL_H_

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/threadsafe_queue.h"

#include <atomic>
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
    status_.store(2);

    recv_fn_ = [this](ThreadSafeQueue<std::string> &recv_queue, char *recv_ptr,
                      uint64_t recv_size) -> int {
      int ret = 0;
      status_.fetch_add(2);

      do {
        ret = this->_channelRecv(recv_queue, recv_ptr, recv_size);
        if (ret == 0)
          break;
        else if (ret == 1)
          sleep(0.5);
        else
          break;
      } while ((status_.load() & 1) == 0);

      status_.fetch_sub(2);
      return ret;
    };
  }

  ~Aby3Channel() {
    // Set channel to invalid status.
    status_++;

    // Wait for async recv quit.
    do {
      sleep(0.5);
    } while (status_.load() != 3);
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
  template <typename T> std::future<int> asyncRecv(T &dest);

  // Recv value into matrix.
  template <typename T> std::future<int> asyncRecv(eMatrix<T> &dest);

  // Recv value into contiguous memory.
  template <typename T> std::future<int> asyncRecv(T *ptr, uint64_t elem_num);

private:
  template <typename T> inline void _channelSend(const T &val);

  template <typename T>
  inline void _channelSend(const T *ptr, uint64_t elem_num);

  inline int _channelRecv(ThreadSafeQueue<std::string> &queue, char *recv_ptr,
                          uint64_t recv_size);

  // Value of 'status_' indicate different channel status:
  // status_ == 2: channel is valid;
  // status_ % 2 == 1: channel is invalid;
  // (status_ > 2) && (status % 2 == 0): async recv runs in ths channel.
  std::atomic_uint32_t status_;

  std::function<int(ThreadSafeQueue<std::string> &, char *, uint64_t)> recv_fn_;

  std::string job_id_;
  std::string task_id_;
  std::string local_node_;
  std::string peer_node_;

  std::shared_ptr<network::IChannel> channel_;
  GetRecvQueueFunc get_queue_func_;
};

int Aby3Channel::_channelRecv(ThreadSafeQueue<std::string> &queue,
                              char *recv_ptr, uint64_t recv_size) {
  std::string recv_str;
  if (queue.try_pop(recv_str) == false) {
    VLOG(5) << "Recv queue 0x" << std::hex << reinterpret_cast<uint64_t>(&queue)
            << " is empty.";
    return 1;
  }

  if (recv_str.size() != recv_size) {
    LOG(ERROR) << "Recv buffer size mismatch, expect " << recv_str.size()
               << " bytes, gives " << recv_size << " bytes.";
    return -1;
  }

  memcpy(recv_ptr, recv_str.c_str(), recv_size);

  VLOG(5) << "Recv finish, recv size " << recv_size << ", recv queue 0x"
          << std::hex << reinterpret_cast<uint64_t>(&queue) << ".";

  return 0;
}

template <typename T> void Aby3Channel::_channelSend(const T &val) {
  std::stringstream ss;
  ss << job_id_ << "_" << task_id_ << "_" << local_node_ << "_" << peer_node_;
  std::string key = ss.str();

  std::string_view send_str(reinterpret_cast<const char *>(&val), sizeof(T));

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

template <typename T> std::future<int> Aby3Channel::asyncRecv(T &dest) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support recv int64_t value.");

  if ((status_.load() & 1) != 0)
    throw std::runtime_error("Forbid recv due to invalid channel.");

  if (status_.load() != 2)
    throw std::runtime_error("Don't support parallel recv.");

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_"
         << local_node_;

  std::string recv_key = key_ss.str();
  auto &recv_queue = get_queue_func_(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(&dest);
  uint64_t recv_size = sizeof(T);

  return std::async(std::launch::async, recv_fn_, std::ref(recv_queue),
                    recv_ptr, recv_size);
}

template <typename T>
std::future<int> Aby3Channel::asyncRecv(eMatrix<T> &dest) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support int64_t matrix.");

  if (VLOG_IS_ON(5)) {
    const std::type_info &r = typeid(T);
    VLOG(5) << "Type of T " << r.name() << ", sizeof T " << sizeof(T)
            << ", elem_num " << dest.size() << ".";
  }

  if ((status_.load() & 1) != 0)
    throw std::runtime_error("Forbid recv due to invalid channel.");

  if (status_.load() != 2)
    throw std::runtime_error("Don't support parallel recv.");

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_"
         << local_node_;

  std::string recv_key = key_ss.str();
  auto &recv_queue = get_queue_func_(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(dest.data());
  uint64_t recv_size = dest.size() * sizeof(T);

  return std::async(std::launch::async, recv_fn_, std::ref(recv_queue),
                    recv_ptr, recv_size);
}

template <typename T>
std::future<int> Aby3Channel::asyncRecv(T *ptr, uint64_t elem_num) {
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

  std::string recv_key = key_ss.str();
  auto &recv_queue = get_queue_func_(recv_key);
  char *recv_ptr = reinterpret_cast<char *>(ptr);
  uint64_t recv_size = elem_num * sizeof(T);

  if ((status_.load() & 1) != 0)
    throw std::runtime_error("Forbid recv due to invalid channel.");

  if (status_.load() != 2)
    throw std::runtime_error("Don't support parallel recv.");

  return std::async(std::launch::async, recv_fn_, std::ref(recv_queue),
                    recv_ptr, recv_size);
}

}; // namespace primihub
#endif
