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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/scheduler.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/language/parser.h"
#include "src/primihub/task/language/py_parser.h"

using primihub::rpc::Task;
using primihub::rpc::Node;
using primihub::rpc::Language;
using primihub::service::DatasetService;
using primihub::service::DatasetWithParamTag;
using primihub::service::DatasetWithParamTag;
using primihub::service::DatasetMetaWithParamTag;
namespace primihub::task {

using PeerDatasetMap = std::map<std::string, std::vector<DatasetWithParamTag>>;
using NodeWithRoleTag = std::pair<Node, std::string>;
using PeerContextMap = std::map<std::string, NodeContext>; // key: role name

// Primihub semantic layer.
// This class is responsible for parsing the task and generating Scheduler
// object.
class ProtocolSemanticParser {
  public:
    ProtocolSemanticParser(const std::string &node_id,
                           bool singleton,
                           std::shared_ptr<DatasetService> dataset_service):
                           node_id_(node_id),
                           singleton_(singleton),
                           dataset_service_(dataset_service) {}

    ~ProtocolSemanticParser() {}
    void parseTaskSyntaxTree(std::shared_ptr<LanguageParser> lan_parser);
    void schedulePirTask(std::shared_ptr<LanguageParser> lan_parser,
                         std::string nodelet_attr);
    void schedulePsiTask(std::shared_ptr<LanguageParser> lan_parser);
    int transformPirRequest(std::shared_ptr<LanguageParser> lan_parser,
                            PushTaskRequest &taskRequest);

  private:
    void scheduleProtoTask(std::shared_ptr<LanguageParser> proto_parser);
    void schedulePythonTask( std::shared_ptr<LanguageParser> python_parser);
    void metasToPeerList(
        const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
        std::vector<Node> &peers);
    void metasToPeerDatasetMap(
          const std::vector<DatasetMetaWithParamTag> &metas_with_param_tag,
          PeerDatasetMap &peer_dataset_map);

    void metasToPeerWithTagList(
          const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
           std::vector<NodeWithRoleTag> &peers_with_tag);

    void metasToPeerWithTagAndPort(
        const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
        const PeerContextMap &peer_context_map,
        std::vector<NodeWithRoleTag> &peers_with_tag);
    
    const std::string node_id_;
    bool singleton_;
    std::shared_ptr<DatasetService> dataset_service_;

    // proto task use
    std::vector<Node> peer_list_;
    PeerDatasetMap peer_dataset_map_;

    // // python task use
    // std::vector<NodeWithRoleTag> peers_with_role_tag_;
    // PeerContextMap peer_context_map_;
   
};

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_
