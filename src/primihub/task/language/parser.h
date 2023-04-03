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

#ifndef SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_
#define SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_

#include <string>

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"

using primihub::service::DatasetWithParamTag;

using primihub::rpc::PushTaskRequest;

namespace primihub::task {
// Primihub language layer
class LanguageParser {
 public:
    LanguageParser(const PushTaskRequest &pushTaskRequest) {
        pushTaskRequest_.CopyFrom(pushTaskRequest);
    }
    ~LanguageParser() {}

    virtual retcode parseTask() = 0;
    virtual retcode parseDatasets() = 0;
    virtual retcode parseNodes()  = 0;
    retcode MergePartyAccessInfo(const std::map<std::string, std::vector<Node>>& party_access_info) {
      std::map<std::string, std::vector<Node>> filtered_party;
      auto party_access_info_ptr = pushTaskRequest_.mutable_task()->mutable_party_access_info();
      // fetch all party from mutable_party_access_info
      for (const auto& [party_name, node_list] : *party_access_info_ptr) {
        for (const auto& pb_node : node_list.node()) {
          Node node;
          pbNode2Node(pb_node, &node);
          auto& party_list = filtered_party[party_name];
          auto iter = std::find(party_list.begin(), party_list.end(), node);
          if (iter == party_list.end()) {
            party_list.emplace_back(node);
          }
        }
      }
      // need_merged
      for (const auto& [party_name, node_list] : party_access_info) {
        for (const auto& node : node_list) {
          auto& party_list = filtered_party[party_name];
          auto iter = std::find(party_list.begin(), party_list.end(), node);
          if (iter == party_list.end()) {
            party_list.emplace_back(node);
          }
        }
      }
      // refill all
      party_access_info_ptr->clear();
      for (auto& [party_name, node_list] : filtered_party) {
        int32_t rank = 0;
        auto& party_node_list = (*party_access_info_ptr)[party_name];
        for (auto& node : node_list) {
          auto pb_node = party_node_list.add_node();
          node2PbNode(node, pb_node);
          pb_node->set_rank(rank);
          rank++;
        }
      }
    }

    PushTaskRequest& getPushTaskRequest() { return pushTaskRequest_; }
    std::vector<DatasetWithParamTag> getDatasets() { return input_datasets_with_tag_; }


 protected:
    PushTaskRequest pushTaskRequest_;
    std::vector<DatasetWithParamTag> input_datasets_with_tag_;
};

} // namespace primihub::task


#endif // SRC_PRIMIHUB_TASK_LANGUAGE_PARSER_H_
