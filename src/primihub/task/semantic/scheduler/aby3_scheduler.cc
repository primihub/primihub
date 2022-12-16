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
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"

using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;

namespace primihub::task {

void ABY3Scheduler::node_push_task(const std::string& node_id,
                    const PeerDatasetMap& peer_dataset_map,
                    const PushTaskRequest& nodePushTaskRequest,
                    const std::map<std::string, std::string>& dataset_owner,
                    const Node& dest_node) {
    grpc::ClientContext context;
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

    // send request
    auto channel = this->getLinkContext()->getChannel(dest_node);
    auto ret = channel->submitTask(_1NodePushTaskRequest, &pushTaskReply);
    if (ret == retcode::SUCCESS) {
        // parse notify server info from reply
    } else {
        LOG(ERROR) << "Node push task node " << dest_node.to_string() << " rpc failed.";
    }
}

void ABY3Scheduler::add_vm(rpc::Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(i);
    EndPoint *ed_next = vm->mutable_next();
    EndPoint *ed_prev = vm->mutable_prev();

    auto next = (i + 1) % 3;
    auto prev = (i + 2) % 3;

    std::string name_prefix = pushTaskRequest->task().job_id() + "_" +
                              pushTaskRequest->task().task_id() + "_";

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
    PushTaskRequest nodePushTaskRequest;
    nodePushTaskRequest.CopyFrom(*actorPushTaskRequest);


    if (actorPushTaskRequest->task().type() == TaskType::ACTOR_TASK) {
        auto mutable_node_map = nodePushTaskRequest.mutable_task()->mutable_node_map();
        nodePushTaskRequest.mutable_task()->set_type(TaskType::NODE_TASK);

        for (size_t i = 0; i < peer_list_.size(); i++) {
            rpc::Node single_node;
            single_node.CopyFrom(peer_list_[i]);
            std::string node_id = peer_list_[i].node_id();
            if (singleton_) {
                for (size_t j = 0; j < peer_list_.size(); j++) {
                    add_vm(&single_node, j, &nodePushTaskRequest);
                }
                (*mutable_node_map)[node_id] = single_node;
                break;
            } else {
                add_vm(&single_node, i, &nodePushTaskRequest);
            }
            (*mutable_node_map)[node_id] = single_node;
        }
    }


    LOG(INFO) << " 📧  Dispatch SubmitTask to "
        << nodePushTaskRequest.mutable_task()->mutable_node_map()->size() << " node";
    // schedule
    std::vector<std::thread> thrds;
    const auto& node_map = nodePushTaskRequest.task().node_map();
    //  3 nodes request paramaeter are differents.
    std::map<std::string, Node> scheduled_nodes;
    for (int i = 0; i < 3; i++) {
        for (auto &pair : node_map) {
            std::string party_name = "node" + std::to_string(i);
            if (party_name != pair.first) {
                continue;
            }
            std::string dest_node_address(
                absl::StrCat(pair.second.ip(), ":", pair.second.port()));
            DLOG(INFO) << "dest_node_address: " << dest_node_address;
            auto& pb_node = pair.second;
            Node dest_node(pb_node.ip(), pb_node.port(), pb_node.use_tls(), pb_node.role());
            scheduled_nodes[dest_node_address] = std::move(dest_node);
            thrds.emplace_back(
                std::thread(
                    &ABY3Scheduler::node_push_task,
                    this,
                    pair.first,              // node_id
                    std::ref(this->peer_dataset_map_),  // peer_dataset_map
                    std::ref(nodePushTaskRequest),  // nodePushTaskRequest
                    this->dataset_owner_,
                    std::ref(scheduled_nodes[dest_node_address])));
        }
    }

    for (auto &t : thrds) {
        t.join();
    }

}

} // namespace primihub::task
