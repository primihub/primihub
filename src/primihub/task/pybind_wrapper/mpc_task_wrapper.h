/*
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_MPC_TASK_WRAPPER_H_
#define SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_MPC_TASK_WRAPPER_H_

#include <string>
#include <vector>
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"

namespace primihub::task {
class MPCExecutor {
 public:
  MPCExecutor(const std::string& task_req, const std::string& protocol);
  retcode Max(const std::vector<double>& input, std::vector<double>* result);
  retcode Min(const std::vector<double>& input, std::vector<double>* result);
  retcode Avg(const std::vector<double>& input, std::vector<double>* result);
  retcode Sum(const std::vector<double>& input, std::vector<double>* result);

 private:
  std::unique_ptr<primihub::rpc::PushTaskRequest> task_req_ptr_{nullptr};
};
}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_MPC_TASK_WRAPPER_H_
