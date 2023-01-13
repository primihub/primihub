// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#include "src/primihub/common/defines.h"
#include "src/primihub/protos/worker.pb.h"
#include <string_view>
#include <string>
#include <unordered_map>
#include <shared_mutex>
namespace primihub::network {
namespace rpc = primihub::rpc;
class IChannel;
/**
 * link connection manager
 * manage channel create by node info
 * notic: link context is bind to task
 * each task has one link context
*/
class LinkContext {
 public:
  LinkContext() = default;
  virtual ~LinkContext() = default;
  inline void setTaskInfo(const std::string& job_id, const std::string& task_id) {
    job_id_ = job_id;
    task_id_ = task_id;
  }
  inline std::string job_id() {
    return job_id_;
  }
  inline std::string task_id() {
    return task_id_;
  }
  /**
   * return channel for specified node
   * if channel is not exist, create
  */
  virtual std::shared_ptr<IChannel> getChannel(const primihub::Node& node) = 0;
  void setRecvTimeout(int32_t recv_timeout_ms) {recv_timeout_ms_ = recv_timeout_ms;}
  void setSendTimeout(int32_t send_timeout_ms) {send_timeout_ms_ = send_timeout_ms;}
  int32_t sendTimeout() const {return send_timeout_ms_;}
  int32_t recvTimeout() const {return recv_timeout_ms_;}
 protected:
  int32_t recv_timeout_ms_{-1};
  int32_t send_timeout_ms_{-1};
  std::shared_mutex connection_mgr_mtx;
  std::unordered_map<std::string, std::shared_ptr<IChannel>> connection_mgr;
  std::string job_id_;
  std::string task_id_;
};

class IChannel {
 public:
  IChannel() = default;
  explicit IChannel(LinkContext* link_ctx) : link_ctx_(link_ctx) {}
  virtual ~IChannel() = default;
  virtual retcode send(const std::string& role, const std::string& data) = 0;
  virtual retcode send(const std::string& role, std::string_view sv_data) = 0;
  virtual int send_wrapper(const std::string& role, const std::string& data) = 0;
  virtual int send_wrapper(const std::string& role, std::string_view sv_data) = 0;
  virtual retcode sendRecv(const std::string& role, const std::string& send_data, std::string* recv_data) = 0;
  virtual retcode sendRecv(const std::string& role, std::string_view send_data, std::string* recv_data) = 0;
  virtual retcode submitTask(const rpc::PushTaskRequest& request, rpc::PushTaskReply* reply) = 0;
  virtual std::string forwardRecv(const std::string& role) = 0;
  // virtual retcode send(const std::vector<std::string>& datas) = 0;
  // virtual retcode send(const std::vector<std::string_view> sv_datas) = 0;
  // virtual retcode send(const rpc::TaskRequest& data) = 0;
  // virtual retcode send(const std::vector<rpc::TaskRequest>& data) = 0;
  // virtual retcode recv(const rpc::TaskRequest& request, std::vector<rpc::TaskResponse>* datas) = 0;
  // virtual retcode sendRecv(const std::vector<rpc::TaskRequest>& senddata,
  //                          std::vector<rpc::TaskResponse>* recv_data) = 0;
  LinkContext* getLinkContext() {
    return link_ctx_;
  }
 protected:
  LinkContext* link_ctx_{nullptr};

};



}  // namespace primihub::network

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
