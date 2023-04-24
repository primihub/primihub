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

using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;
using primihub::rpc::PsiTag;

namespace primihub::task {

void set_psi_request_param(const std::string &node_id,
                       const PeerDatasetMap &peer_dataset_map,
                       PushTaskRequest &taskRequest,
                       bool is_client) {
    // Add params to request
    google::protobuf::Map<std::string, ParamValue> *param_map =
        taskRequest.mutable_task()->mutable_params()->mutable_param_map();
    auto peer_dataset_map_it = peer_dataset_map.find(node_id);
    if (peer_dataset_map_it == peer_dataset_map.end()) {
        LOG(ERROR) << "node_push_task: peer_dataset_map not found";
        return;
    }

    std::vector<DatasetWithParamTag> dataset_param_list = peer_dataset_map_it->second;

    for (auto& dataset_param : dataset_param_list) {
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        VLOG(5) << "📤 push task dataset : " << dataset_param.first << ", " << dataset_param.second;
        pv.set_value_string(dataset_param.first);
        (*param_map)[dataset_param.second] = pv;
    }

    if (!is_client)
        return;


    // set server node dataset info
    // current psi task for one server with one dataset
    std::string server_address = "";
    for (auto &pair : taskRequest.task().node_map()) {
        if (pair.first == node_id) {
            continue;
        }
        std::string server_addr(
            absl::StrCat(pair.second.ip(), ":", pair.second.port()));

        if (server_address == "") {
            server_address = server_addr;
        } else {
            server_address = absl::StrCat(server_address, ",", server_addr);
        }

        auto peer_dataset_map_it = peer_dataset_map.find(pair.first);
        std::string dataset_path = "";
        for (auto &dataset_param : peer_dataset_map_it->second) {
            if (dataset_path == "") {
                dataset_path = dataset_param.first;
            } else {
                dataset_path = absl::StrCat(dataset_path, ",", dataset_param.first);
            }
            break;
        }

        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        pv.set_value_string(dataset_path);
        (*param_map)[server_address] = pv;
        DLOG(INFO) << "📤 push task dataset : "
                   << server_address << ", " << dataset_path;
        break;
    }
    ParamValue pv_addr;
    pv_addr.set_var_type(VarType::STRING);
    pv_addr.set_value_string(server_address);
    (*param_map)["serverAddress"] = pv_addr;
    DLOG(INFO) << "📤 push psi task server address : server_address, "
               << server_address;
}

void set_kkrt_psi_request_param(const std::string &node_id,
                                const PeerDatasetMap &peer_dataset_map,
                                PushTaskRequest &taskRequest,
                                bool is_client) {
    // Add params to request
    google::protobuf::Map<std::string, ParamValue> *param_map =
        taskRequest.mutable_task()->mutable_params()->mutable_param_map();
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
        (*param_map)[dataset_param.second] = pv;
    }

    std::string server_address = "";
    for (auto &pair : taskRequest.task().node_map()) {
        if (pair.first == node_id) { // get the server address for psi client and server
            if (is_client) {
                continue;
            }
        } else {
            if (!is_client) {
                continue;
            }
        }

        std::string server_addr(
            absl::StrCat(pair.second.ip(), ":", pair.second.port()));

        if (server_address == "") {
            server_address = server_addr;
        } else {
            server_address = absl::StrCat(server_address, ",", server_addr);
        }
    }

