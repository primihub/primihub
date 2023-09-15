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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_

#include <string>
#include <vector>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/semantic/task.h"
#include "Poco/Process.h"

using primihub::rpc::PushTaskRequest;

namespace primihub::task {
/* *
 * @brief Federated Learning task, only support python code.
 *  TODO Run python use pybind11.
 */
class FLTask : public TaskBase {
 public:
    FLTask(const std::string &node_id, const TaskParam *task_param,
           const PushTaskRequest &task_request,
           std::shared_ptr<DatasetService> dataset_service);

    ~FLTask() = default;
    int execute() override;
    void kill_task() {
      LOG(WARNING) << "task receives kill task request and stop stauts";
      stop_.store(true);
      task_context_.clean();
    };

 private:
  const PushTaskRequest* task_request_{nullptr};
  std::unique_ptr<Poco::ProcessHandle> process_handler_{nullptr};
};
} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
