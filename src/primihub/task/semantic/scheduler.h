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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_H_

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/link_factory.h"

using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::VMNode;
using primihub::service::DatasetWithParamTag;

namespace primihub::task {
using PeerDatasetMap = std::map<std::string, std::vector<DatasetWithParamTag>>;

class VMScheduler {
 public:
  VMScheduler(const std::string &node_id, bool singleton)
      : node_id_(node_id), singleton_(singleton) {
    link_ctx_ = primihub::network::LinkFactory::createLinkContext(primihub::network::LinkMode::GRPC);
  }

  std::unique_ptr<primihub::network::LinkContext>& getLinkContext() {
    return link_ctx_;
  }

  virtual void dispatch(const PushTaskRequest *pushTaskRequest) = 0;
  virtual void set_dataset_owner(std::map<std::string, std::string> &dataset_owner) {}
  std::string get_node_id() const {
    return node_id_;
  }
  void parseNotifyServer(const PushTaskReply& reply) {
    for (const auto& node : reply.notify_server()) {
        Node notify_server_info(node.ip(), node.port(), node.use_tls(), node.role());
        VLOG(5) << "notify_server_info: " << notify_server_info.to_string();
        this->addNotifyServer(std::move(notify_server_info));
    }
  }
  void addNotifyServer(Node&& node_info) {
    std::lock_guard<std::mutex> lck(notify_server_mtx);
    notify_server_info.push_back(std::move(node_info));
  }
  void addNotifyServer(const Node& node_info) {
    std::lock_guard<std::mutex> lck(notify_server_mtx);
    notify_server_info.push_back(node_info);
  }
  std::vector<Node>& notifyServer() {
    return notify_server_info;
  }
 protected:
  const std::string node_id_;
  bool singleton_;
  std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};
  std::mutex notify_server_mtx;
  std::vector<Node> notify_server_info;
};
} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_H_
