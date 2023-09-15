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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_TEE_SCHEDULER_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_TEE_SCHEDULER_H_

#include <string>
#include <vector>
#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/util/util.h"
#include "src/primihub/common/common.h"


using primihub::service::DatasetMeta;
using primihub::service::DatasetMetaWithParamTag;
using primihub::rpc::Params;

namespace primihub::task {
/**
 * @brief The TeeScheduler class
 *     TEE scheduler is a SGX2.0 scheduler. Support role:
 *        1. Executor: node that can execute a task in SGX2.0 enclave.
 *        2. DataProvider:  node that can provide data to a task.
 * @todo
 */
class TEEScheduler : public VMScheduler {
  public:
    TEEScheduler() = default;
    TEEScheduler(const std::string &node_id,
                 std::vector<rpc::Node> &peer_list,
                 const PeerDatasetMap &peer_dataset_map,
                 const Params params,
                 bool singleton)
        : VMScheduler(node_id, singleton), peer_list_(peer_list),
          peer_dataset_map_(peer_dataset_map) {

        rpc::Node node;
        node.set_node_id("TEE_Executor");
        auto param_map = params.param_map();
        try {
            std::string server_addr = param_map["server"].value_string();
            // split ip:port
            std::vector<std::string> v;
            str_split(server_addr, &v);
            node.set_ip(v[0]);
            node.set_port(std::stoi(v[1]));
            // Add server node to peer_list_ head.
            peer_list_.insert(peer_list_.begin(), node);
        } catch (std::exception &e) {
            LOG(ERROR) << "get TEE server addr error: " << e.what();
        }

    }

    ~TEEScheduler() {}

    retcode dispatch(const PushTaskRequest *pushTaskRequest) override;

  private:
    void add_vm(rpc::Node *executor, rpc::Node *dpv, int party_id,
                const PushTaskRequest *pushTaskRequest);
    void push_task_to_node(const std::string &node_id,
                           const PeerDatasetMap &peer_dataset_map,
                           const PushTaskRequest &request,
                           const Node& dest_node_address);

    std::vector<rpc::Node> peer_list_;
    const PeerDatasetMap peer_dataset_map_;

}; // class TEEScheduler

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_TEE_SCHEDULER_H_
