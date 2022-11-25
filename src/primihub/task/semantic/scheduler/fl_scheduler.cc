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
#include <memory>

#include "absl/strings/str_cat.h"

#include "src/primihub/task/semantic/scheduler/fl_scheduler.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/service/notify/model.h"

using grpc::Channel;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using primihub::rpc::Task;
using primihub::rpc::Params;
using primihub::rpc::ParamValue;
using primihub::rpc::VarType;
using primihub::rpc::TaskType;
using primihub::rpc::VirtualMachine;
using primihub::rpc::EndPoint;
using primihub::rpc::LinkType;

using primihub::service::DataURLToDetail;
using primihub::service::EventBusNotifyDelegate;

namespace primihub::task {
    
    void nodeContext2TaskParam(NodeContext node_context, 
                               const std::vector<std::shared_ptr<DatasetMeta>> &dataset_meta_list,
                               PushTaskRequest* node_task_request) {
        google::protobuf::Map<std::string, ParamValue> *params_map = 
                    node_task_request->mutable_task()->mutable_params()->mutable_param_map();
        // Role
        ParamValue pv_role;
        pv_role.set_var_type(VarType::STRING);
        pv_role.set_value_string(node_context.role);
        (*params_map)["role"] = pv_role;

        // Protocol
        ParamValue pv_protocol;
        pv_protocol.set_var_type(VarType::STRING);
        pv_protocol.set_value_string(node_context.protocol);
        (*params_map)["protocol"] = pv_protocol;

        // topic
        std::string job_id = node_task_request->task().job_id();
        std::string task_id = node_task_request->task().task_id();
//        static std::atomic<int64_t> index = 0;
//        std::string topic_content = job_id + task_id + std::to_string(index.load());
        std::string topic_content = task_id;
//        index++;
        ParamValue pv_topic;
        pv_topic.set_var_type(VarType::STRING);
        pv_topic.set_value_string(topic_content);
        (*params_map)["topic"] = pv_topic;


        // Dataset meta
        std::map<std::string, std::string> node_dataset_map;
        for (auto &dataset_meta : dataset_meta_list) {
            ParamValue pv_dataset;
            pv_dataset.set_var_type(VarType::STRING);
            // Get data path from data URL
            std::string node_id, node_ip, dataset_path;
            int node_port;
            std::string data_url = dataset_meta->getDataURL();
            DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);
            // Only set dataset path
            pv_dataset.set_value_string(dataset_path);
            (*params_map)[dataset_meta->getDescription()] = pv_dataset;

            node_dataset_map[node_id] = dataset_meta->getDescription();
        }
        
        // Save every node's dataset name.
        for (auto pair : node_dataset_map) {
          std::string key = pair.first + "_dataset";
          ParamValue pv_name;
          pv_name.set_var_type(VarType::STRING);
          pv_name.set_value_string(pair.second);
          (*params_map)[key] = pv_name;
        }

