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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SCHEDULER_H_

#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <grpc/grpc.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#ifdef USE_MICROSOFT_APSI
#include <apsi/oprf/oprf_sender.h>
#include <apsi/thread_pool_mgr.h>
#include <apsi/version.h>
#include <apsi/zmq/sender_dispatcher.h>
#endif

#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/scheduler.h"

using grpc::Channel;
// using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using primihub::rpc::Node;
using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::VMNode;
using primihub::service::DatasetWithParamTag;
using primihub::task::PeerDatasetMap;

#ifdef USE_MICROSOFT_APSI
using namespace apsi;
using namespace apsi::sender;
using namespace apsi::network;
using namespace apsi::oprf;
#endif

namespace primihub::task {

class PIRScheduler : public VMScheduler {
public:
    PIRScheduler(const std::string &node_id,
                 const std::vector<Node> &peer_list,
                 const PeerDatasetMap &peer_dataset_map, bool singleton)
        : VMScheduler(node_id, singleton),
          peer_list_(peer_list),
          peer_dataset_map_(peer_dataset_map) {}

    void dispatch(const PushTaskRequest *pushTaskRequest) override;

    void add_vm(Node *single_node, int i,
                const PushTaskRequest *pushTaskRequest);

    int transformRequest(PushTaskRequest &taskRequest);

private:
    const std::vector<Node> peer_list_;
    const PeerDatasetMap peer_dataset_map_;
};
}

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PIR_SCHEDULER_H_