    ParamValue pv_addr;
    pv_addr.set_var_type(VarType::STRING);
    pv_addr.set_value_string(server_address);
    if (is_client) {
        (*param_map)["serverAddress"] = pv_addr;
        DLOG(INFO) << "📤 push psi task server address : server_address, "
                   << server_address;
    } else {
        (*param_map)["clientAddress"] = pv_addr;
        DLOG(INFO) << "📤 push psi task client address : server_address, "
                   << server_address;
    }
}
void PSIScheduler::node_push_psi_task(const std::string& node_id,
                                    const PeerDatasetMap& peer_dataset_map,
                                    const PushTaskRequest& nodePushTaskRequest,
                                    const Node& dest_node,
                                    bool is_client) {
    SET_THREAD_NAME("PSIScheduler");
    PushTaskReply pushTaskReply;
    PushTaskRequest _1NodePushTaskRequest;
    _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);
    const auto& params = nodePushTaskRequest.task().params().param_map();
    int psiTag = PsiTag::ECDH;
    auto param_it = params.find("psiTag");
    if (param_it != params.end()) {
        psiTag =param_it->second.value_int32();
    }
    if (psiTag == PsiTag::ECDH) {
        set_psi_request_param(node_id, peer_dataset_map, _1NodePushTaskRequest, is_client);
    } else if (psiTag == PsiTag::KKRT) {
        set_kkrt_psi_request_param(node_id, peer_dataset_map, _1NodePushTaskRequest, is_client);
    } else {
        LOG(ERROR) << "Unknown psiTag: " << psiTag;
        return ;
    }
    {
        // fill scheduler info
        auto node_map = _1NodePushTaskRequest.mutable_task()->mutable_node_map();
        auto& local_node = getLocalNodeCfg();
        rpc::Node scheduler_node;
        node2PbNode(local_node, &scheduler_node);
        (*node_map)[SCHEDULER_NODE] = std::move(scheduler_node);
    }
    // send request
    std::string dest_node_address = dest_node.to_string();
    LOG(INFO) << "dest node " << dest_node_address;
    auto channel = this->getLinkContext()->getChannel(dest_node);
    auto ret = channel->submitTask(_1NodePushTaskRequest, &pushTaskReply);
    if (ret == retcode::SUCCESS) {
        VLOG(5) << "submit task to : " << dest_node_address << " reply success";
    } else {
        LOG(ERROR) << "submit task to : " << dest_node_address << " reply failed";
    }
    parseNotifyServer(pushTaskReply);
}

void PSIScheduler::add_vm(rpc::Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(i);
}

void PSIScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
    PushTaskRequest nodePushTaskRequest;
    nodePushTaskRequest.CopyFrom(*pushTaskRequest);

    if (pushTaskRequest->task().type() == TaskType::PSI_TASK) {
        auto mutable_node_map = nodePushTaskRequest.mutable_task()->mutable_node_map();
        nodePushTaskRequest.mutable_task()->set_type(TaskType::NODE_PSI_TASK);

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

    LOG(INFO) << " 📧  Dispatch SubmitTask to PSI client node";

    std::vector<std::thread> thrds;
    std::map<std::string, Node> scheduled_nodes;
    // google::protobuf::Map<std::string, Node>
    const auto& node_map = nodePushTaskRequest.task().node_map();
     std::set<std::string> duplicate_filter;
    for (auto &pair : node_map) {
        auto peer_dataset_map_it = this->peer_dataset_map_.find(pair.first);
        if (peer_dataset_map_it == this->peer_dataset_map_.end()) {
            LOG(ERROR) << "dispatchTask: peer_dataset_map not found:  " << pair.first;
            return;
        }
        // std::vector<DatasetWithParamTag>
        const auto& dataset_param_list = peer_dataset_map_it->second;
        for (auto &dataset_param : dataset_param_list) {
            bool is_client = false;
            if (dataset_param.second == "clientData") {
                is_client = true;
            }
            //TODO (fixbug), maybe query dataset has some bug, temperary, filter the same destionation
            auto& pb_node = pair.second;
            std::string dest_node_address(absl::StrCat(pb_node.ip(), ":", pb_node.port()));
            if (duplicate_filter.find(dest_node_address) != duplicate_filter.end()) {
                VLOG(5) << "duplicate request for same destination, avoid";
                continue;
            }
            Node dest_node;
            pbNode2Node(pb_node, &dest_node);
            duplicate_filter.emplace(dest_node_address);
            VLOG(5) << "dest_node_address: " << dest_node.to_string();
            scheduled_nodes[dest_node_address] = std::move(dest_node);
            thrds.emplace_back(
                std::thread(
                    &PSIScheduler::node_push_psi_task,
                    this,
                    pair.first,              // node_id
                    this->peer_dataset_map_,  // peer_dataset_map
                    std::ref(nodePushTaskRequest),  // task request
                    std::ref(scheduled_nodes[dest_node_address]),
                    is_client));
        }
    }
    if (thrds.size() != 2) {
      LOG(ERROR) << "PSI needes two party participate, but get: " << thrds.size();
    }
    for (auto &t : thrds) {
        t.join();
    }
}

}
