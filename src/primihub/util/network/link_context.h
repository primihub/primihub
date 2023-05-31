// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/util/threadsafe_queue.h"
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
  inline void setTaskInfo(const std::string& job_id,
                          const std::string& task_id,
                          const std::string& request_id) {
    job_id_ = job_id;
    task_id_ = task_id;
    request_id_ = request_id;
  }
  inline std::string job_id() const {
    return job_id_;
  }
  inline std::string task_id() const {
    return task_id_;
  }
  inline std::string request_id() const {
    return request_id_;
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
  primihub::common::CertificateConfig& getCertificateConfig() {return *cert_config_;}

  void initCertificate(const std::string& root_ca_path,
                      const std::string& key_path,
                      const std::string& cert_path) {
    cert_config_ = std::make_unique<primihub::common::CertificateConfig>(
        root_ca_path, key_path, cert_path);
  }

  retcode initCertificate(const primihub::common::CertificateConfig& cert_cfg) {
    cert_config_ = std::make_unique<primihub::common::CertificateConfig>(
        cert_cfg.rootCAPath(), cert_cfg.keyPath(), cert_cfg.certPath());
    return retcode::SUCCESS;
  }

  primihub::ThreadSafeQueue<std::string>&
  GetRecvQueue(const std::string& key = "default") {
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

  primihub::ThreadSafeQueue<std::string>&
  GetSendQueue(const std::string& key = "default") {
    std::unique_lock<std::mutex> lck(this->out_queue_mtx);
    auto it = out_data_queue.find(key);
    if (it != out_data_queue.end()) {
      return it->second;
    } else {
      return out_data_queue[key];
    }
  }

  primihub::ThreadSafeQueue<retcode>&
  GetCompleteQueue(const std::string& role = "default") {
    std::unique_lock<std::mutex> lck(this->complete_queue_mtx);
    auto it = complete_queue.find(role);
    if (it != complete_queue.end()) {
      return it->second;
    } else {
      return complete_queue[role];
    }
  }

  void Clean() {
    stop_.store(true);
    LOG(ERROR) << "stop all in data queue";
    {
      std::lock_guard<std::mutex> lck(in_queue_mtx);
      for(auto it = in_data_queue.begin(); it != in_data_queue.end(); ++it) {
          it->second.shutdown();
      }
    }
    LOG(ERROR) << "stop all out data queue";
    {
      std::lock_guard<std::mutex> lck(out_queue_mtx);
      for(auto it = out_data_queue.begin(); it != out_data_queue.end(); ++it) {
        it->second.shutdown();
      }
    }
    LOG(ERROR) << "stop all complete queue";
    {
      std::lock_guard<std::mutex> lck(complete_queue_mtx);
      for(auto it = complete_queue.begin(); it != complete_queue.end(); ++it) {
        it->second.shutdown();
      }
    }
  }

 protected:
  int32_t recv_timeout_ms_{-1};
  int32_t send_timeout_ms_{-1};
  std::shared_mutex connection_mgr_mtx;
  std::unordered_map<std::string, std::shared_ptr<IChannel>> connection_mgr;
  std::string job_id_;
  std::string task_id_;
  std::string request_id_;
  std::unique_ptr<primihub::common::CertificateConfig> cert_config_{nullptr};

  std::mutex in_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<std::string>> in_data_queue;
  std::mutex out_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<std::string>> out_data_queue;
  std::mutex complete_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<retcode>> complete_queue;
  std::atomic<bool> stop_{false};
};

class IChannel {
 public:
  IChannel() = default;
  explicit IChannel(LinkContext* link_ctx) : link_ctx_(link_ctx) {}
  virtual ~IChannel() = default;
  virtual retcode send(const std::string& key, const std::string& data) = 0;
  virtual retcode send(const std::string& key, std::string_view sv_data) = 0;
  virtual bool send_wrapper(const std::string& key, const std::string& data) = 0;
  virtual bool send_wrapper(const std::string& key, std::string_view sv_data) = 0;
  virtual retcode sendRecv(const std::string& key,
                           const std::string& send_data,
                           std::string* recv_data) = 0;
  virtual retcode sendRecv(const std::string& key,
                          std::string_view send_data,
                          std::string* recv_data) = 0;
  virtual retcode submitTask(const rpc::PushTaskRequest& request, rpc::PushTaskReply* reply) = 0;
  virtual retcode executeTask(const rpc::PushTaskRequest& request, rpc::PushTaskReply* reply) = 0;
  virtual retcode killTask(const rpc::KillTaskRequest& request, rpc::KillTaskResponse* reply) = 0;
  virtual retcode updateTaskStatus(const rpc::TaskStatus& request, rpc::Empty* reply) = 0;
  virtual retcode fetchTaskStatus(const rpc::TaskContext& request, rpc::TaskStatusReply* reply) = 0;
  virtual std::string forwardRecv(const std::string& key) = 0;
  LinkContext* getLinkContext() {
    return link_ctx_;
  }
 protected:
  LinkContext* link_ctx_{nullptr};
};



}  // namespace primihub::network

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
