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
#include "src/primihub/task/semantic/scheduler/psi_scheduler.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "src/primihub/util/proto_log_helper.h"

using ParamValue = primihub::rpc::ParamValue;
using TaskType = primihub::rpc::TaskType;
using VirtualMachine = primihub::rpc::VirtualMachine;
using VarType = primihub::rpc::VarType;
using PsiTag = primihub::rpc::PsiTag;

namespace pb_util = primihub::proto::util;
namespace primihub::task {
retcode PSIScheduler::ScheduleTask(const std::string& party_name,
                                  const Node dest_node,
                                  const PushTaskRequest& request) {
  VLOG(5) << "begin schedule task to party: " << party_name;
  SET_THREAD_NAME("PSIScheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_party_name(party_name);
  // fill scheduler info
  AddSchedulerNode(task_ptr);
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->submitTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << "submit task to : " << dest_node_address << " reply success";
  } else {
    std::string error_msg = "submit task to : "
      error_msg.append(dest_node_address).append(" reply failed");
    LOG(ERROR) << error_msg;
    this->error_.store(true);
    this->error_msg_[party_name] = error_msg;
    return retcode::FAIL;
  }
  parseNotifyServer(reply);
  return retcode::SUCCESS;
}

void PSIScheduler::add_vm(rpc::Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
  VirtualMachine *vm = node->add_vm();
  vm->set_party_id(i);
}

retcode PSIScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
  PushTaskRequest push_request;
  push_request.CopyFrom(*pushTaskRequest);
  push_request.mutable_task()->set_type(TaskType::NODE_PSI_TASK);
  std::string str = pb_util::TaskRequestToString(push_request);
  LOG(INFO) << "PSIScheduler::dispatch: " << str;
  LOG(INFO) << "Dispatch SubmitTask to PSI client node";
  const auto& participate_node = push_request.task().party_access_info();
  std::vector<std::thread> thrds;
  // allocate space for error msg
  for (const auto& [party_name, node] : participate_node) {
    this->error_msg_.insert({party_name, ""});
  }
  for (const auto& [party_name, node] : participate_node) {
    Node dest_node;
    pbNode2Node(node, &dest_node);
    LOG(INFO) << "Dispatch SubmitTask to PSI client node to: "
        << dest_node.to_string() << " party_name: " << party_name;
    thrds.emplace_back(
      std::thread(
        &PSIScheduler::ScheduleTask,
        this,
        party_name,
        dest_node,
        std::ref(push_request)));
  }
  for (auto&& t : thrds) {
    t.join();
  }
  if (has_error()) {
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::task
