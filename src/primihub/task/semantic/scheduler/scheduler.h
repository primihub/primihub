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
#include <future>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/config/server_config.h"

using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::VMNode;
using primihub::service::DatasetWithParamTag;

namespace primihub::task {
using PeerDatasetMap = std::map<std::string, std::vector<DatasetWithParamTag>>;

class VMScheduler {
 public:
  VMScheduler();
  VMScheduler(const std::string &node_id, bool singleton);
  virtual ~VMScheduler() = default;
  virtual retcode dispatch(const PushTaskRequest *pushTaskRequest);
  virtual void set_dataset_owner(
      std::map<std::string, std::string> &dataset_owner) {}

  inline std::string get_node_id() const {return node_id_;}
  void parseNotifyServer(const PushTaskReply& reply);

  void addTaskServer(Node&& node_info);
  void addTaskServer(const Node& node_info);

  auto getLinkContext() -> std::unique_ptr<primihub::network::LinkContext>& {
    return link_ctx_;
  }
  std::vector<Node>& taskServer() {
    return task_server_info;
  }

 protected:
  retcode AddSchedulerNode(rpc::Task* task);
  void initCertificate();
  Node& getLocalNodeCfg() const;
  void InitLinkContext();
  retcode ScheduleTask(const std::string& party_name,
                    const Node dest_node,
                    const PushTaskRequest& request);
  void set_error() {error_.store(true);}
  bool has_error() {return error_.load(std::memory_order::memory_order_relaxed);}
 protected:
  const std::string node_id_;
  bool singleton_;
  std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};
  std::mutex task_server_mtx;
  std::vector<Node> task_server_info;
  std::atomic<bool> error_{false};        //
  std::map<std::string, std::string> error_msg_;
};
} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_H_
