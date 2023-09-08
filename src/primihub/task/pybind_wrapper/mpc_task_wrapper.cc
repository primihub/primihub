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
#include "src/primihub/task/pybind_wrapper/mpc_task_wrapper.h"
#include <glog/logging.h>

#include "src/primihub/common/common.h"

namespace primihub::task {
MPCExecutor::MPCExecutor(const std::string& task_req_str,
                         const std::string& protocol) {
//
  task_req_ptr_ = std::make_unique<primihub::rpc::PushTaskRequest>();
  bool succ_flag = task_req_ptr_->ParseFromString(task_req_str);
  if (!succ_flag) {
    throw std::runtime_error("parser task request encountes error");
  }
  auto& task_config = task_req_ptr_->task();
  // task_ptr_ = std::make_unique<PsiTask>(&task_config);
}

retcode MPCExecutor::Max(const std::vector<double>& input,
                         std::vector<double>* result) {
  return retcode::SUCCESS;
}

retcode MPCExecutor::Min(const std::vector<double>& input,
                         std::vector<double>* result) {
  return retcode::SUCCESS;
}

retcode MPCExecutor::Avg(const std::vector<double>& input,
                         std::vector<double>* result) {
  return retcode::SUCCESS;
}

retcode MPCExecutor::Sum(const std::vector<double>& input,
                         std::vector<double>* result) {
  return retcode::SUCCESS;
}

}  // namespace primihub::task
