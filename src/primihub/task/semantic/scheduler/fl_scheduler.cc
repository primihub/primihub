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

#include "src/primihub/task/semantic/scheduler/fl_scheduler.h"
#include <memory>
#include "absl/strings/str_cat.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/service/dataset/util.hpp"
#include <google/protobuf/text_format.h>

using primihub::rpc::Task;
using primihub::rpc::Params;
using primihub::rpc::ParamValue;
using primihub::rpc::VarType;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;

using primihub::service::DataURLToDetail;

namespace primihub::task {


void nodeContext2TaskParam(const NodeContext& node_context,
                            const std::vector<std::shared_ptr<DatasetMeta>> &dataset_meta_list,
                            PushTaskRequest* node_task_request) {
    auto task_ptr = node_task_request->mutable_task();
    // google::protobuf::Map<std::string, ParamValue> *params_map =
    auto params_map = task_ptr->mutable_params()->mutable_param_map();
    // Role
    ParamValue pv_role;
    pv_role.set_var_type(VarType::STRING);
    pv_role.set_value_string(node_context.role);
    (*params_map)["role"] = std::move(pv_role);

    // Protocol
    ParamValue pv_protocol;
    pv_protocol.set_var_type(VarType::STRING);
    pv_protocol.set_value_string(node_context.protocol);
    (*params_map)["protocol"] = std::move(pv_protocol);

    // Dataset meta
    std::map<std::string, std::string> node_dataset_map;
    for (auto &dataset_meta : dataset_meta_list) {
        ParamValue pv_dataset;
        pv_dataset.set_var_type(VarType::STRING);
        // Get data path from data URL
        std::string dataset_path;
        Node node_info;
        std::string data_url = dataset_meta->getDataURL();
        DataURLToDetail(data_url, node_info, dataset_path);
        // Only set dataset path
        pv_dataset.set_value_string(dataset_path);
        (*params_map)[dataset_meta->getDescription()] = std::move(pv_dataset);

        node_dataset_map[node_info.id()] = dataset_meta->getDescription();
    }

    // Save every node's dataset name.
    for (auto pair : node_dataset_map) {
        std::string key = pair.first + "_dataset";
        ParamValue pv_name;
        pv_name.set_var_type(VarType::STRING);
        pv_name.set_value_string(pair.second);
        (*params_map)[key] = std::move(pv_name);
    }

    // Dataset key list
    for (size_t i = 0; i < node_context.datasets.size(); i++) {
        task_ptr->add_input_datasets(node_context.datasets[i]);
    }
    // dumps code
    // task_ptr->set_code(node_context.dumps_func);
}

retcode FLScheduler::ScheduleTask(const std::string& party_name,
                                  const Node dest_node,
                                  const PushTaskRequest& request) {
  SET_THREAD_NAME("FLScheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_party_name(party_name);
  // fill scheduler info
  {
    auto party_access_info_ptr = task_ptr->mutable_party_access_info();
    auto& local_node = getLocalNodeCfg();
    rpc::Node node;
    node2PbNode(local_node, &node);
    auto& schduler_node = (*party_access_info_ptr)[SCHEDULER_NODE];
    schduler_node = std::move(node);
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

void FLScheduler::push_node_py_task(const std::string& node_id,
                        const std::string& role,
                        const Node& dest_node,
                        const PushTaskRequest& nodePushTaskRequest,
                        const PeerContextMap& peer_context_map,
                        const std::vector<std::shared_ptr<DatasetMeta>>& dataset_meta_list) {
    SET_THREAD_NAME("FLScheduler");
    PushTaskReply pushTaskReply;
    PushTaskRequest _1NodePushTaskRequest;
    _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);
    {
        // fill scheduler info
        auto node_map = _1NodePushTaskRequest.mutable_task()->mutable_node_map();
        auto& local_node = getLocalNodeCfg();
        rpc::Node scheduler_node;
        node2PbNode(local_node, &scheduler_node);
        (*node_map)[SCHEDULER_NODE] = std::move(scheduler_node);
    }
    const NodeContext& peer_context = peer_context_map.find(role)->second;
    nodeContext2TaskParam(peer_context, dataset_meta_list, &_1NodePushTaskRequest);
    auto channel = this->getLinkContext()->getChannel(dest_node);
    auto ret = channel->submitTask(_1NodePushTaskRequest, &pushTaskReply);
    if (ret == retcode::SUCCESS) {
        //
    } else {
        LOG(ERROR) << "Node push task node " << dest_node.to_string() << " rpc failed.";
    }
    parseNotifyServer(pushTaskReply);
}

/**
 * @brief Dispatch FL task to different role. eg: xgboost host & guest.
 *
 */
void FLScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
  PushTaskRequest send_request;
  send_request.CopyFrom(*pushTaskRequest);
  std::string str;
  send_request.mutable_task()->set_type(TaskType::NODE_TASK);
  google::protobuf::TextFormat::PrintToString(send_request, &str);
  LOG(INFO) << "FLScheduler::dispatch: " << str;

  // schedule
  std::vector<std::thread> thrds;
  const auto& party_access_info = send_request.task().party_access_info();
  for (const auto& [party_name, node] : party_access_info) {
    Node dest_node;
    pbNode2Node(node, &dest_node);
    LOG(INFO) << "Dispatch SubmitTask to: " << dest_node.to_string() << " "
        << "party_name: " << party_name;
    thrds.emplace_back(
      std::thread(
        &FLScheduler::ScheduleTask,
        this,
        party_name,
        dest_node,
        std::ref(send_request)));
  }
  for (auto&& t : thrds) {
    t.join();
  }
    // // Construct node map
    // if (pushTaskRequest->task().type() == TaskType::ACTOR_TASK) {
    //     auto task_ptr = nodePushTaskRequest.mutable_task();
    //     auto mutable_node_map = task_ptr->mutable_node_map();
    //     task_ptr->set_type(TaskType::NODE_TASK);

    //     // role: host -> party 0   role: guest -> party 1
    //     for (size_t i = 0; i < this->peers_with_tag_.size(); i++) {
    //         rpc::Node single_node;

    //         single_node.CopyFrom(this->peers_with_tag_[i].first);
    //         std::string node_id = this->peers_with_tag_[i].first.node_id();
    //         std::string role = this->peers_with_tag_[i].second;

    //         int party_id = 0;
    //         if (role == "host") {
    //             party_id = 0;
    //         } else if (role == "guest") {
    //             party_id = 1;
    //         }
    //         (*mutable_node_map)[node_id] = single_node;
    //         add_vm(&single_node, party_id, 2, &nodePushTaskRequest);
    //         //Update single_node with vm info
    //         (*mutable_node_map)[node_id] = single_node;
    //     }
    // }

    // // schedule
    // std::vector<std::thread> thrds;
    // std::map<std::string, Node> scheduled_nodes;
    // for (size_t i = 0; i < peers_with_tag_.size(); i++) {
    //     NodeWithRoleTag peer_with_tag = peers_with_tag_[i];
    //     auto& pb_node = peer_with_tag.first;
    //     std::string dest_node_address{absl::StrCat(pb_node.ip(), ":", pb_node.port())};
    //     LOG(INFO) << "dest_node_address: " << dest_node_address;

    //     Node dest_node;
    //     pbNode2Node(pb_node, &dest_node);
    //     scheduled_nodes[dest_node_address] = std::move(dest_node);
    //     // TODO 获取当Role的data meta list
    //     std::vector<std::shared_ptr<DatasetMeta>> data_meta_list;
    //     getDataMetaListByRole(peer_with_tag.second, &data_meta_list);
    //     thrds.emplace_back(
    //         std::thread(
    //             &FLScheduler::push_node_py_task,
    //             this,
    //             peer_with_tag.first.node_id(),    // node_id
    //             peer_with_tag.second,             // role
    //             std::ref(scheduled_nodes[dest_node_address]),  // dest_node
    //             std::ref(nodePushTaskRequest), // nodePushTaskRequest
    //             this->peer_context_map_,
    //             data_meta_list));
    // }

    // for (auto &t : thrds) {
    //     t.join();
    // }
}

void FLScheduler::getDataMetaListByRole(const std::string &role,
                                        std::vector<std::shared_ptr<DatasetMeta>> *data_meta_list) {
    for (size_t i = 0; i < this->metas_with_role_tag_.size(); i++) {
        if (this->metas_with_role_tag_[i].second == role) {
            data_meta_list->push_back(this->metas_with_role_tag_[i].first);
        }
    }
}

void FLScheduler::add_vm(rpc::Node *node, int i, int role_num,
                        const PushTaskRequest *pushTaskRequest) {
    int ret = 0;
    for (auto node_with_tag : peers_with_tag_) {
        VirtualMachine *vm = node->add_vm();
        EndPoint *ep_next = vm->mutable_next();
        ep_next->set_ip(node_with_tag.first.ip());
        ep_next->set_link_type(LinkType::SERVER);
        ep_next->set_port(node_with_tag.first.port());

        std::string ep_name =
            node_with_tag.first.node_id() + "_" + node_with_tag.second;
        ep_next->set_name(ep_name);
    }
}
} // namespace primihub::task
