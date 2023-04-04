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
#include "src/primihub/task/semantic/scheduler/psi_scheduler.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include <google/protobuf/text_format.h>

using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;
using primihub::rpc::PsiTag;

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
  {
    auto party_access_info_ptr = task_ptr->mutable_party_access_info();
    auto& local_node = getLocalNodeCfg();
    rpc::Node scheduler_node;
    node2PbNode(local_node, &scheduler_node);
    auto& schduler_node = (*party_access_info_ptr)[SCHEDULER_NODE];
    schduler_node = std::move(scheduler_node);
  }
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->submitTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << "submit task to : " << dest_node_address << " reply success";
  } else {
    LOG(ERROR) << "submit task to : " << dest_node_address << " reply failed";
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

void PSIScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
  PushTaskRequest push_request;
  push_request.CopyFrom(*pushTaskRequest);
  push_request.mutable_task()->set_type(TaskType::NODE_PSI_TASK);
  std::string str;
  google::protobuf::TextFormat::PrintToString(push_request, &str);
  LOG(INFO) << "PSIScheduler::dispatch: " << str;
  LOG(INFO) << "Dispatch SubmitTask to PSI client node";
  const auto& participate_node = push_request.task().party_access_info();
  std::vector<std::thread> thrds;
  for (const auto& [party_name, node] : participate_node) {
    Node dest_node;
    pbNode2Node(node, &dest_node);
    LOG(INFO) << "Dispatch SubmitTask to PSI client node to: " << dest_node.to_string() << " "
        << "party_name: " << party_name;
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
}

}
