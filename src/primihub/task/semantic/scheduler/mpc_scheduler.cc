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
#include "src/primihub/task/semantic/scheduler/mpc_scheduler.h"
#include "absl/strings/str_cat.h"
#include "glog/logging.h"

using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VarType;
using primihub::rpc::VirtualMachine;

namespace primihub::task {
void MPCScheduler::push_task(const std::string &node_id,
                             const PeerDatasetMap &peer_dataset_map,
                             const PushTaskRequest &nodePushTaskRequest,
                             const Node& dest_node) {
  PushTaskReply pushTaskReply;
  PushTaskRequest push_request;
  push_request.CopyFrom(nodePushTaskRequest);
  // Add params to request
  // google::protobuf::Map<std::string, ParamValue> *param_map =
  auto param_map = push_request.mutable_task()->mutable_params()->mutable_param_map();
  auto peer_dataset_map_it = peer_dataset_map.find(node_id);
  if (peer_dataset_map_it == peer_dataset_map.end()) {
    LOG(ERROR) << "Error, peer_dataset_map not found.";
    return;
  }

  // std::vector<DatasetWithParamTag>
  auto& dataset_param_list = peer_dataset_map_it->second;
  for (auto& dataset_param : dataset_param_list) {
    ParamValue pv;
    pv.set_var_type(VarType::STRING);
    DLOG(INFO) << "push task dataset : " << dataset_param.first << ", "
               << dataset_param.second;
    pv.set_value_string(dataset_param.first);
    (*param_map)[dataset_param.second] = std::move(pv);
  }
  auto task_config = push_request.mutable_task();
  AddSchedulerNode(task_config);
  std::string dest_node_address = dest_node.to_string();
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->submitTask(push_request, &pushTaskReply);
  if (ret == retcode::SUCCESS) {
      VLOG(5) << "submit task to : " << dest_node_address << " reply success";
  } else {
      LOG(ERROR) << "submit task to : " << dest_node_address << " reply failed";
  }
  parseNotifyServer(pushTaskReply);
}

retcode MPCScheduler::dispatch(const PushTaskRequest *push_request) {
  PushTaskRequest request;
  request.CopyFrom(*push_request);

  if (push_request->task().type() == TaskType::ACTOR_TASK) {
    auto node_map = request.mutable_task()->mutable_node_map();
    request.mutable_task()->set_type(TaskType::NODE_TASK);

    if (peer_list_.size() != party_num_) {
      LOG(ERROR) << "Only support two party in crypTFlow2 protocol.";
      return retcode::FAIL;
    }

    for (uint8_t i = 0; i < party_num_; i++) {
      rpc::Node node;
      node.CopyFrom(peer_list_[i]);
      std::string node_id = node.node_id();
      add_vm(&node, i, &request);
      (*node_map)[node_id] = node;
    }
  }

  std::vector<std::thread> threads;
  std::map<std::string, Node> scheduled_nodes;
  auto &node_map = request.task().node_map();
  for (int i = 0; i < party_num_; i++) {
    std::string node_id = absl::StrCat("node", std::to_string(i));
    auto iter = node_map.find(node_id);
    if (iter == node_map.end()) {
      LOG(ERROR) << "Can't find node " << node_id << " in node map.";
      return retcode::FAIL;
    }
    auto& pb_node = iter->second;
    std::string node_addr = absl::StrCat(pb_node.ip(), ":", pb_node.port());
    Node dest_node;
    pbNode2Node(pb_node, &dest_node);
    scheduled_nodes[node_addr] = std::move(dest_node);
    threads.emplace_back(
      std::thread(
        &CRYPTFLOW2Scheduler::push_task,
        this,
        iter->first,
        this->peer_dataset_map_,
        std::ref(request),
        std::ref(scheduled_nodes[node_addr])));
  }

  for (auto &t : threads) {
    t.join();
  }
  if (has_error()) {
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

void CRYPTFLOW2Scheduler::add_vm(rpc::Node *node, int i,
                                 const PushTaskRequest *push_request) {
  VirtualMachine *vm = node->add_vm();
  vm->set_party_id(i);

  EndPoint *ep_next = vm->mutable_next();
  if (i == 0) {
    ep_next->set_ip(peer_list_[i].ip());
    ep_next->set_port(30020);
    ep_next->set_link_type(LinkType::SERVER);
    ep_next->set_name("CRYPTFLOW2_Server");
  } else {
    ep_next->set_ip(peer_list_[i - 1].ip());
    ep_next->set_port(30020);
    ep_next->set_link_type(LinkType::CLIENT);
    ep_next->set_name("CRYPTFLOW2_Client");
  }

  return;
}

void FalconScheduler::add_vm(rpc::Node *node, int i,
                             const PushTaskRequest *push_request) {
  VirtualMachine *vm0, *vm1;
  vm0 = node->add_vm();
  vm0->set_party_id(i);
  vm1 = node->add_vm();
  vm1->set_party_id(i);

  if (i == 0) {
    EndPoint *ep_next = vm0->mutable_next();
    ep_next->set_ip(peer_list_[i].ip());
    ep_next->set_port(32003);
    ep_next->set_link_type(LinkType::SERVER);
    ep_next->set_name("Falcon_server_0");

    EndPoint *ep_prev = vm0->mutable_prev();
    ep_prev->set_ip(peer_list_[i].ip());
    ep_prev->set_port(32006);
    ep_prev->set_link_type(LinkType::SERVER);
    ep_prev->set_name("Falcon_server_1");

    ep_next = vm1->mutable_next();
    ep_next->set_ip(peer_list_[1].ip());
    ep_next->set_port(32001);
    ep_next->set_link_type(LinkType::CLIENT);
    ep_next->set_name("Falcon_client_0");

    ep_prev = vm1->mutable_prev();
    ep_prev->set_ip(peer_list_[2].ip());
    ep_prev->set_port(32002);
    ep_prev->set_link_type(LinkType::CLIENT);
    ep_prev->set_name("Falcon_client_1");
  } else if (i == 1) {
    EndPoint *ep_next = vm0->mutable_next();
    ep_next->set_ip(peer_list_[i].ip());
    ep_next->set_port(32001);
    ep_next->set_link_type(LinkType::SERVER);
    ep_next->set_name("Falcon_server_2");

    EndPoint *ep_prev = vm0->mutable_prev();
    ep_prev->set_ip(peer_list_[i].ip());
    ep_prev->set_port(32007);
    ep_prev->set_link_type(LinkType::SERVER);
    ep_prev->set_name("Falcon_server_3");

    ep_next = vm1->mutable_next();
    ep_next->set_ip(peer_list_[0].ip());
    ep_next->set_port(32003);
    ep_next->set_link_type(LinkType::CLIENT);
    ep_next->set_name("Falcon_client_2");

    ep_prev = vm1->mutable_prev();
    ep_prev->set_ip(peer_list_[2].ip());
    ep_prev->set_port(32005);
    ep_prev->set_link_type(LinkType::CLIENT);
    ep_prev->set_name("Falcon_client_3");
  } else {
    EndPoint *ep_next = vm0->mutable_next();
    ep_next->set_ip(peer_list_[i].ip());
    ep_next->set_port(32002);
    ep_next->set_link_type(LinkType::SERVER);
    ep_next->set_name("Falcon_server_4");

    EndPoint *ep_prev = vm0->mutable_prev();
    ep_prev->set_ip(peer_list_[i].ip());
    ep_prev->set_port(32005);
    ep_prev->set_link_type(LinkType::SERVER);
    ep_prev->set_name("Falcon_server_5");

    ep_next = vm1->mutable_next();
    ep_next->set_ip(peer_list_[0].ip());
    ep_next->set_port(32006);
    ep_next->set_link_type(LinkType::CLIENT);
    ep_next->set_name("Falcon_client_4");

    ep_prev = vm1->mutable_prev();
    ep_prev->set_ip(peer_list_[1].ip());
    ep_prev->set_port(32007);
    ep_prev->set_link_type(LinkType::CLIENT);
    ep_prev->set_name("Falcon_client_4");
  }

  return;
}

} // namespace primihub::task
