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

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "absl/strings/str_cat.h"

#include "src/primihub/task/semantic/scheduler/tee_scheduler.h"

using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;
using primihub::rpc::Params;
using primihub::rpc::ParamValue;
using primihub::rpc::Task;
using primihub::rpc::TaskType;
using primihub::rpc::VarType;
using primihub::rpc::VirtualMachine;

namespace primihub::task {
void TEEScheduler::dispatch(const PushTaskRequest *push_request) {
    PushTaskRequest request;
    Node executor_node;
    request.CopyFrom(*push_request);

    if (push_request->task().type() == TaskType::TEE_TASK) {
        auto mutable_node_map = request.mutable_task()->mutable_node_map();
        request.mutable_task()->set_type(TaskType::TEE_DATAPROVIDER_TASK);

        int party_id = 0;

        for (int i = 0; i < this->peers_with_role_tag_.size(); i++) {

            std::string node_id = this->peers_with_role_tag_[i].first.node_id();
            std::string role = this->peers_with_role_tag_[i].second;

            if (role == "executor") {
                party_id = 0;
                executor_node.CopyFrom(this->peers_with_role_tag_[i].first);
                (*mutable_node_map)[node_id] = executor_node;

            } else if (role == "data_provider") {
                party_id += 1;
                Node data_pvd_node;
                data_pvd_node.CopyFrom(this->peers_with_role_tag_[i].first);
                (*mutable_node_map)[node_id] = data_pvd_node;
                // update node_map
                add_vm(&executor_node, &data_pvd_node, party_id, &request);
            }
        }
    }

    LOG(INFO) << " 📧  Dispatching task to: "
              << request.mutable_task()->mutable_node_map()->size() << " nodes";

    google::protobuf::Map<std::string, Node> node_map =
        request.task().node_map();
    for (auto &pair : node_map) {
        if (executor_node.node_id() == pair.second.node_id()) {
            // do nothing to executor node
            continue;
        }
        std::string dest_node_address(
            absl::StrCat(pair.second.ip(), ":", pair.second.port()));

        LOG(INFO) << " 📧  Dispatching task to: " << dest_node_address;
        this->push_task_to_node(pair.first, std::ref(request),
                                dest_node_address);
    }
}


void TEEScheduler::add_vm(Node *executor, Node *dpv, int party_id,
                          const PushTaskRequest *pushTaskRequest) {
    // Executor node
    VirtualMachine *dpv_vm = executor->add_vm();
    dpv_vm->set_party_id(party_id);
    EndPoint *ed_next = dpv_vm->mutable_next();

    std::string name_prefix = pushTaskRequest->task().job_id() + "_" +
                              pushTaskRequest->task().task_id() + "_";

    int session_basePort = 12120;

    ed_next->set_ip(dpv->ip());
    ed_next->set_port(session_basePort);
    ed_next->set_name(name_prefix + absl::StrCat(party_id));
    ed_next->set_link_type(LinkType::CLIENT);

    // Data provider node
    VirtualMachine *executor_vm = dpv->add_vm();
    executor_vm->set_party_id(0);
    EndPoint *ed_next_executor = executor_vm->mutable_next();
    ed_next_executor->set_ip(executor->ip());
    ed_next_executor->set_port(session_basePort);
    ed_next_executor->set_name(name_prefix + absl::StrCat(0));
    ed_next_executor->set_link_type(LinkType::SERVER);
}


void TEEScheduler::push_task_to_node (const std::string &node_id,
                                      const PushTaskRequest &request,
                                      const std::string &dest_node_address) {                              
    grpc::ClientContext context;
    grpc::Status status;
    auto channel = grpc::CreateChannel(
        dest_node_address, grpc::InsecureChannelCredentials());
    std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(channel);
    primihub::rpc::PushTaskReply response;
    status = stub->SubmitTask(&context, request, &response);
    if (!status.ok()) {
        LOG(ERROR) << "Failed to push task to node: " << node_id
                   << " with error: " << status.error_message();
    }
}

} // namespace primihub::task
