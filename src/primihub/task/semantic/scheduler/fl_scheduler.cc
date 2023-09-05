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
        std::string server_info = dataset_meta->getServerInfo();
        Node node_info;
        node_info.fromString(server_info);
        // Only set dataset path
        pv_dataset.set_value_string(dataset_meta->getAccessInfo());
        (*params_map)[dataset_meta->getDescription()] = std::move(pv_dataset);

        node_dataset_map[node_info.id()] = dataset_meta->getDescription();
    }

    // Save every node's dataset name.
    for (auto pair : node_dataset_map) {
        std::string key = pair.first + "_dataset";
        VLOG(7) << "key: " << key << " value: " << pair.second;
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
  AddSchedulerNode(task_ptr);
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->executeTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << "submit task to : " << dest_node_address << " reply success";
  } else {
    LOG(ERROR) << "submit task to : " << dest_node_address << " reply failed";
    return retcode::FAIL;
  }
  parseNotifyServer(reply);
  return retcode::SUCCESS;
}

/**
 * @brief Dispatch FL task to different role. eg: xgboost host & guest.
 *
 */
retcode FLScheduler::dispatch(const PushTaskRequest *pushTaskRequest) {
  PushTaskRequest send_request;
  send_request.CopyFrom(*pushTaskRequest);
  std::string str;
  google::protobuf::TextFormat::PrintToString(send_request, &str);
  LOG(INFO) << "FLScheduler::dispatch: " << str;

  // schedule
  std::vector<std::thread> thrds;
  const auto& party_access_info = send_request.task().party_access_info();
  for (const auto& [party_name, node] : party_access_info) {
    this->error_msg_.insert({party_name, ""});
  }
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
  if (has_error()) {
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
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
