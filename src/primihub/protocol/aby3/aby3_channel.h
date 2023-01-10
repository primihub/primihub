#ifndef ABY3_CHANNEL_H_
#define ABY3_CHANNEL_H_

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/socket/channel.h"

#include <string_view>

namespace primihub {
class Aby3Channel {
public:
  Aby3Channel(const std::string &job_id, const std::string &task_id,
              const std::string &local_node_id) {
    local_node_ = local_node_id;
    task_id_ = task_id;
    job_id_ = job_id;
  }

  Aby3Channel() = delete;

  void SetupCommChannel(const std::string &peer_node_id,
                        std::shared_ptr<network::IChannel> channel);

  // Send a single value.
  template <typename T> void asyncSendCopy(const T &val);
  template <typename T> void asyncSend(const T &val);

  // Send value in matrix.
  template <typename T> void asyncSendCopy(const eMatrix<T> &c);
  template <typename T> void asyncSend(const eMatrix<T> &c);

  // Send value in contiguous memory.
  template <typename T> void asyncSend(const T *c, uint64_t elem_num);
  template <typename T> void asyncSendCopy(const T *c, uint64_t elem_num);

private:
  template <typename T> inline void _channelSend(const T &val);
  template <typename T>
  inline void _channelSend(const T *ptr, uint64_t elem_num);

  std::string job_id_;
  std::string task_id_;
  std::string local_node_;
  std::string peer_node_;
  std::shared_ptr<network::IChannel> channel_;
};

template <typename T> void Aby3Channel::_channelSend(const T &val) {
  std::stringstream ss;
  ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_send";
  std::string key = ss.str();

  std::string send_str = std::to_string(val);

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
  const char *raw_ptr = reinterpret_cast<const char *>(ptr);
  uint64_t raw_size = elem_num * sizeof(T);

  std::string_view str(raw_ptr, raw_size);

  std::stringstream key_ss;
  key_ss << job_id_ << "_" << task_id_ << "_" << peer_node_ << "_send";

  auto ret = channel_->send(key_ss.str(), str);
  if (ret != retcode::SUCCESS) {
    std::stringstream ss;
    ss << "Send value in pointer 0x" << std::hex
       << reinterpret_cast<uint64_t>(raw_ptr) << " to " << peer_node_
       << " failed, send size " << raw_size;
    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }

  VLOG(5) << "Issue send finish, pointer " << std::hex
          << reinterpret_cast<uint64_t>(raw_ptr) << ", send size " << raw_size
          << ".";
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

  _channelSend(reinterpret_cast<char *>(c.data()), c.size());
}

template <typename T> void Aby3Channel::asyncSendCopy(const eMatrix<T> &c) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t matrix .");

  _channelSend(reinterpret_cast<char *>(c.data()), c.size());
}

template <typename T>
void Aby3Channel::asyncSend(const T *val, uint64_t elem_num) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t array.");

  _channelSend(reinterpret_cast<char *>(val), elem_num);
}

template <typename T>
void Aby3Channel::asyncSendCopy(const T *val, uint64_t elem_num) {
  if (!(std::is_same<T, std::int64_t>::value))
    throw std::runtime_error("Only support send int64_t array.");

  _channelSend(reinterpret_cast<char *>(val), elem_num);
}

}; // namespace primihub
#endif
