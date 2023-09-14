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
#include <atomic>

#include "src/primihub/common/common.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/semantic/mpc_task.h"


namespace primihub::task {
class MPCExecutor {
 public:
  MPCExecutor(const std::string& task_req, const std::string& protocol);
  ~MPCExecutor();
  retcode Max(const std::vector<double>& input, std::vector<double>* result);
  retcode Min(const std::vector<double>& input, std::vector<double>* result);
  retcode Avg(const std::vector<double>& input, std::vector<double>* result);
  retcode Sum(const std::vector<double>& input, std::vector<double>* result);

 protected:
  /**
   * party id 0 generate sub task uid
  */
  retcode GenerateSubtaskId(std::string* sub_task_id);
  /**
   * party id 0 broadcast sub task uid to the party who participate
  */
  retcode BroadcastSubtaskId(const std::string& sub_task_id);
  /**
   * the parties except party id 0, will receive sub task id from party id 0
  */
  retcode RecvSubTaskId(std::string* sub_task_id);
  /**
   * party id 0, invite auxiliary server to join the sub task
  */
  retcode InviteAuxiliaryServerToTask();
  /**
   * all party negotiate sub task id, auxiliary compute party does not,
   * cause the task id launched by party who`s party id is 0
  */
  retcode NegotiateSubTaskId(std::string* sub_task_id);
  /**
   * add auxiliary compute server to task config
  */
  retcode AddAuxiliaryComputeServer(rpc::Task* task);
  retcode StopTask();
  retcode SetStatisticsOperation(rpc::Algorithm::StatisticsOpType op_type,
                                 rpc::Task* task);
  retcode SetArithmeticOperation(rpc::Algorithm::ArithmeticOpType op_type,
                                 rpc::Task* task);
  retcode BroadcastShape(const std::vector<int64_t>& shape);

 private:
  std::unique_ptr<rpc::PushTaskRequest> task_req_ptr_{nullptr};
  std::unique_ptr<MPCTask> task_ptr_{nullptr};
};
}  // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_MPC_TASK_WRAPPER_H_
