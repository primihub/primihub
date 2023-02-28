/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_TASK_CONTEXT_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_TASK_CONTEXT_H_
#include <unordered_map>
#include <queue>
#include <mutex>
#include <string>
#include <memory>

#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/node/server_config.h"

namespace primihub::task {
// temp data storage
/**
 * TaskContext
 * contains temperary storage, communication link info
*/
template<typename T = std::string, typename U = std::string,
        typename std::enable_if<!std::is_pointer<T>::value, T>::type* = nullptr,
        typename std::enable_if<!std::is_pointer<U>::value, U>::type* = nullptr>
class TaskContext {
 public:
  TaskContext() {
    link_ctx_ = primihub::network::LinkFactory::createLinkContext(primihub::network::LinkMode::GRPC);
    initCertificate();
  }

  explicit TaskContext(primihub::network::LinkMode mode) {
    link_ctx_ = primihub::network::LinkFactory::createLinkContext(mode);
    initCertificate();
  }

  void setTaskInfo(const std::string& job_id, const std::string& task_id) {
    link_ctx_->setTaskInfo(job_id, task_id);
  }

  primihub::ThreadSafeQueue<T>& getRecvQueue(const std::string& role = "default") {
    std::unique_lock<std::mutex> lck(this->in_queue_mtx);
    auto it = in_data_queue.find(role);
    if (it != in_data_queue.end()) {
      return it->second;
    } else {
      in_data_queue[role];
      if (stop_.load(std::memory_order::memory_order_relaxed)) {
          in_data_queue[role].shutdown();
      }
      return in_data_queue[role];
    }
  }

  primihub::ThreadSafeQueue<U>& getSendQueue(const std::string& role = "default") {
    std::unique_lock<std::mutex> lck(this->out_queue_mtx);
    auto it = out_data_queue.find(role);
    if (it != out_data_queue.end()) {
      return it->second;
    } else {
      return out_data_queue[role];
    }
  }
  primihub::ThreadSafeQueue<retcode>& getCompleteQueue(const std::string& role = "default") {
    std::unique_lock<std::mutex> lck(this->complete_queue_mtx);
    auto it = complete_queue.find(role);
    if (it != complete_queue.end()) {
      return it->second;
    } else {
      return complete_queue[role];
    }
  }
  std::unique_ptr<primihub::network::LinkContext>& getLinkContext() {
    return link_ctx_;
  }

  void clean() {
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
    void initCertificate() {
        auto& server_config = primihub::ServerConfig::getInstance();
        auto& host_cfg = server_config.getServiceConfig();
        if (host_cfg.use_tls()) {
            link_ctx_->initCertificate(server_config.getCertificateConfig());
        }
    }

 private:
  std::mutex in_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<T>> in_data_queue;
  std::mutex out_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<U>> out_data_queue;
  std::mutex complete_queue_mtx;
  std::unordered_map<std::string, primihub::ThreadSafeQueue<retcode>> complete_queue;
  std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};
  std::atomic<bool> stop_{false};
};
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TASK_CONTEXT_H_
