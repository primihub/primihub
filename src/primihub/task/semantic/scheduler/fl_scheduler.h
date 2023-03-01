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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FL_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FL_SCHEDULER_H_

#include "src/primihub/task/semantic/scheduler.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/task/common.h"

using primihub::service::DatasetMetaWithParamTag;
using primihub::service::DatasetMeta;

namespace primihub::task {

class FLScheduler : public VMScheduler {
 public:
     FLScheduler(const std::string &node_id,
                    bool singleton,
                    const std::vector<NodeWithRoleTag> &peers_with_tag,
                    const PeerContextMap &peer_context_map,
                    const std::vector<DatasetMetaWithParamTag> &metas_with_role_tag)
               : VMScheduler(node_id, singleton),
               peers_with_tag_(peers_with_tag),
               peer_context_map_(peer_context_map),
               metas_with_role_tag_(metas_with_role_tag) {}
     ~FLScheduler() {}

     void dispatch(const PushTaskRequest *pushTaskRequest) override;
 protected:
     void push_node_py_task(const std::string& node_id,
                        const std::string& role,
                        const Node& dest_node,
                        const PushTaskRequest& nodePushTaskRequest,
                        const PeerContextMap& peer_context_map,
                        const std::vector<std::shared_ptr<DatasetMeta>>& dataset_meta_list);

 private:
     void add_vm(rpc::Node *node, int i, int role_num,
                    const PushTaskRequest *pushTaskRequest);

     void getDataMetaListByRole(const std::string &role,
                              std::vector<std::shared_ptr<DatasetMeta>> *data_meta_list);

     std::vector<NodeWithRoleTag> peers_with_tag_;
     PeerContextMap peer_context_map_;
     std::vector<DatasetMetaWithParamTag> metas_with_role_tag_;

};

} // namespace primihub::task


#endif // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FL_SCHEDULER_H_
