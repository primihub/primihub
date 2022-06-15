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

#include <memory>
#include <string>

#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/task/language/proto_parser.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/task/semantic/scheduler/aby3_scheduler.h"
#include "src/primihub/task/semantic/scheduler/fl_scheduler.h"
#include "src/primihub/task/semantic/scheduler/mpc_scheduler.h"
#include "src/primihub/task/semantic/scheduler/pir_scheduler.h"
#include "src/primihub/task/semantic/scheduler/psi_scheduler.h"

using primihub::service::DataURLToDetail;

namespace primihub::task {

/**
 * @brief Task syntx tree parser.
 *
 * 1. Get input task syntax tree from language parser
 * 2. Find all datasets in the task syntax tree using dataset service.
 * 3. Generate a scheduler object
 */
void ProtocolSemanticParser::parseTaskSyntaxTree(
    std::shared_ptr<LanguageParser> lan_parser) {
  if (lan_parser == nullptr) {
    return;
  }

  if (lan_parser->getPushTaskRequest().task().language() == Language::PROTO) {
    scheduleProtoTask(lan_parser);
  } else if (lan_parser->getPushTaskRequest().task().language() ==
             Language::PYTHON) {
    schedulePythonTask(lan_parser);
  } else {
    // TODO(primihub): implement other languages task parser
  }
}

void ProtocolSemanticParser::scheduleProtoTask(
    std::shared_ptr<LanguageParser> proto_parser) {
  auto _proto_parser = std::dynamic_pointer_cast<ProtoParser>(proto_parser);
  auto datasets_with_tag = _proto_parser->getDatasets();
  // Start find peer node by dataset list
  LOG(INFO) << " 🔍 Proto task finding meta list from datasets...";
  dataset_service_->metaService_->findPeerListFromDatasets(
      datasets_with_tag,
      [&](std::vector<DatasetMetaWithParamTag> &metas_with_param_tag) {
        LOG(INFO) << " 🔍 Proto task found meta list from datasets: "
                  << metas_with_param_tag.size();

        metasToPeerList(metas_with_param_tag, peer_list_);
        metasToPeerDatasetMap(metas_with_param_tag, peer_dataset_map_);

        //  Generate ABY3 scheduler
        auto pushTaskRequest = _proto_parser->getPushTaskRequest();
        if (pushTaskRequest.task().code() == "maxpool") {
          std::shared_ptr<VMScheduler> scheduler =
              std::make_shared<CRYPTFLOW2Scheduler>(
                  node_id_, peer_list_, peer_dataset_map_, singleton_);
          scheduler->dispatch(&pushTaskRequest);
        } else if (pushTaskRequest.task().code() == "lenet") {
          std::shared_ptr<VMScheduler> scheduler =
              std::make_shared<FalconScheduler>(node_id_, peer_list_,
                                                peer_dataset_map_, singleton_);
          scheduler->dispatch(&pushTaskRequest);
        } else {
          std::shared_ptr<VMScheduler> scheduler =
              std::make_shared<ABY3Scheduler>(node_id_, peer_list_,
                                              peer_dataset_map_, singleton_);
          scheduler->dispatch(&pushTaskRequest);
        }
      });
}

void ProtocolSemanticParser::schedulePythonTask(
    std::shared_ptr<LanguageParser> python_parser) {

    auto _python_parser = std::dynamic_pointer_cast<PyParser>(python_parser);
    auto datasets_with_tag = _python_parser->getDatasets(); // dataset with role tag
    peer_context_map_ = _python_parser->getNodeContextMap();

  // Start find peer node by dataset list
  LOG(INFO) << " 🔍 Python task finding meta list from datasets...";
  dataset_service_->metaService_->findPeerListFromDatasets(
      datasets_with_tag,
      [&](std::vector<DatasetMetaWithParamTag> &metas_with_param_tag) {
        LOG(INFO) << " 🔍 Python task found meta list from datasets: "
                  << metas_with_param_tag.size();

        metasToPeerWithTagList(metas_with_param_tag, peers_with_role_tag_);
        std::shared_ptr<VMScheduler> scheduler = std::make_shared<FLScheduler>(
                        node_id_, singleton_, 
                        peers_with_role_tag_, 
                        peer_context_map_, 
                        metas_with_param_tag);

        // Dispatch task to worker nodes
        auto pushTaskRequest = _python_parser->getPushTaskRequest();
        scheduler->dispatch(&pushTaskRequest);
      });
}

void ProtocolSemanticParser::schedulePirTask(
        std::shared_ptr<LanguageParser> lan_parser,
        std::string node_id,
        std::string node_ip,
        int node_port) {
    if (lan_parser == nullptr) {
        return;
    }

    if (lan_parser->getPushTaskRequest().task().language() ==
        Language::PROTO) {
        auto _proto_parser = std::dynamic_pointer_cast<ProtoParser>(lan_parser);
        auto datasets_with_tag = _proto_parser->getDatasets();
        LOG(INFO) << " 🔍 Finding meta list from datasets...";
        dataset_service_->metaService_->findPeerListFromDatasets(
            datasets_with_tag,
            [&](std::vector<DatasetMetaWithParamTag> &metas_with_param_tag) {
                LOG(INFO) << " 🔍 Found meta list from datasets: "
                          << metas_with_param_tag.size();
                metasToPeerList(metas_with_param_tag, peer_list_);

                Node client_node;
                client_node.set_node_id(node_id);
                client_node.set_ip(node_ip);
                client_node.set_port(node_port);
                peer_list_.push_back(client_node);

                metasToPeerDatasetMap(metas_with_param_tag, peer_dataset_map_);
                std::shared_ptr<VMScheduler> scheduler =
                    std::make_shared<PIRScheduler>(node_id_,
                                                   peer_list_,
                                                   peer_dataset_map_,
                                                   singleton_);
                auto pushTaskRequest = _proto_parser->getPushTaskRequest();
                scheduler->dispatch(&pushTaskRequest);
            });
    }
}

void ProtocolSemanticParser::schedulePsiTask(
        std::shared_ptr<LanguageParser> lan_parser){
    if (lan_parser == nullptr)
        return;

    if (lan_parser->getPushTaskRequest().task().language() ==
            Language::PROTO) {
        auto _proto_parser = std::dynamic_pointer_cast<ProtoParser>(lan_parser);
        auto datasets_with_tag = _proto_parser->getDatasets();
        // Start find peer node by dataset list
        LOG(INFO) << " �  Proto task finding meta list from datasets...";
        dataset_service_->metaService_->findPeerListFromDatasets(
            datasets_with_tag,
            [&](std::vector<DatasetMetaWithParamTag> &metas_with_param_tag) {
	        LOG(INFO) << " �  Proto task found meta list from datasets: "
                          << metas_with_param_tag.size();

                metasToPeerList(metas_with_param_tag, peer_list_);
                metasToPeerDatasetMap(metas_with_param_tag, peer_dataset_map_);
                std::shared_ptr<VMScheduler> scheduler =
                    std::make_shared<PSIScheduler>(node_id_,
                                                   peer_list_,
                                                   peer_dataset_map_,
                                                   singleton_);
                auto pushTaskRequest = _proto_parser->getPushTaskRequest();
                scheduler->dispatch(&pushTaskRequest);
	    });
    }
}

int ProtocolSemanticParser::transformPirRequest(std::shared_ptr<LanguageParser> lan_parser,
                                                PushTaskRequest &taskRequest) {
    if (lan_parser == nullptr) {
        LOG(ERROR) << "Language parser is null.";
        return -1;
    }
    LOG(INFO) << "test transformPirRequest.";

    if (lan_parser->getPushTaskRequest().task().language() ==
            Language::PROTO) {
        auto _proto_parser = std::dynamic_pointer_cast<ProtoParser>(lan_parser);
        taskRequest.CopyFrom(_proto_parser->getPushTaskRequest());
    } else {
        LOG(INFO) << "Pir task needs to implement this language: "
                  << lan_parser->getPushTaskRequest().task().language();
        return -1;
    }

    auto scheduler =
        std::make_shared<PIRScheduler>(node_id_, peer_list_,
                                       peer_dataset_map_, singleton_);

    int ret = scheduler->transformRequest(taskRequest);
    if (ret) {
        LOG(ERROR) << "Transform pir request failed.";
        return -1;
    }

    return 0;
}

void ProtocolSemanticParser::metasToPeerList(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    std::vector<Node> &peers) {
  // reset peers
  peers.clear();

  for (auto &meta_with_tag : metas_with_tag) {
    auto _meta = meta_with_tag.first;
    std::string node_id, node_ip, dataset_path;
    int node_port;
    std::string data_url = _meta->getDataURL();
    DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);

    Node node;
    node.set_node_id(node_id);
    node.set_ip(node_ip);
    node.set_port(node_port);
    bool is_new_peer = true;
    for (auto &peer : peers) {
      if (peer.node_id() == node_id) {
        is_new_peer = false;
        break;
      }
    }
    if (is_new_peer) {
      peers.push_back(node);
    }
  }
}

