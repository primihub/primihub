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

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"

#include "src/primihub/task/semantic/scheduler/psi_scheduler.h"


//using primihub::rpc::EndPoint;
using primihub::rpc::Node;
using primihub::rpc::LinkType;
using primihub::rpc::ParamValue;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::VarType;
using primihub::rpc::PsiTag;

namespace primihub::task {

void PSIScheduler::set_psi_request_param(const std::string &node_id,
                       const PeerDatasetMap &peer_dataset_map,
                       PushTaskRequest &taskRequest,
                       bool is_client) {
    // Add params to request
    google::protobuf::Map<std::string, ParamValue> *param_map =
        taskRequest.mutable_task()->mutable_params()->mutable_param_map();
    auto peer_dataset_map_it = peer_dataset_map.find(node_id);
    if (peer_dataset_map_it == peer_dataset_map.end()) {
        LOG_ERROR() << "node_push_task: peer_dataset_map not found";
        return;
    }

    std::vector<DatasetWithParamTag> dataset_param_list = peer_dataset_map_it->second;

    for (auto &dataset_param : dataset_param_list) {
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        V_VLOG(5) << "📤 push task dataset : " << dataset_param.first << ", " << dataset_param.second;
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
        V_VLOG(1) << "📤 push task dataset : "
                   << server_address << ", " << dataset_path;
        break;
    }
    ParamValue pv_addr;
    pv_addr.set_var_type(VarType::STRING);
    pv_addr.set_value_string(server_address);
    (*param_map)["serverAddress"] = pv_addr;
    V_VLOG(1) << "📤 push psi task server address : server_address, "
               << server_address;
}

void PSIScheduler::set_kkrt_psi_request_param(const std::string &node_id,
                                const PeerDatasetMap &peer_dataset_map,
                                PushTaskRequest &taskRequest,
                                bool is_client) {
    // Add params to request
    google::protobuf::Map<std::string, ParamValue> *param_map =
        taskRequest.mutable_task()->mutable_params()->mutable_param_map();
    auto peer_dataset_map_it = peer_dataset_map.find(node_id);
    if (peer_dataset_map_it == peer_dataset_map.end()) {
        LOG_ERROR() << "node_push_task: peer_dataset_map not found";
        return;
    }

    std::vector<DatasetWithParamTag> dataset_param_list = peer_dataset_map_it->second;
    for (auto &dataset_param : dataset_param_list) {
        ParamValue pv;
        pv.set_var_type(VarType::STRING);
        V_VLOG(5) << "📤 push task dataset : " << dataset_param.first << ", " << dataset_param.second;
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
        V_VLOG(5) << "📤 push psi task server address : server_address, "
                   << server_address;
    } else {
        (*param_map)["clientAddress"] = pv_addr;
        V_VLOG(5) << "📤 push psi task client address : server_address, "
                   << server_address;
    }
}

void PSIScheduler::node_push_psi_task(const std::string &node_id,
                    const PeerDatasetMap &peer_dataset_map,
                    const PushTaskRequest &nodePushTaskRequest,
                    std::string dest_node_address,
                    bool is_client) {
    grpc::ClientContext context;

    PushTaskReply pushTaskReply;
    PushTaskRequest _1NodePushTaskRequest;
    _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);

    auto params = nodePushTaskRequest.task().params().param_map();
    int psiTag = PsiTag::ECDH;
    auto param_it = params.find("psiTag");
    if (param_it != params.end()) {
        psiTag = params["psiTag"].value_int32();
    }

    if (psiTag == PsiTag::ECDH) {
        set_psi_request_param(node_id, peer_dataset_map,
                              _1NodePushTaskRequest, is_client);
    } else if (psiTag == PsiTag::KKRT) {
        set_kkrt_psi_request_param(node_id, peer_dataset_map,
			           _1NodePushTaskRequest, is_client);
    } else {
        LOG_ERROR() << "psiTag is set error.";
        return ;
    }

    // send request
    LOG_INFO() << "dest node " << dest_node_address;
    std::unique_ptr<VMNode::Stub> stub_ = VMNode::NewStub(grpc::CreateChannel(
        dest_node_address, grpc::InsecureChannelCredentials()));
    // const auto& task_id = _1NodePushTaskRequest.task().task_id();
    // const auto& job_id = _1NodePushTaskRequest.task().job_id();
    // V_VLOG(5) << "task_id: " << task_id << " job_id: " << job_id;
    Status status =
        stub_->SubmitTask(&context, _1NodePushTaskRequest, &pushTaskReply);
    if (status.ok()) {
        if (is_client) {
            LOG_INFO() << "Node push psi task rpc succeeded.";
        } else {
            LOG_INFO() << "Psi task server node is active.";
        }
    } else {
        if (is_client) {
            LOG_ERROR() << "Node push psi task rpc failed. "
                       << status.error_code() << ": " << status.error_message();
        } else {
            LOG_ERROR() << "Psi task server node is inactive."
                       << status.error_code() << ": " << status.error_message();
        }
    }
}

void PSIScheduler::add_vm(Node *node, int i,
                         const PushTaskRequest *pushTaskRequest) {
    VirtualMachine *vm = node->add_vm();
    vm->set_party_id(i);
}

void PSIScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
    PushTaskRequest nodePushTaskRequest;
    nodePushTaskRequest.CopyFrom(*pushTaskRequest);
    // const auto& task_id = nodePushTaskRequest.task().task_id();
    // const auto& job_id = nodePushTaskRequest.task().job_id();
    // V_VLOG(5) << "task_id: " << task_id << " job_id: " << job_id;
    if (pushTaskRequest->task().type() == TaskType::PSI_TASK) {
        google::protobuf::Map<std::string, Node> *mutable_node_map =
            nodePushTaskRequest.mutable_task()->mutable_node_map();
        nodePushTaskRequest.mutable_task()->set_type(TaskType::NODE_PSI_TASK);

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

    LOG_INFO() << " 📧  Dispatch SubmitTask to PSI client node";

    std::vector<std::thread> thrds;
    // google::protobuf::Map<std::string, Node>
    const auto& node_map = nodePushTaskRequest.task().node_map();
     std::set<std::string> duplicate_filter;
    for (auto &pair : node_map) {
        auto peer_dataset_map_it = this->peer_dataset_map_.find(pair.first);
        if (peer_dataset_map_it == this->peer_dataset_map_.end()) {
            LOG_ERROR() << "dispatchTask: peer_dataset_map not found";
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
            std::string dest_node_address(absl::StrCat(pair.second.ip(), ":", pair.second.port()));
            if (duplicate_filter.find(dest_node_address) != duplicate_filter.end()) {
                V_VLOG(5) << "duplicate request for same destination, avoid";
                continue;
            }
            duplicate_filter.emplace(dest_node_address);
            V_VLOG(5) << "dest_node_address: " << dest_node_address;

            thrds.emplace_back(
                std::thread(&PSIScheduler::node_push_psi_task,
                            this,
                            pair.first,              // node_id
                            this->peer_dataset_map_,  // peer_dataset_map
                            std::ref(nodePushTaskRequest),  // nodePushTaskRequest
                            dest_node_address,
                            is_client));
        }
    }

    for (auto &t : thrds) {
        t.join();
    }
}

}
