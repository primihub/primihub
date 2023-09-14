// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_MESSAGE_INTERFACE_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_MESSAGE_INTERFACE_H_
#include <string>
#include <queue>
#include <memory>
#include <atomic>
#include <utility>

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Network/SocketAdapter.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/threadsafe_queue.h"

using io_completion_handle = osuCrypto::io_completion_handle;
using SocketInterface = osuCrypto::SocketInterface;
using primihub::ThreadSafeQueue;

namespace primihub::network {
class TaskMessagePassInterface : public SocketInterface {
 public:
  // TaskMessagePassInterface(const std::string &job_id,
  //                          const std::string &task_id,
  //                          const std::string &request_id,
  //                          const std::string &local_node_id,
  //                          const std::string &peer_node_id,
  //                          LinkContext *link_context,
  //                          std::shared_ptr<network::IChannel> channel) {
  //   job_id_ = job_id;
  //   task_id_ = task_id;
  //   request_id_ = request_id;
  //   local_node_id_ = local_node_id;
  //   peer_node_id_ = peer_node_id;
  //   link_context_ = link_context;
  //   send_channel_ = channel;
  //   cancel_ = false;

  //   send_count_.store(0);
  //   recv_count_.store(0);

  //   VLOG(3) << "job_id " << job_id_ << ", task_id " << task_id_
  //           << ", request_id " << request_id_
  //           << ", local_node " << local_node_id_ << ", peer node "
  //           << peer_node_id_;
  // }

  TaskMessagePassInterface(const std::string &local_node_id,
                           const std::string &peer_node_id,
                           LinkContext *link_context,
                           std::shared_ptr<network::IChannel> channel) {
    Init(local_node_id, peer_node_id, link_context, channel, channel);
  }

  TaskMessagePassInterface(const std::string &local_node_id,
                           const std::string &peer_node_id,
                           LinkContext *link_context,
                           std::shared_ptr<network::IChannel> send_channel,
                           std::shared_ptr<network::IChannel> recv_channel) {
    Init(local_node_id, peer_node_id, link_context, send_channel, recv_channel);
  }

  void Init(const std::string& local_party,
            const std::string remote_party,
            LinkContext* link_context,
            std::shared_ptr<network::IChannel> send_channel,
            std::shared_ptr<network::IChannel> recv_channel = nullptr) {
    job_id_ = link_context->job_id();
    task_id_ = link_context->task_id();
    request_id_ = link_context->request_id();
    sub_task_id_ =  link_context->sub_task_id();
    local_node_id_ = local_party;
    peer_node_id_ = remote_party;
    link_context_ = link_context;
    send_channel_ = std::move(send_channel);
    recv_channel_ = std::move(recv_channel);
    cancel_ = false;
    std::stringstream ss_send;
    ss_send << request_id_ << "_"
            << sub_task_id_ << "_"
            << local_node_id_ << "_"
            << peer_node_id_;
    send_key_ = ss_send.str();
    std::stringstream ss_recv;
    ss_recv << request_id_ << "_"
            << sub_task_id_ << "_"
            << peer_node_id_ << "_"
            << local_node_id_;
    recv_key_ = ss_recv.str();
    send_count_.store(0);
    recv_count_.store(0);
    VLOG(3) << "job_id " << job_id_ << ", task_id " << task_id_
            << ", request_id " << request_id_
            << ", local_node " << local_node_id_ << ", peer node "
            << peer_node_id_;
  }
  ~TaskMessagePassInterface() {}

  void async_recv(osuCrypto::span<boost::asio::mutable_buffer> buffers,
                  io_completion_handle &&fn) override;

  void async_send(osuCrypto::span<boost::asio::mutable_buffer> buffers,
                  io_completion_handle &&fn) override;

  void cancel();

 private:
  class WaitLock {
   public:
      void wait(void) {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock);
      }

      void notify(void) {
        cv_.notify_all();
      }

   private:
      std::mutex mu_;
      std::condition_variable cv_;
  };

  void _channelSend(const std::string send_key,
                    osuCrypto::span<boost::asio::mutable_buffer> buffers,
                    io_completion_handle &&fn);

  void _channelRecv(const std::string recv_key,
                    ThreadSafeQueue<std::string> &queue,
                    osuCrypto::span<boost::asio::mutable_buffer> buffers,
                    io_completion_handle &&fn);
  std::string& SendKey() {
    return send_key_;
  }
  std::string& RecvKey() {
    return recv_key_;
  }
  std::atomic<bool> cancel_{false};
  std::string job_id_;
  std::string task_id_;
  std::string request_id_;
  std::string sub_task_id_;
  std::string local_node_id_;
  std::string peer_node_id_;
  std::shared_ptr<network::IChannel> send_channel_;
  std::shared_ptr<network::IChannel> recv_channel_;
  network::LinkContext* link_context_{nullptr};
  // All receive threads must get a message in the order in which they start,
  // the queue and the mutex guarantee it.
  std::queue<std::shared_ptr<WaitLock>> recv_wait_queue_;
  std::mutex recv_queue_mu_;
  std::atomic_int send_count_{0};
  std::atomic_int recv_count_{0};
  std::string send_key_;
  std::string recv_key_;
};
}  // namespace primihub::network
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_MESSAGE_INTERFACE_H_
