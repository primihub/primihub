/*
 Copyright 2022 PrimiHub

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
#include "src/primihub/common/config/server_config.h"

namespace primihub::task {
// temp data storage
/**
 * TaskContext
 * contains temporary storage, communication link info
*/
class TaskContext {
 public:
  TaskContext() {
    auto link_mode = primihub::network::LinkMode::GRPC;
    link_ctx_ = primihub::network::LinkFactory::createLinkContext(link_mode);
    auto& server_config = primihub::ServerConfig::getInstance();
    if (!server_config.IsInitFlag()) {
      LOG(WARNING) << "instance is not init";
    }

    auto& host_cfg = server_config.getServiceConfig();
    if (host_cfg.use_tls()) {
      LOG(INFO) << "link_ctx_->initCertificate";
      link_ctx_->initCertificate(server_config.getCertificateConfig());
    }
  }

  explicit TaskContext(primihub::network::LinkMode mode) {
    link_ctx_ = primihub::network::LinkFactory::createLinkContext(mode);
    auto& server_config = primihub::ServerConfig::getInstance();
    auto& host_cfg = server_config.getServiceConfig();
    if (host_cfg.use_tls()) {
      LOG(ERROR) << "link_ctx_->initCertificate";
      link_ctx_->initCertificate(server_config.getCertificateConfig());
    }
  }

  void setTaskInfo(const std::string& job_id,
                  const std::string& task_id,
                  const std::string& request_id,
                  const std::string& sub_task_id) {
    link_ctx_->setTaskInfo(job_id, task_id, request_id, sub_task_id);
  }

  std::unique_ptr<primihub::network::LinkContext>& getLinkContext() {
    return link_ctx_;
  }

  void clean() {
    stop_.store(true);
    if (link_ctx_) {
      link_ctx_->Clean();
    }
  }

 private:
  std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};
  std::atomic<bool> stop_{false};
};

}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TASK_CONTEXT_H_
