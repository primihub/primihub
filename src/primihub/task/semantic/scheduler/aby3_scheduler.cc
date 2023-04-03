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

#include "src/primihub/task/semantic/scheduler/aby3_scheduler.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include <google/protobuf/text_format.h>

using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;

namespace primihub::task {
retcode ABY3Scheduler::ScheduleTask(const std::string& role,
    const int32_t rank, const Node dest_node, const PushTaskRequest& request) {
  SET_THREAD_NAME("ABY3Scheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_role(role);
  task_ptr->set_rank(rank);
  // fill scheduler info
  {
    auto party_access_info_ptr = task_ptr->mutable_party_access_info();
    auto& local_node = getLocalNodeCfg();
    rpc::Node scheduler_node;
    node2PbNode(local_node, &scheduler_node);
    auto& schduler_node_list = (*party_access_info_ptr)[SCHEDULER_NODE];
    auto node_item = schduler_node_list.add_node();
    *node_item = std::move(scheduler_node);
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

void ABY3Scheduler::node_push_task(const std::string& node_id,
                    const PeerDatasetMap& peer_dataset_map,
                    const PushTaskRequest& nodePushTaskRequest,
                    const std::map<std::string, std::string>& dataset_owner,
                    const Node& dest_node) {
    SET_THREAD_NAME("ABY3Scheduler");
    PushTaskReply pushTaskReply;
    PushTaskRequest _1NodePushTaskRequest;
    _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);

    // Add params to request
    google::protobuf::Map<std::string, ParamValue> *param_map =
        _1NodePushTaskRequest.mutable_task()->mutable_params()->mutable_param_map();
    auto peer_dataset_map_it = peer_dataset_map.find(node_id);
    if (peer_dataset_map_it == peer_dataset_map.end()) {
        LOG(ERROR) << "node_push_task: peer_dataset_map not found";
        return;
    }

    std::vector<DatasetWithParamTag> dataset_param_list = peer_dataset_map_it->second;

    for (auto &dataset_param : dataset_param_list) {
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        DLOG(INFO) << "📤 push task dataset : " << dataset_param.first << ", " << dataset_param.second;
        pv.set_value_string(dataset_param.first);
        (*param_map)[dataset_param.second] = std::move(pv);
    }

    for (auto &pair : dataset_owner) {
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        pv.set_value_string(pair.second);
        (*param_map)[pair.first] = std::move(pv);
        DLOG(INFO) << "Insert " << pair.first << ":" << pair.second << "into params.";
    }
    {
        // fill scheduler info
        auto party_access_info = _1NodePushTaskRequest.mutable_task()->mutable_party_access_info();
        auto& local_node = getLocalNodeCfg();
        rpc::Node scheduler_node;
        node2PbNode(local_node, &scheduler_node);
        auto& node_list = (*party_access_info)[SCHEDULER_NODE];
        auto node = node_list.add_node();
        *node = std::move(scheduler_node);
    }
    // send request
    auto channel = this->getLinkContext()->getChannel(dest_node);
    auto ret = channel->submitTask(_1NodePushTaskRequest, &pushTaskReply);
    if (ret == retcode::SUCCESS) {
        // parse notify server info from reply
    } else {
        LOG(ERROR) << "Node push task node " << dest_node.to_string() << " rpc failed.";
    }
    parseNotifyServer(pushTaskReply);
}

void ABY3Scheduler::add_vm(rpc::Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(i);
    EndPoint *ed_next = vm->mutable_next();
    EndPoint *ed_prev = vm->mutable_prev();

    auto next = (i + 1) % 3;
    auto prev = (i + 2) % 3;

    auto task_info = pushTaskRequest->task().task_info();
    std::string name_prefix = task_info.job_id() + "_" + task_info.task_id() + "_";

    int session_basePort = 12120;  // TODO move to configfile
    ed_next->set_ip(peer_list_[next].ip());
    // ed_next->set_port(peer_list[std::min(i, next)].data_port());
    ed_next->set_port(std::min(i, next) + session_basePort);
    ed_next->set_name(name_prefix +
                      absl::StrCat(std::min(i, next), std::max(i, next)));
    ed_next->set_link_type(i < next ? LinkType::SERVER : LinkType::CLIENT);

    ed_prev->set_ip(peer_list_[prev].ip());
    // ed_prev->set_port(peer_list[std::min(i, prev)].data_port());
    ed_prev->set_port(std::min(i, prev) + session_basePort);
    ed_prev->set_name(name_prefix +
                      absl::StrCat(std::min(i, prev), std::max(i, prev)));
    ed_prev->set_link_type(i < prev ? LinkType::SERVER : LinkType::CLIENT);
}


/**
 * @brief  Dispatch ABY3  MPC task
 *
 */
void ABY3Scheduler::dispatch(const PushTaskRequest *actorPushTaskRequest) {
  PushTaskRequest send_request;
  send_request.CopyFrom(*actorPushTaskRequest);
  if (actorPushTaskRequest->task().type() == TaskType::ACTOR_TASK) {
    // auto mutable_node_map = nodePushTaskRequest.mutable_task()->mutable_node_map();
    auto party_access_info = send_request.mutable_task()->mutable_party_access_info();
    send_request.mutable_task()->set_type(TaskType::NODE_TASK);
    const auto& party_info = send_request.task().party_access_info();
    std::set<std::string> party_name;
    for (const auto& [role, node_list] : party_info) {
      party_name.insert(role);
    }
    if (party_name.size() != 3) {
      LOG(ERROR) << "ABY3 need 3 party, but get " << party_name.size();
      return;
    }
    int party_id = {0};
    for (const auto& name_ : party_name) {
      auto node = (*party_access_info)[name_].mutable_node(0);
      add_vm(node, party_id, &send_request);
      party_id++;
    }
  }

  std::string str;
  google::protobuf::TextFormat::PrintToString(send_request, &str);
  LOG(INFO) << "ABY3Scheduler::dispatch: " << str;

  LOG(INFO) << "Dispatch SubmitTask to "
      << send_request.task().party_access_info().size() << " node";
  // schedule
  std::vector<std::thread> thrds;
  const auto& party_access_info = send_request.task().party_access_info();
  for (const auto& [role, node_list] : party_access_info) {
    for (const auto& pb_node : node_list.node()) {
      Node dest_node;
      pbNode2Node(pb_node, &dest_node);
      int32_t rank = pb_node.rank();
      LOG(INFO) << "Dispatch SubmitTask to PSI client node to: " << dest_node.to_string() << " "
          << "role: " << role << " rank: " << rank;
      thrds.emplace_back(
        std::thread(
          &ABY3Scheduler::ScheduleTask,
          this,
          role,
          rank,
          dest_node,
          std::ref(send_request)));
    }
  }
  for (auto&& t : thrds) {
    t.join();
  }
}

} // namespace primihub::task
