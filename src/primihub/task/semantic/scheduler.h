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

#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/service/dataset/service.h"

using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::VMNode;
using primihub::service::DatasetWithParamTag;

namespace primihub::task {
using PeerDatasetMap = std::map<std::string, std::vector<DatasetWithParamTag>>;

class VMScheduler {
  public:
    VMScheduler(const std::string &node_id,
                 bool singleton)
        : node_id_(node_id),
          singleton_(singleton) {}

    virtual void dispatch(const PushTaskRequest *pushTaskRequest) = 0;
    virtual void set_dataset_owner(std::map<std::string, std::string> &dataset_owner) {}
    std::string get_node_id() const {
      return node_id_;
    }

  protected:
    const std::string node_id_;
    bool singleton_;
};
} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_H_
