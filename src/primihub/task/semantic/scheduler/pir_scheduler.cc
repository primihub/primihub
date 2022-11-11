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
#include "src/primihub/task/semantic/scheduler/pir_scheduler.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"


//using primihub::rpc::EndPoint;
using primihub::rpc::Node;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;
using primihub::rpc::PirType;

namespace primihub::task {

void set_pir_request_param(const std::string &node_id,
                       const PeerDatasetMap &peer_dataset_map,
                       PushTaskRequest &taskRequest,
                       bool is_client) {
    google::protobuf::Map<std::string, ParamValue> *param_map =
        taskRequest.mutable_task()->mutable_params()->mutable_param_map();

    if (!is_client)
        return;

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

void set_keyword_pir_request_param(const std::string &node_id,
                                   const PeerDatasetMap &peer_dataset_map,
                                   PushTaskRequest &taskRequest,
                                   bool is_client) {

    // Add params to request
    // google::protobuf::Map<std::string, ParamValue>
    auto param_map = taskRequest.mutable_task()->mutable_params()->mutable_param_map();
    auto peer_dataset_map_it = peer_dataset_map.find(node_id);
    if (peer_dataset_map_it != peer_dataset_map.end()) {

        VLOG(5) << "peer_dataset_map: " << peer_dataset_map.size();
        const std::vector<DatasetWithParamTag>& dataset_param_list = peer_dataset_map_it->second;
        for (const auto& dataset_param : dataset_param_list) {
            ParamValue pv;
            pv.set_var_type(VarType::STRING);
            DLOG(INFO) << "📤 push task dataset : " << dataset_param.first << ", " << dataset_param.second;
            pv.set_value_string(dataset_param.first);
            (*param_map)[dataset_param.second] = pv;
        }
    }

    std::string server_address{""};
    VLOG(5) << "node_id: " << node_id;
    for (auto &pair : taskRequest.task().node_map()) {
        auto& _node_id = pair.first;
        if (peer_dataset_map.find(_node_id) == peer_dataset_map.end()) {
            continue;
        }
        if (_node_id == node_id) { // get the server address for psi client and server
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
            VLOG(5) << "server_addr: " << server_addr;
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
        VLOG(3) << "📤 push pir task server address : server_address, "
                   << server_address;
    } else {
        // erase client data to avoid data leak
        param_map->erase("clientData");
        (*param_map)["clientAddress"] = pv_addr;
        VLOG(3) << "📤 push pir task client address : server_address, "
                   << server_address;
    }
}

void node_push_pir_task(const std::string &node_id,
                        const PeerDatasetMap &peer_dataset_map,
                        const PushTaskRequest &nodePushTaskRequest,
                        std::string dest_node_address, bool is_client) {
    grpc::ClientContext context;
    PushTaskReply pushTaskReply;
    PushTaskRequest _1NodePushTaskRequest;
    _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);

    auto params = nodePushTaskRequest.task().params().param_map();
    int pirType = PirType::ID_PIR;
    auto param_it = params.find("pirType");
    if (param_it != params.end()) {
        pirType = params["pirType"].value_int32();
    }

    if (pirType == PirType::ID_PIR) {
        set_pir_request_param(node_id, peer_dataset_map,
                              _1NodePushTaskRequest, is_client);
    } else if (pirType == PirType::KEY_PIR) {
        set_keyword_pir_request_param(node_id, peer_dataset_map,
                                      _1NodePushTaskRequest, is_client);
    } else {
        LOG(ERROR) << "pirType is set error.";
        return ;
    }

    // send request
    VLOG(5) << "begin to submit task to: " << dest_node_address;
    std::unique_ptr<VMNode::Stub> stub_ = VMNode::NewStub(grpc::CreateChannel(
        dest_node_address, grpc::InsecureChannelCredentials()));
    Status status =
        stub_->SubmitTask(&context, _1NodePushTaskRequest, &pushTaskReply);
    if (status.ok()) {
        LOG(INFO) << "Node push pir task rpc succeeded for remot node: " << dest_node_address;
    } else {
        LOG(ERROR) << "Node push pir task rpc failed to node: " << node_id << " address: " << dest_node_address;
    }
    VLOG(5) << "dest_node: " << dest_node_address << " reply success";
}

void PIRScheduler::add_vm(Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(i);
}

void PIRScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
    PushTaskRequest nodePushTaskRequest;
    nodePushTaskRequest.CopyFrom(*pushTaskRequest);

    const auto& params = nodePushTaskRequest.task().params().param_map();
    int pirType = PirType::ID_PIR;
    auto param_it = params.find("pirType");
    if (param_it != params.end()) {
        pirType = param_it->second.value_int32();
    }

    if (pushTaskRequest->task().type() == TaskType::PIR_TASK) {
        google::protobuf::Map<std::string, Node> *mutable_node_map =
            nodePushTaskRequest.mutable_task()->mutable_node_map();
        nodePushTaskRequest.mutable_task()->set_type(TaskType::NODE_PIR_TASK);

        for (size_t i = 0; i < peer_list_.size(); i++) {
            Node single_node;
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

    LOG(INFO) << " 📧  Dispatch SubmitTask to PIR client node " << this->get_node_id();
    std::set<std::string> duplicate_server;
    std::vector<std::thread> thrds;
    // google::protobuf::Map<std::string, Node>
    const auto& node_map = nodePushTaskRequest.task().node_map();
    for (const auto& pair : node_map) {
        VLOG(5) << "pair.first_pair.first_pair.first: " << pair.first << " peer_list_: " << peer_list_.size();
        if (pirType == PirType::ID_PIR) {
            bool is_client = pair.first == node_id_ ? true : false;

            std::string dest_node_address(
                absl::StrCat(pair.second.ip(), ":", pair.second.port()));
            DLOG(INFO) << "dest_node_address: " << dest_node_address;

            if (duplicate_server.find(dest_node_address) != duplicate_server.end()) {
                continue;
            }
            duplicate_server.emplace(dest_node_address);
            thrds.emplace_back(
                std::thread(node_push_pir_task,
                            pair.first,                      // node_id
                            this->peer_dataset_map_,         // peer_dataset_map
                            std::ref(nodePushTaskRequest),   // nodePushTaskRequest
                            dest_node_address,
                            is_client));
        } else if (pirType == PirType::KEY_PIR) {
            auto peer_dataset_map_it = this->peer_dataset_map_.find(pair.first);
            for (const auto& it : peer_dataset_map_) {
                LOG(INFO) << "peer_dataset_map_detail: " << it.first;
            }
            auto& node_id = pair.first;
            bool is_client = pair.first == this->get_node_id() ? true : false;

            // LOG(INFO) << "peer_dataset_map_: " << pair.first << " current node_id: " << this->get_node_id()
            //         << " peer_dataset_map_ size: " << peer_dataset_map_.size();
            // if (peer_dataset_map_it == this->peer_dataset_map_.end()) {
            //     if (node_id == this->get_node_id()) { // role as control node
            //         i
            //     }
            //     LOG(ERROR) << "dispatchTask: peer_dataset_map not found";
            //     return;
            // }
            // const std::vector<DatasetWithParamTag>& dataset_param_list = peer_dataset_map_it->second;
            // for (const auto& dataset_param : dataset_param_list) {
            // if (dataset_param.second == "clientData") {
            //     is_client = true;
            //     LOG(ERROR) << "node_id: " << node_id << " is as role of client";
            // }
            std::string dest_node_address(absl::StrCat(pair.second.ip(), ":", pair.second.port()));
            VLOG(5) << "dest_node_address: " << dest_node_address;
            if (duplicate_server.find(dest_node_address) != duplicate_server.end()) {
                continue;
            }
            duplicate_server.emplace(dest_node_address);
            thrds.emplace_back(
                std::thread(node_push_pir_task,
                            pair.first,                     // node_id
                            this->peer_dataset_map_,        // peer_dataset_map
                            std::ref(nodePushTaskRequest),  // nodePushTaskRequest
                            dest_node_address,
                            is_client));
            // }
        } else {
            LOG(ERROR) << "The pir type is error";
            return;
        }
    }
    // wait until all task finished
    for (auto &t : thrds) {
        t.join();
    }
    VLOG(5) << "end of PIRScheduler::dispatch";
}


int PIRScheduler::transformRequest(PushTaskRequest &taskRequest) {
    if (taskRequest.task().type() == TaskType::PIR_TASK) {
        google::protobuf::Map<std::string, Node> *mutable_node_map =
            taskRequest.mutable_task()->mutable_node_map();
        taskRequest.mutable_task()->set_type(TaskType::NODE_PIR_TASK);

        for (size_t i = 0; i < peer_list_.size(); i++) {
            Node single_node;
            single_node.CopyFrom(peer_list_[i]);
            std::string node_id = peer_list_[i].node_id();
            if (singleton_) {
                for (size_t j = 0; j < peer_list_.size(); j++) {
                    add_vm(&single_node, j, &taskRequest);
                }
                (*mutable_node_map)[node_id] = single_node;
                break;
            } else {
                add_vm(&single_node, i, &taskRequest);
            }
            (*mutable_node_map)[node_id] = single_node;
        }
    }

    google::protobuf::Map<std::string, Node> node_map =
        taskRequest.task().node_map();

    auto node_map_it = node_map.find(node_id_);
    if (node_map_it == node_map.end()) {
        LOG(ERROR) << "Find node info failed: " << node_id_;
        return -1;
    }

    set_pir_request_param(node_id_, this->peer_dataset_map_,
                          taskRequest, true);

    return 0;
}

} // namespace primihub::task
