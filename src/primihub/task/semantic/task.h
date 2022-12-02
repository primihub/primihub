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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#include <glog/logging.h>
#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/task_context.h"

using primihub::rpc::Task;
using primihub::service::DatasetService;

namespace primihub::task {
using TaskParam = primihub::rpc::Task;

/**
 * @brief Basic task class
 *
 */

class TaskBase {
 public:
   using task_context_t = TaskContext<primihub::rpc::TaskRequest, primihub::rpc::TaskResponse>;
   TaskBase(const TaskParam *task_param,
            std::shared_ptr<DatasetService> dataset_service);
   virtual ~TaskBase() = default;
   virtual int execute() = 0;
   virtual int kill_task() {
      LOG(INFO) << "UNIMPLEMENT";
   };
  inline task_context_t& getTaskContext() {
    return task_context_;
  }
  inline task_context_t* getMutableTaskContext() {
    return &task_context_;
  }
  void setTaskParam(const TaskParam *task_param);
  TaskParam* getTaskParam();

 protected:
   std::atomic<bool> stop_{false};
   TaskParam task_param_;
   std::shared_ptr<DatasetService> dataset_service_;
   task_context_t task_context_;
};

} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
