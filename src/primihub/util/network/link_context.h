// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#include <string_view>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include "arrow/api.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/util/threadsafe_queue.h"

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
  using StringDataQueue = primihub::ThreadSafeQueue<std::string>;
  using StringDataContainer = std::unordered_map<std::string, StringDataQueue>;
  using StatusDataQueue = primihub::ThreadSafeQueue<retcode>;
  using StatusDataContainer = std::unordered_map<std::string, StatusDataQueue>;
  LinkContext() = default;
  virtual ~LinkContext() = default;
  inline void setTaskInfo(const std::string& job_id,
                          const std::string& task_id,
                          const std::string& request_id,
                          const std::string& sub_task_id) {
    job_id_ = job_id;
    task_id_ = task_id;
    request_id_ = request_id;
    sub_task_id_ = sub_task_id;
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

  inline std::string sub_task_id() const {
    return sub_task_id_;
  }
  /**
   * return channel for specified node
   * if channel is not exist, create
  */
  virtual std::shared_ptr<IChannel> getChannel(const primihub::Node& node) = 0;
  inline void setRecvTimeout(const int32_t recv_timeout_ms) {
    recv_timeout_ms_ = recv_timeout_ms;
  }
  inline void setSendTimeout(const int32_t send_timeout_ms) {
    send_timeout_ms_ = send_timeout_ms;
  }
  int32_t sendTimeout() const {return send_timeout_ms_;}
  int32_t recvTimeout() const {return recv_timeout_ms_;}
  primihub::common::CertificateConfig& getCertificateConfig() {
    return *cert_config_;
  }

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

  StringDataQueue& GetRecvQueue(const std::string& key = "default");
  StringDataQueue& GetSendQueue(const std::string& key = "default");
  StatusDataQueue& GetCompleteQueue(const std::string& role = "default");

  void Clean();
  retcode Send(const std::string& key,
               const Node& dest_node, const std::string& send_buf);
  retcode Send(const std::string& key,
               const Node& dest_node, std::string_view send_buf);
  retcode Send(const std::string& key,
               const Node& dest_node, char* send_buf, size_t send_size);
  retcode Recv(const std::string& key, std::string* recv_buf);
  retcode Recv(const std::string& key, char* recv_buf, size_t recv_size);
  retcode Recv(const std::string& key,
               const Node& dest_node, std::string* recv_buf);
  retcode Recv(const std::string& key,
               const Node& dest_node, char* recv_buf, size_t recv_size);
  /**
   * sender to process send recv
  */
  retcode SendRecv(const std::string& key,
                   const Node& dest_node,
                   const std::string& send_buf,
                   std::string* recv_buf);
  retcode SendRecv(const std::string& key,
                   const Node& dest_node,
                   std::string_view send_buf,
                   std::string* recv_buf);
  retcode SendRecv(const std::string& key,
                   const Node& dest_node,
                   const char* send_buf, size_t length,
                   std::string* recv_buf);
  /**
   * receiver to process send recv
  */
  retcode SendRecv(const std::string& key,
                   const std::string& send_buf,
                   std::string* recv_buf);


 protected:
  bool HasStopped() {
    return stop_.load(std::memory_order::memory_order_relaxed);
  }
  int32_t recv_timeout_ms_{-1};
  int32_t send_timeout_ms_{-1};
  std::shared_mutex connection_mgr_mtx;
  std::unordered_map<std::string, std::shared_ptr<IChannel>> connection_mgr;
  std::string job_id_;
  std::string task_id_;
  std::string request_id_;
  std::string sub_task_id_;
  std::unique_ptr<primihub::common::CertificateConfig> cert_config_{nullptr};

  std::mutex in_queue_mtx;
  StringDataContainer in_data_queue;

  std::mutex out_queue_mtx;
  StringDataContainer out_data_queue;

  std::mutex complete_queue_mtx;
  StatusDataContainer complete_queue;
  std::atomic<bool> stop_{false};
};

class IChannel {
 public:
  IChannel() = default;
  explicit IChannel(LinkContext* link_ctx) : link_ctx_(link_ctx) {}
  virtual ~IChannel() = default;
  virtual retcode send(const std::string& key, const std::string& data) = 0;
  virtual retcode send(const std::string& key, std::string_view sv_data) = 0;
  virtual bool send_wrapper(const std::string& key,
                            const std::string& data) = 0;
  virtual bool send_wrapper(const std::string& key,
                            std::string_view sv_data) = 0;
  virtual retcode sendRecv(const std::string& key,
                           const std::string& send_data,
                           std::string* recv_data) = 0;
  virtual retcode sendRecv(const std::string& key,
                           std::string_view send_data,
                           std::string* recv_data) = 0;
  virtual retcode submitTask(const rpc::PushTaskRequest& request,
                             rpc::PushTaskReply* reply) = 0;
  virtual retcode executeTask(const rpc::PushTaskRequest& request,
                              rpc::PushTaskReply* reply) = 0;
  virtual retcode killTask(const rpc::KillTaskRequest& request,
                           rpc::KillTaskResponse* reply) = 0;
  virtual retcode updateTaskStatus(const rpc::TaskStatus& request,
                                   rpc::Empty* reply) = 0;
  virtual retcode fetchTaskStatus(const rpc::TaskContext& request,
                                  rpc::TaskStatusReply* reply) = 0;
  virtual retcode StopTask(const rpc::TaskContext& request,
                           rpc::Empty* reply) = 0;
  virtual retcode DownloadData(const rpc::DownloadRequest& request,
                               std::vector<std::string>* data) = 0;
  virtual std::shared_ptr<arrow::Table>
  FetchData(const std::string& request_id,
            const std::string& dataset_id,
            std::shared_ptr<arrow::Schema> schema) = 0;
  virtual std::string forwardRecv(const std::string& key) = 0;
  LinkContext* getLinkContext() { return link_ctx_; }

 protected:
  LinkContext* link_ctx_{nullptr};
};



}  // namespace primihub::network

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
