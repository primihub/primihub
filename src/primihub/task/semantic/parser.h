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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/language/parser.h"
#include "src/primihub/task/language/py_parser.h"
#include "src/primihub/task/common.h"

namespace primihub::task {

// PrimiHub semantic layer.
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
  retcode parseTaskSyntaxTree(std::shared_ptr<LanguageParser> lan_parser);
  void prepareReply(rpc::PushTaskReply* reply);
  std::vector<Node>& taskServer() {return task_server_;}

 private:
  void parseTaskServer(const std::vector<Node>& task_servers);
  bool DatasetMetaSerivceEnabled();
  /**
   * brief request dataset meta service, get the location of the dataset
  */
  retcode ParseDatasetToPartyAccessInfo(LanguageParser* _parser);
  retcode ProcessAuxiliaryServer(LanguageParser* _parser);

  retcode scheduleProtoTask(std::shared_ptr<LanguageParser> proto_parser);
  retcode schedulePythonTask(std::shared_ptr<LanguageParser> python_parser);
  void metasToPeerList(
      const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
      std::vector<rpc::Node> &peers);
  void metasToPartyAccessInfo(
      const std::vector<DatasetMetaWithParamTag>& metas_with_tag,
      std::map<std::string, Node>* peers);
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

  void metasToDatasetAndOwner(
      const std::vector<DatasetMetaWithParamTag> &metas_with_tag,
      std::map<std::string, std::string> &dataset_owner);

 private:
  const std::string node_id_;
  bool singleton_;
  std::shared_ptr<DatasetService> dataset_service_;

  // proto task use
  std::vector<rpc::Node> peer_list_;
  PeerDatasetMap peer_dataset_map_;
  std::vector<Node> task_server_;
};

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PARSER_H_
