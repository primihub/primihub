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

#include "src/primihub/task/semantic/scheduler/aby3_scheduler.h"
#include <set>
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/common/value_check_util.h"

using EndPoint = primihub::rpc::EndPoint;
using LinkType = primihub::rpc::LinkType;
using ParamValue = primihub::rpc::ParamValue;
using TaskType = primihub::rpc::TaskType;
using VirtualMachine = primihub::rpc::VirtualMachine;
using VarType = primihub::rpc::VarType;

namespace pb_util = primihub::proto::util;
namespace primihub::task {
retcode ABY3Scheduler::ScheduleTask(const std::string& party_name,
    const Node dest_node, const PushTaskRequest& request) {
  SET_THREAD_NAME("ABY3Scheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_party_name(party_name);
  // fill scheduler info
  AddSchedulerNode(task_ptr);
  const auto& task_info = send_request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << TASK_INFO_STR << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->executeTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << TASK_INFO_STR
            << "submit task to : " << dest_node_address << " reply success";
  } else {
    std::string error_msg = "submit task to : ";
    error_msg.append(dest_node_address).append(" reply failed");
    LOG(ERROR) << TASK_INFO_STR << error_msg;
    this->set_error();
    this->error_msg_[party_name] = error_msg;
    return retcode::FAIL;
  }
  parseNotifyServer(reply);
  return retcode::SUCCESS;
}

void ABY3Scheduler::add_vm(int party_id,
                         const PushTaskRequest& task_request,
                         const std::vector<rpc::Node>& party_nodes,
                         rpc::Node *node) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(party_id);
    EndPoint *ed_next = vm->mutable_next();
    EndPoint *ed_prev = vm->mutable_prev();

    auto next = (party_id + 1) % 3;
    auto prev = (party_id + 2) % 3;

    auto& task_info = task_request.task().task_info();
    std::string name_prefix = task_info.job_id();
    name_prefix.append("_").append(task_info.task_id()).append("_");

    int session_basePort = 12120;
    ed_next->set_ip(party_nodes[next].ip());
    // ed_next->set_port(peer_list[std::min(i, next)].data_port());
    ed_next->set_port(std::min(party_id, next) + session_basePort);
    ed_next->set_name(
      name_prefix +
        absl::StrCat(std::min(party_id, next), std::max(party_id, next)));
    ed_next->set_link_type(
      party_id < next ? LinkType::SERVER : LinkType::CLIENT);

    ed_prev->set_ip(party_nodes[prev].ip());
    // ed_prev->set_port(peer_list[std::min(i, prev)].data_port());
    ed_prev->set_port(std::min(party_id, prev) + session_basePort);
    ed_prev->set_name(
      name_prefix +
        absl::StrCat(std::min(party_id, prev), std::max(party_id, prev)));
    ed_prev->set_link_type(
      party_id < prev ? LinkType::SERVER : LinkType::CLIENT);
}


/**
 * @brief  Dispatch ABY3  MPC task
 *
 */
retcode ABY3Scheduler::dispatch(const PushTaskRequest *actorPushTaskRequest) {
  PushTaskRequest send_request;
  send_request.CopyFrom(*actorPushTaskRequest);
  if (actorPushTaskRequest->task().type() == TaskType::ACTOR_TASK) {
    auto party_access_info =
        send_request.mutable_task()->mutable_party_access_info();
    const auto& party_info = send_request.task().party_access_info();
    std::vector<std::string> party_names;
    std::vector<rpc::Node> party_nodes;
    std::set<std::string> dup_names;
    for (const auto& [party_name, node] : party_info) {
      dup_names.insert(party_name);
      party_names.emplace_back(party_name);
      party_nodes.emplace_back(node);
    }
    if (dup_names.size() != 3) {
      std::string error_msg = "ABY3 protocol need 3 party, but get ";
      error_msg.append(std::to_string(dup_names.size()));
      RaiseException(error_msg);
    }
    for (const auto& name_ : party_names) {
      auto& node = (*party_access_info)[name_];
      int32_t party_id = node.party_id();
      add_vm(party_id, send_request, party_nodes, &node);
    }
  }

  if (VLOG_IS_ON(5)) {
    std::string str;
    str = pb_util::TaskRequestToString(send_request);
    VLOG(5) << "ABY3Scheduler::dispatch: " << str;
  }
  size_t party_count = send_request.task().party_access_info().size();
  LOG(INFO) << "Dispatch SubmitTask to " << party_count << " node";

  // schedule
  std::vector<std::thread> thrds;
  std::vector<std::future<retcode>> result_fut;
  const auto& party_access_info = send_request.task().party_access_info();
  for (const auto& [party_name, node] : party_access_info) {
    this->error_msg_.insert({party_name, ""});
  }
  for (const auto& [party_name, node] : party_access_info) {
    Node dest_node;
    pbNode2Node(node, &dest_node);
    LOG(INFO) << "Dispatch Task to party: " << dest_node.to_string() << " "
        << "party_name: " << party_name;
      result_fut.emplace_back(
        std::async(
          std::launch::async,
        &ABY3Scheduler::ScheduleTask,
        this,
        party_name,
        dest_node,
        std::ref(send_request)));
  }
  for (auto&& fut : result_fut) {
    auto ret = fut.get();
  }
  if (has_error()) {
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::task
