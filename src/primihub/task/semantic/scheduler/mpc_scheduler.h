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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_MPC_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_MPC_SCHEDULER_H_
#include <string>
#include <vector>

#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/scheduler.h"

using primihub::rpc::Node;
using primihub::rpc::PushTaskRequest;

namespace primihub::task {
class MPCScheduler : public VMScheduler {
public:
  MPCScheduler(const std::string &node_id, const std::vector<Node> &peer_list,
               const PeerDatasetMap &peer_dataset_map, uint8_t party_num,
               bool singleton)
      : VMScheduler(node_id, singleton) {
    party_num_ = party_num;
    peer_list_ = peer_list;
    peer_dataset_map_ = peer_dataset_map;
  }

  void dispatch(const PushTaskRequest *pus_request) override;

protected:
  std::vector<Node> peer_list_;
  PeerDatasetMap peer_dataset_map_;

private:
  static void push_task(const std::string &node_id,
                        const PeerDatasetMap &peer_dataset_map,
                        const PushTaskRequest &request,
                        const std::string node_addr);

  virtual void add_vm(Node *node, int i,
                      const PushTaskRequest *push_request) = 0;
  uint8_t party_num_;
};

class CRYPTFLOW2Scheduler : public MPCScheduler {
public:
  CRYPTFLOW2Scheduler(const std::string &node_id,
                      const std::vector<Node> &peer_list,
                      const PeerDatasetMap &peer_dataset_map, bool singleton)
      : MPCScheduler(node_id, peer_list, peer_dataset_map, 2, singleton) {}

private:
  void add_vm(Node *node, int i, const PushTaskRequest *push_request) override;
};

class FalconScheduler : public MPCScheduler {
public:
  FalconScheduler(const std::string &node_id,
                  const std::vector<Node> &peer_list,
                  const PeerDatasetMap &peer_dataset_map, bool singleton)
      : MPCScheduler(node_id, peer_list, peer_dataset_map, 3, singleton) {}

private:
  void add_vm(Node *node, int i, const PushTaskRequest *push_request) override;
};
} // namespace primihub::task

#endif