// output  key: node_id, value: <dataset_path, dataset_name>
void ProtocolSemanticParser::metasToPeerDatasetMap(
    const std::vector<DatasetMetaWithParamTag> &metas_with_param_tag,
    PeerDatasetMap &peer_dataset_map) {
  for (auto &meta : metas_with_param_tag) {
    auto _meta = meta.first;
    auto _param_tag = meta.second;
    DLOG(INFO) << "metasToPeerDatasetMap： " << _meta->getDataURL() << " "
               << _param_tag;
    std::string node_id, node_ip, dataset_path;
    int node_port;
    std::string data_url = _meta->getDataURL();
    DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);

    // find node_id in peer_dataset_map
    auto it = peer_dataset_map.find(node_id);
    if (it == peer_dataset_map.end()) {
      // create new peer
      std::vector<DatasetWithParamTag> datasets_with_tag;
      datasets_with_tag.push_back(std::make_pair(dataset_path, _param_tag));
      peer_dataset_map[node_id] = datasets_with_tag;
    } else {
      // add dataset to peer
      it->second.push_back(std::make_pair(dataset_path, _param_tag));
    }
  }
}

void ProtocolSemanticParser::metasToPeerWithTagList(
    const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
    std::vector<NodeWithRoleTag> &peers_with_tag) {
  // reset peers
  peers_with_tag.clear();

  for (auto &meta_with_tag : metas_with_tag) {
    auto _meta = meta_with_tag.first;
    auto _tag = meta_with_tag.second;
    std::string node_id, node_ip, dataset_path;
    int node_port;
    std::string data_url = _meta->getDataURL();
    DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);

    Node node;
    node.set_node_id(node_id);
    node.set_ip(node_ip);
    node.set_port(node_port);
    bool is_new_peer = true;
    for (auto &peer : peers_with_tag) {
      if (peer.first.node_id() == node_id) {
        is_new_peer = false;
        break;
      }
    }
    if (is_new_peer) {
      peers_with_tag.push_back(std::make_pair(node, _tag));
    }
  }
}

} // namespace primihub::task
