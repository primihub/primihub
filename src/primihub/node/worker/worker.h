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


#ifndef SRC_PRIMIHUB_NODE_WORKER_WORKER_H_
#define SRC_PRIMIHUB_NODE_WORKER_WORKER_H_

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <grpc/grpc.h>
#include <google/protobuf/text_format.h>
#include <cmath>

#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <numeric>
#include <thread>
#include <vector>
#include <list>

#include "Eigen/Dense"

#include "src/primihub/node/nodelet.h"
#include "src/primihub/common/clp.h"
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/algorithm/regression.h"
#include "src/primihub/algorithm/linear_model_gen.h"
#include "src/primihub/algorithm/aby3ML.h"
#include "src/primihub/algorithm/plainML.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/semantic/private_server_base.h"
#include "src/primihub/common/defines.h"

using namespace std;
using namespace Eigen;

using primihub::rpc::PushTaskRequest;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;

namespace primihub {
class Worker {
 public:
    explicit Worker(const std::string& node_id_,
                     std::shared_ptr<Nodelet> nodelet_)
        : node_id(node_id_), nodelet(nodelet_) {}
    Worker(const std::string& node_id_, const std::string& worker_id,
            std::shared_ptr<Nodelet> nodelet_)
        : node_id(node_id_), worker_id_(worker_id), nodelet(nodelet_) {}
    int execute(const PushTaskRequest* pushTaskRequest);

    void execute(const ExecuteTaskRequest *taskRequest,
                 ExecuteTaskResponse *taskResponse);
    inline std::string worker_id() {return worker_id_;}
    void kill_task();

 private:
    std::unordered_map<std::string, std::shared_ptr<Worker>> workers_
        GUARDED_BY(worker_map_mutex_);

    mutable absl::Mutex worker_map_mutex_;
    std::shared_ptr<primihub::task::TaskBase> task_ptr{nullptr};
    std::shared_ptr<primihub::task::ServerTaskBase> task_server_ptr{nullptr};
    const std::string& node_id;
    std::shared_ptr<Nodelet> nodelet;
    std::string worker_id_;
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_NODE_WORKER_WORKER_H_
