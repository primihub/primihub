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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SCHEDULER_H_

#include <glog/logging.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/scheduler.h"
#include "src/primihub/common/defines.h"

using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::VMNode;
using primihub::service::DatasetWithParamTag;
using primihub::task::PeerDatasetMap;

namespace primihub::task {

class PSIScheduler : public VMScheduler {
 public:
  PSIScheduler(const std::string &node_id,
                const std::vector<rpc::Node> &peer_list,
                const PeerDatasetMap &peer_dataset_map, bool singleton)
    : VMScheduler(node_id, singleton), peer_list_(peer_list),
      peer_dataset_map_(peer_dataset_map) {}

  void dispatch(const PushTaskRequest *pushTaskRequest) override;
  void add_vm(rpc::Node *single_node, int i, const PushTaskRequest *pushTaskRequest);
 protected:
  void node_push_psi_task(const std::string &node_id,
                    const PeerDatasetMap &peer_dataset_map,
                    const PushTaskRequest &nodePushTaskRequest,
                    const Node& dest_node,
                    bool is_client);
private:
    const std::vector<rpc::Node> peer_list_;
    const PeerDatasetMap peer_dataset_map_;
};

} // namespace primihub::task
#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_SCHEDULER_H_
