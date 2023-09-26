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
#include "src/primihub/task/pybind_wrapper/psi_wrapper.h"
#include <glog/logging.h>

#include "src/primihub/common/common.h"

namespace primihub::task {
PsiExecutor::PsiExecutor(const std::string& task_req_str,
                       const std::string& protocol) {
//
  task_req_ptr_ = std::make_unique<primihub::rpc::PushTaskRequest>();
  bool succ_flag = task_req_ptr_->ParseFromString(task_req_str);
  if (!succ_flag) {
    throw std::runtime_error("parser task request encountes error");
  }
  auto& task_config = task_req_ptr_->task();
  task_ptr_ = std::make_unique<PsiTask>(&task_config);
}

auto PsiExecutor::RunPsi(const std::vector<std::string>& input) ->
    std::vector<std::string> {
  std::vector<std::string> result;
  auto ret = task_ptr_->ExecuteTask(input, &result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "run psi task failed";
    throw std::runtime_error("run psi task failed");
  }
  return result;
}
}  // namespace primihub::task
