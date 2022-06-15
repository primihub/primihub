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

#ifndef SRC_PRIMIHUB_NODE_NODE_H_
#define SRC_PRIMIHUB_NODE_NODE_H_

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <pybind11/embed.h>  

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"

#include "src/primihub/common/config/config.h"
#include "src/primihub/node/nodelet.h"

#include "src/primihub/node/worker/worker.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/util.h"
#include "src/primihub/task/semantic/parser.h"


using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;
using primihub::rpc::Node;
using primihub::rpc::PirRequest;
using primihub::rpc::PirResponse;
using primihub::rpc::PsiRequest;
using primihub::rpc::PsiResponse;
using primihub::rpc::PushTaskReply;
using primihub::rpc::PushTaskRequest;

using primihub::rpc::VMNode;
using primihub::service::DatasetMetaWithParamTag;
using primihub::task::LanguageParser;

namespace py = pybind11;

namespace primihub {

class VMNodeImpl final: public VMNode::Service {
  public:
    explicit VMNodeImpl(const std::string &node_id_,
                        const std::string &node_ip_, int service_port_,
                        bool singleton_, const std::string &config_file_path_)
        : node_id(node_id_), node_ip(node_ip_), service_port(service_port_),
          singleton(singleton_), config_file_path(config_file_path_) {
        running_set.clear();
        nodelet = std::make_shared<Nodelet>(config_file_path);
    }

    Status SubmitTask(ServerContext *context,
                      const PushTaskRequest *pushTaskRequest,
                      PushTaskReply *pushTaskReply) override;
    
    Status ExecuteTask(ServerContext *context,
                       const ExecuteTaskRequest *taskRequest,
                       ExecuteTaskResponse *taskResponse) override;

    std::shared_ptr<Worker> CreateWorker();

    std::string get_node_address() {
        return absl::StrCat(this->node_id, this->node_ip, this->service_port);
    }

    std::string get_node_id() { return this->node_id; }

    std::shared_ptr<Nodelet> getNodelet() { return this->nodelet; }

  private:
    std::unordered_map<std::string, std::shared_ptr<Worker>>
        workers_ GUARDED_BY(worker_map_mutex_);

    mutable absl::Mutex worker_map_mutex_;
    mutable absl::Mutex parser_mutex_;
    const std::string node_id;
    const std::string node_ip;
    int service_port;

    // std::vector<Node> peer_list;
    // PeerDatasetMap peer_dataset_map;
    // std::shared_ptr<LanguageParser> lan_parser_;
    bool singleton;
    std::set<std::string> running_set;

    std::shared_ptr<Nodelet> nodelet;
    std::string config_file_path;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODE_H_
