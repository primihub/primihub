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

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/task/semantic/scheduler/factory.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"

using primihub::service::DataURLToDetail;
using primihub::rpc::TaskType;
namespace pb_util = primihub::proto::util;
namespace primihub::task {

/**
 * @brief Task syntx tree parser.
 *
 * 1. Get input task syntax tree from language parser
 * 2. Find all datasets in the task syntax tree using dataset service.
 * 3. Generate a scheduler object
 */
retcode ProtocolSemanticParser::parseTaskSyntaxTree(
    std::shared_ptr<LanguageParser> lan_parser) {
  if (lan_parser == nullptr) {
    LOG(ERROR) << "no language handler found";
    return retcode::FAIL;
  }
  retcode ret_code{retcode::SUCCESS};
  auto task_language = lan_parser->getPushTaskRequest().task().language();
  switch (task_language) {
  case Language::PROTO:
    ret_code = scheduleProtoTask(lan_parser);
    break;
  case Language::PYTHON:
    ret_code = schedulePythonTask(lan_parser);
    break;
  default:
    auto& request = lan_parser->getPushTaskRequest();
    const auto& task_info = request.task().task_info();
    LOG(WARNING) << pb_util::TaskInfoToString(task_info)
                 << "unsupported language type: " << task_language;
    ret_code = retcode::FAIL;
    break;
  }
  return ret_code;
}

void ProtocolSemanticParser::parseTaskServer(
    const std::vector<Node>& task_servers) {
  for (const auto& node : task_servers) {
    task_server_.push_back(node);
  }
}

bool ProtocolSemanticParser::DatasetMetaSerivceEnabled() {
  return true;
}

retcode ProtocolSemanticParser::ProcessAuxiliaryServer(
    LanguageParser* _parser) {
  auto& task_request = _parser->getPushTaskRequest();
  const auto& param_map = task_request.task().params().param_map();
  auto it = param_map.find("auxiliary_server");
  if (it == param_map.end()) {
    return retcode::SUCCESS;
  }
  const auto& task_info = task_request.task().task_info();
  rpc::Node auxiliary_node;
  std::string info = it->second.value_string();
  auto task_info_str = pb_util::TaskInfoToString(task_info);
  nlohmann::json js_info = nlohmann::json::parse(info);
  bool is_dataset = js_info["is_dataset"].get<bool>();
  if (is_dataset) {
    std::string dataset_id = js_info["dataset_id"].get<std::string>();
    std::vector<std::pair<std::string, std::string>> datasets_with_tag = {
      {dataset_id, "default"}
    };
    dataset_service_->MetaService()->FindPeerListFromDatasets(
    datasets_with_tag,
    [&, this](std::vector<DatasetMetaWithParamTag> &metas_with_param_tag) {
      VLOG(2) << task_info_str
              << "Find meta list from datasets: "
              << metas_with_param_tag.size();
      if (metas_with_param_tag.size() != 1) {
        return;
      }
      auto& meta_with_tag = metas_with_param_tag[0];
      const auto& meta_info = meta_with_tag.first;
      const auto& party_name = meta_with_tag.second;
      std::string server_info = meta_info->getServerInfo();
      Node access_info;
      access_info.fromString(server_info);
      node2PbNode(access_info, &auxiliary_node);
    });
  } else {
    std::string ip = js_info["ip"].get<std::string>();
    uint32_t port = js_info["port"].get<uint32_t>();
    bool use_tls = js_info["use_tls"].get<bool>();
    auxiliary_node.set_ip(ip);
    auxiliary_node.set_port(port);
    auxiliary_node.set_use_tls(use_tls);
  }
  auto auxiliary_server_ptr =
      task_request.mutable_task()->mutable_auxiliary_server();
  (*auxiliary_server_ptr)[AUX_COMPUTE_NODE] = std::move(auxiliary_node);
  return retcode::SUCCESS;
}

retcode ProtocolSemanticParser::ParseDatasetToPartyAccessInfo(
    LanguageParser* _parser) {
  auto& task_request = _parser->getPushTaskRequest();
  const auto& party_access_info = task_request.task().party_access_info();
  if (party_access_info.size() > 0) {
  } else {
    if (DatasetMetaSerivceEnabled()) {
      auto& task_request = _parser->getPushTaskRequest();
      const auto& task_info = task_request.task().task_info();
      auto task_info_str = pb_util::TaskInfoToString(task_info);
      auto datasets_with_tag = _parser->getDatasets();
      VLOG(2) << task_info_str << "Finding meta list from datasets...";
      // get party access info from dataset meta service using dataset id
      dataset_service_->MetaService()->FindPeerListFromDatasets(
        datasets_with_tag,
        [&, this](std::vector<DatasetMetaWithParamTag>& metas_with_param_tag) {
          VLOG(2) << task_info_str
                  << "Find meta list from datasets: "
                  << metas_with_param_tag.size();
          std::map<std::string, Node> party_access_info;
          metasToPartyAccessInfo(metas_with_param_tag, &party_access_info);
          _parser->MergePartyAccessInfo(party_access_info);
          VLOG(2) << task_info_str << "end of MergePartyAccessInfo";
        });
      // process auxiliary server if search by dataset
      ProcessAuxiliaryServer(_parser);
    }
  }
  LOG(INFO) << "number of parties: " << party_access_info.size();

  return retcode::SUCCESS;
}

retcode ProtocolSemanticParser::scheduleProtoTask(
    std::shared_ptr<LanguageParser> proto_parser) {
  auto _proto_parser = std::dynamic_pointer_cast<ProtoParser>(proto_parser);
  ParseDatasetToPartyAccessInfo(proto_parser.get());
  // schedule task
  auto& task_request = _proto_parser->getPushTaskRequest();
  auto& task_config = task_request.task();
  const auto& task_info = task_request.task().task_info();
  auto task_info_str = pb_util::TaskInfoToString(task_info);
  if (task_config.party_access_info().empty()) {
    LOG(ERROR) << task_info_str << "no party access config found for dispatch";
    return retcode::FAIL;
  }
  auto scheduler_ptr = SchedulerFactory::CreateScheduler(task_config);
  if (scheduler_ptr == nullptr) {
    LOG(ERROR) << task_info_str << "no scheduler created to dispatch task";
    return retcode::FAIL;
  }
  auto ret = scheduler_ptr->dispatch(&task_request);
  parseTaskServer(scheduler_ptr->taskServer());
  return ret;
}

retcode ProtocolSemanticParser::schedulePythonTask(
    std::shared_ptr<LanguageParser> python_parser) {
  auto _python_parser = std::dynamic_pointer_cast<PyParser>(python_parser);
  ParseDatasetToPartyAccessInfo(python_parser.get());
  // schedule task
  auto& task_request = _python_parser->getPushTaskRequest();
  auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  auto task_info_str = pb_util::TaskInfoToString(task_info);
  if (task_config.party_access_info().empty()) {
    LOG(ERROR) << task_info_str << "no party access config found for dispatch";
    return retcode::FAIL;
  }
  auto scheduler_ptr = SchedulerFactory::CreateScheduler(task_config);
  if (scheduler_ptr == nullptr) {
    LOG(ERROR) << task_info_str << "no scheduler created to dispatch task";
    return retcode::FAIL;
  }
  auto ret = scheduler_ptr->dispatch(&task_request);
  parseTaskServer(scheduler_ptr->taskServer());
  return ret;
}

void ProtocolSemanticParser::metasToDatasetAndOwner(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    std::map<std::string, std::string> &dataset_owner) {
    for (auto &pair: metas_with_tag) {
        auto meta = pair.first;
        //   std::string node_id, node_ip, dataset_path;
        //   int node_port;
        std::string dataset_path;
        std::string data_url = meta->getDataURL();
        Node node_info;
        DataURLToDetail(data_url, node_info, dataset_path);
        dataset_owner.insert(std::make_pair(meta->getDescription(), node_info.id()));

        LOG(INFO) << "Dataset " << meta->getDescription() << "'s owner is "
                << node_info.id() << ".";
    }
}

void ProtocolSemanticParser::metasToPeerList(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    std::vector<rpc::Node> &peers) {
  // reset peers
  peers.clear();

  for (auto &meta_with_tag : metas_with_tag) {
    auto _meta = meta_with_tag.first;
    Node node_info;
    std::string server_info = _meta->getServerInfo();
    node_info.fromString(server_info);
    rpc::Node node;
    primihub::node2PbNode(node_info, &node);
    bool is_new_peer = true;
    for (auto &peer : peers) {
      if (peer.node_id() == node_info.id()) {
        is_new_peer = false;
        break;
      }
    }
    if (is_new_peer) {
      peers.push_back(node);
    }
  }
}
void ProtocolSemanticParser::metasToPartyAccessInfo(
    const std::vector<DatasetMetaWithParamTag>& metas_with_tag,
    std::map<std::string, Node>* party_access_info) {
  party_access_info->clear();
  std::map<std::string, std::set<std::string>> duplicate_filter;
  for (const auto& meta_with_tag : metas_with_tag) {
    const auto& meta_info = meta_with_tag.first;
    const auto& party_name = meta_with_tag.second;
    std::string server_info = meta_info->getServerInfo();
    Node access_info;
    access_info.fromString(server_info);

    auto it = party_access_info->find(party_name);
    if (it != party_access_info->end()) {
      LOG(WARNING) << "update access info for party_name: " << party_name;
    }
    (*party_access_info)[party_name] = access_info;
  }
}
// output  key: node_id, value: <dataset_path, dataset_name>
void ProtocolSemanticParser::metasToPeerDatasetMap(
    const std::vector<DatasetMetaWithParamTag> &metas_with_param_tag,
    PeerDatasetMap &peer_dataset_map) {
  for (auto &meta : metas_with_param_tag) {
    auto _meta = meta.first;
    auto _param_tag = meta.second;
    VLOG(5) << "metasToPeerDatasetMap: " << _meta->getDataURL() << " "
               << _param_tag;
    Node node_info;
    auto server_info = _meta->getServerInfo();
    node_info.fromString(server_info);
    std::string dataset_id = _meta->id;
    // find node_id in peer_dataset_map
    auto it = peer_dataset_map.find(node_info.id());
    if (it == peer_dataset_map.end()) {
      // create new peer
      std::vector<DatasetWithParamTag> datasets_with_tag;
      datasets_with_tag.push_back(std::make_pair(dataset_id, _param_tag));
      peer_dataset_map[node_info.id()] = datasets_with_tag;
    } else {
      // add dataset to peer
      it->second.push_back(std::make_pair(dataset_id, _param_tag));
    }
  }
}

void ProtocolSemanticParser::metasToPeerWithTagAndPort(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    const PeerContextMap &peer_context_map,
    std::vector<NodeWithRoleTag> &peers_with_tag) {
  bool errors = false;
  for (auto &meta_with_tag : metas_with_tag) {
    auto meta = meta_with_tag.first;
    auto tag = meta_with_tag.second;

    Node node_info;
    std::string server_info = meta->getServerInfo();
    node_info.fromString(server_info);
    // Get tcp port used by FL algorithm.
    std::string ds_name = meta->getDescription();
    VLOG(7) << "ds_name: " << ds_name << " tag: " << tag;
    // auto &ds_port_map = peer_context_map[tag].dataset_port_map;
    auto& ds_port_map = peer_context_map.find(tag)->second.dataset_port_map;
    auto iter = ds_port_map.find(ds_name);
    if (iter == ds_port_map.end()) {
      LOG(ERROR) << "Can't find data port for dataset " << ds_name << ".";
      errors = true;
      break;
    }

    rpc::Node node;
    node2PbNode(node_info, &node);
    bool is_new_peer = true;
    for (auto &peer : peers_with_tag) {
      if (peer.first.node_id() == node_info.id()) {
        is_new_peer = false;
        break;
      }
    }

    if (is_new_peer) {
      peers_with_tag.push_back(std::make_pair(node, tag));
    }
  }


  if (errors) {
    LOG(ERROR) << "Error occurs during construct Node structure.";
  } else {
    LOG(INFO) << "Dump content of all node in FL task before schedule:";
    uint32_t count = 0;

    for (auto &peer_with_tag : peers_with_tag) {
      auto &node = peer_with_tag.first;
      count++;
      LOG(INFO) << "Node content: node_id " << node.node_id() << ", role "
                << peer_with_tag.second << ", ip " << node.ip() << ", port "
                << node.port() << ", data port " << node.data_port() << ".";
    }

    LOG(INFO) << "Dump finish, dump count " << count << ".";
  }

  return;
}

void ProtocolSemanticParser::metasToPeerWithTagList(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    std::vector<NodeWithRoleTag> &peers_with_tag) {
  // reset peers
  peers_with_tag.clear();

  for (auto &meta_with_tag : metas_with_tag) {
    auto _meta = meta_with_tag.first;
    auto _tag = meta_with_tag.second;
    std::string dataset_path;
    Node node_info;
    std::string data_url = _meta->getDataURL();
    DataURLToDetail(data_url, node_info, dataset_path);

    rpc::Node node;
    node2PbNode(node_info, &node);
    bool is_new_peer = true;
    for (auto &peer : peers_with_tag) {
      if (peer.first.node_id() == node_info.id()) {
        is_new_peer = false;
        break;
      }
    }
    if (is_new_peer) {
      peers_with_tag.push_back(std::make_pair(node, _tag));
    }
  }
}
void ProtocolSemanticParser::prepareReply(rpc::PushTaskReply* reply) {
  for (const auto& node : this->task_server_) {
    auto server = reply->add_task_server();
    server->set_ip(node.ip_);
    server->set_port(node.port_);
    server->set_use_tls(node.use_tls_);
    server->set_role(node.role_);
  }
}

} // namespace primihub::task
