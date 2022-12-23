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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/task/semantic/task.h"
#include <pybind11/embed.h>
#include <string>
#include <vector>

using primihub::rpc::PushTaskRequest;

namespace py = pybind11;
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

    ~FLTask();

    int execute() override;

  private:
    std::string jobid_;
    std::string taskid_;

    PushTaskRequest task_request_;
    std::string py_code_;
    NodeContext node_context_;
    py::object ph_exec_m_, ph_context_m_;
    std::map<std::string, std::string> dataset_meta_map_;

    // Key is the combine of node's nodeid and role,
    // and value is 'ip:port'.
    std::map<std::string, std::string> node_addr_map_;

    // Save all params.
    std::map<std::string, std::string> params_map_;
};

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
