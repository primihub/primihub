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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_ABY3_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_ABY3_SCHEDULER_H_
#include <glog/logging.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include "src/primihub/common/common.h"

using PushTaskReply = primihub::rpc::PushTaskReply;
using PushTaskRequest = primihub::rpc::PushTaskRequest;
using VMNode = primihub::rpc::VMNode;
using DatasetWithParamTag = primihub::service::DatasetWithParamTag;
using PeerDatasetMap = primihub::task::PeerDatasetMap;

namespace primihub::task {
class ABY3Scheduler : public VMScheduler {
 public:
  ABY3Scheduler() = default;
  ABY3Scheduler(const std::string &node_id,
                const std::vector<rpc::Node> &peer_list,
                const PeerDatasetMap &peer_dataset_map, bool singleton)
      : VMScheduler(node_id, singleton),
        peer_list_(peer_list),
        peer_dataset_map_(peer_dataset_map) {}

  retcode dispatch(const PushTaskRequest *pushTaskRequest) override;
  void add_vm(int party_id,
              const PushTaskRequest& pushTaskRequest,
              const std::vector<rpc::Node>& party_nodes,
              rpc::Node *single_node);

 protected:
  retcode ScheduleTask(const std::string& party_name,
                      const Node dest_node,
                      const PushTaskRequest& request);

 private:
  const std::vector<rpc::Node> peer_list_;
  const PeerDatasetMap peer_dataset_map_;
  std::map<std::string, std::string> dataset_owner_;
};

}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_ABY3_SCHEDULER_H_