        // Dataset key list
        for (size_t i = 0; i < node_context.datasets.size(); i++) {
           node_task_request->mutable_task()->add_input_datasets(node_context.datasets[i]);
        }
        // dumps code
        node_task_request->mutable_task()->set_code(node_context.dumps_func);
    }

    void push_node_py_task(const std::string &node_id,
                          const std::string &role,
                          const std::string &dest_node_address,
                          const PushTaskRequest &nodePushTaskRequest,
                          const PeerContextMap peer_context_map,
                          const std::vector<std::shared_ptr<DatasetMeta>> &dataset_meta_list) {
        grpc::ClientContext context;
        PushTaskReply pushTaskReply;
        PushTaskRequest _1NodePushTaskRequest;
        _1NodePushTaskRequest.CopyFrom(nodePushTaskRequest);
        NodeContext peer_context = peer_context_map.find(role)->second;
        nodeContext2TaskParam(peer_context, dataset_meta_list, &_1NodePushTaskRequest);

        std::unique_ptr<VMNode::Stub> stub_ = VMNode::NewStub(grpc::CreateChannel(
            dest_node_address, grpc::InsecureChannelCredentials()));
        Status status =
            stub_->SubmitTask(&context, _1NodePushTaskRequest, &pushTaskReply);

        if (status.ok()) {
            LOG(INFO) << "Node push task rpc succeeded.";
        } else {
            LOG(ERROR) << "Node push task rpc failed.";
        }
    }

    /**
     * @brief Dispatch FL task to different role. eg: xgboost host & guest.
     * 
     */
    void FLScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
        
        PushTaskRequest nodePushTaskRequest;
        nodePushTaskRequest.CopyFrom(*pushTaskRequest);
        // Construct node map
        if (pushTaskRequest->task().type() == TaskType::ACTOR_TASK) {
            google::protobuf::Map<std::string, Node> *mutable_node_map =
                nodePushTaskRequest.mutable_task()->mutable_node_map();
            nodePushTaskRequest.mutable_task()->set_type(TaskType::NODE_TASK);
            
            // role: host -> party 0   role: guest -> party 1
            for (size_t i = 0; i < this->peers_with_tag_.size(); i++) {
                Node single_node;

                single_node.CopyFrom(this->peers_with_tag_[i].first);
                std::string node_id = this->peers_with_tag_[i].first.node_id();
                std::string role = this->peers_with_tag_[i].second;
                
                int party_id = 0;
                if (role == "host") {
                   party_id = 0;
                } else if (role == "guest") {
                    party_id = 1;
                }
                (*mutable_node_map)[node_id] = single_node;
                add_vm(&single_node, party_id, 2, &nodePushTaskRequest);
                //Update single_node with vm info
                (*mutable_node_map)[node_id] = single_node;
            }
        }

        // TODO Fire TASK_STATUS event using NotifyService
        auto taskId = nodePushTaskRequest.task().task_id();
        auto submitClientId = nodePushTaskRequest.submit_client_id();

        LOG(INFO) << "nodePushTaskRequest task_id: " << taskId;
        LOG(INFO) << "nodePushTaskRequest submit_client_id: " << submitClientId;
        
        
        EventBusNotifyDelegate::getInstance().notifyStatus(taskId, submitClientId, 
                                                            "RUNNING", 
                                                            "task status test message");

        // schedule
        std::vector<std::thread> thrds;
        for (size_t i = 0; i < peers_with_tag_.size(); i++) {
            NodeWithRoleTag peer_with_tag = peers_with_tag_[i];
          
            std::string dest_node_address(
                    absl::StrCat(peer_with_tag.first.ip(), ":", peer_with_tag.first.port()));
            LOG(INFO) << "dest_node_address: " << dest_node_address;
            // TODO 获取当Role的data meta list
            std::vector<std::shared_ptr<DatasetMeta>> data_meta_list;
            getDataMetaListByRole(peer_with_tag.second, &data_meta_list);
            thrds.emplace_back(std::thread(push_node_py_task,
                                               peer_with_tag.first.node_id(),    // node_id
                                               peer_with_tag.second,             // role
                                               dest_node_address,               // dest_node_address
                                               std::ref(nodePushTaskRequest), // nodePushTaskRequest
                                               this->peer_context_map_,
                                               data_meta_list
                                               ));
            
        }

        for (auto &t : thrds) {
            t.join();
        }
    }

    void FLScheduler::getDataMetaListByRole(const std::string &role,
                                            std::vector<std::shared_ptr<DatasetMeta>> *data_meta_list) {
        for (size_t i = 0; i < this->metas_with_role_tag_.size(); i++) {
            if (this->metas_with_role_tag_[i].second == role) {
                data_meta_list->push_back(this->metas_with_role_tag_[i].first);
            }
        }
    }

    void FLScheduler::add_vm(Node *node, int i, int role_num, 
                            const PushTaskRequest *pushTaskRequest) {
        int ret = 0;
        for (auto node_with_tag : peers_with_tag_) {
            VirtualMachine *vm = node->add_vm();
            EndPoint *ep_next = vm->mutable_next();
            ep_next->set_ip(node_with_tag.first.ip());
            ep_next->set_link_type(LinkType::SERVER);
            ep_next->set_port(node_with_tag.first.data_port());

            std::string ep_name =
                node_with_tag.first.node_id() + "_" + node_with_tag.second;
            ep_next->set_name(ep_name);
        }
    }
} // namespace primihub::task
