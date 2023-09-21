/*
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef SRC_PRIMIHUB_UTIL_NETWORK_MEM_CHANNEL_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_MEM_CHANNEL_H_
#include <glog/logging.h>
#include <string>
#include <queue>
#include <memory>
#include <atomic>
#include <map>
#include <utility>
#include "src/primihub/util/threadsafe_queue.h"
#include "network/base_channel.h"

using primihub::ThreadSafeQueue;
namespace ph_link = primihub::link;
namespace primihub::network {
using StorageType = std::map<std::string, ThreadSafeQueue<std::string>>;
class SimpleMemoryChannel : public ph_link::ChannelBase {
 public:
  SimpleMemoryChannel(const std::string& job_id,
                      const std::string& task_id,
                      const std::string& request_id,
                      const std::string& local_node_id,
                      const std::string& peer_node_id,
                      std::shared_ptr<StorageType> storage) {
    job_id_ = job_id;
    task_id_ = task_id;
    request_id_ = request_id;
    local_node_id_ = local_node_id;
    peer_node_id_ = peer_node_id;

    cancel_ = false;
    std::stringstream ss_send;
    ss_send << request_id_ << "_" << local_node_id_ << "_" << peer_node_id_;
    send_key_ = ss_send.str();
    std::stringstream ss_recv;
    ss_recv << request_id_ << "_" << peer_node_id_ << "_" << local_node_id_;
    recv_key_ = ss_recv.str();

    VLOG(3) << "job_id " << job_id_ << ", task_id " << task_id_ << ", "
            << "request_id " << request_id_ << ", "
            << "local_node " << local_node_id_ << ", peer node "
            << peer_node_id_;
    if (storage == nullptr) {
      std::string err_info = "storage is invalid";
      LOG(ERROR) << err_info;
      throw std::runtime_error(err_info);
    }
    storage_ = storage;
    // create key for send recv
    LOG(INFO) << "create storage for send key: " << send_key_;
    (*storage)[send_key_];
    LOG(INFO) << "create storage for recv key: " << recv_key_;
    (*storage)[recv_key_];
  }

  ~SimpleMemoryChannel() = default;
  ph_link::retcode SendImpl(const std::string& send_buf) override;
  ph_link::retcode SendImpl(std::string_view send_buff_sv) override;
  ph_link::retcode SendImpl(const char* buff, size_t size) override;
  ph_link::retcode RecvImpl(std::string* recv_buf) override;
  ph_link::retcode RecvImpl(char* recv_buf, size_t recv_size) override;
  void close() override;
  void cancel() override;

 private:
  std::atomic<bool> cancel_{false};
  std::string job_id_;
  std::string task_id_;
  std::string request_id_;
  std::string local_node_id_;
  std::string peer_node_id_;

  std::string send_key_;
  std::string recv_key_;
  std::shared_ptr<StorageType> storage_{nullptr};
};
}  // namespace primihub::network
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_MEM_CHANNEL_H_

