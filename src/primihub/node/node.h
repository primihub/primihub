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
#include <future>

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
#include "src/primihub/util/threadsafe_queue.h"

// using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using primihub::rpc::TaskRequest;
using primihub::rpc::TaskResponse;
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
        task_executor_map.clear();
        nodelet = std::make_shared<Nodelet>(config_file_path);
        finished_worker_fut = std::async(std::launch::async,
          [&]() {
            while(true) {
              std::string finished_worker_id;
              fininished_workers.wait_and_pop(finished_worker_id);
              if (stop_.load(std::memory_order::memory_order_relaxed)) {
                break;
              }
              {
                std::lock_guard<std::mutex> lck(this->task_executor_mtx);
                auto it = task_executor_map.find(finished_worker_id);
                if (it != task_executor_map.end()) {
                  VLOG(5) << "worker id : " << finished_worker_id << " has finished, begin to erase";
                  task_executor_map.erase(finished_worker_id);
                  VLOG(5) << "erase worker id : " << finished_worker_id << " success";
                }
              }
            }
          });
    }
    ~VMNodeImpl() override {
      stop_.store(true);
      fininished_workers.shutdown();
      finished_worker_fut.get();
      this->nodelet.reset();
    }

    Status SubmitTask(ServerContext *context,
                      const PushTaskRequest *pushTaskRequest,
                      PushTaskReply *pushTaskReply) override;
    Status ExecuteTask(ServerContext* context,
        grpc::ServerReaderWriter<ExecuteTaskResponse, ExecuteTaskRequest>* stream) override;

    Status Send(ServerContext* context,
        ServerReader<TaskRequest>* reader, TaskResponse* response) override;

    Status Recv(::grpc::ServerContext* context,
        const TaskRequest* request,
        grpc::ServerWriter< ::primihub::rpc::TaskResponse>* writer) override;

    // for communication between different process
    Status ForwardSend(::grpc::ServerContext* context,
        ::grpc::ServerReader< ::primihub::rpc::ForwardTaskReqeust>* reader,
        ::primihub::rpc::TaskResponse* response) override;

    // for communication between different process
    Status ForwardRecv(::grpc::ServerContext* context,
        const ::primihub::rpc::ForwardTaskReqeust* request,
        ::grpc::ServerWriter< ::primihub::rpc::TaskResponse>* writer) override;

    std::shared_ptr<Worker> CreateWorker();
    std::shared_ptr<Worker> CreateWorker(const std::string& worker_id);

    std::string get_node_address() {
        return absl::StrCat(this->node_id, this->node_ip, this->service_port);
    }

    std::string get_node_id() { return this->node_id; }

    std::shared_ptr<Nodelet> getNodelet() { return this->nodelet; }
 protected:
    std::unique_ptr<VMNode::Stub> get_stub(const std::string& dest_address, bool use_tls);
    int process_task_reseponse(bool is_psi_response,
          const ExecuteTaskResponse& response,
          std::vector<ExecuteTaskResponse>* splited_responses);
    int process_psi_response(const ExecuteTaskResponse& response,
          std::vector<ExecuteTaskResponse>* splited_responses);
    int process_pir_response(const ExecuteTaskResponse& response,
          std::vector<ExecuteTaskResponse>* splited_responses);
    int save_data_to_file(const std::string& data_path, std::vector<std::string>&& save_data);
    int validate_file_path(const std::string& data_path) { return 0;}
    int ClearWorker(const std::string& worker_id);
    bool IsPSIECDHServer(const PushTaskRequest& request);  // for temp check
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
    // key; job_id+task_id value: tasker
    using task_executor_t = std::tuple<std::shared_ptr<Worker>, std::future<void>>;
    std::mutex task_executor_mtx;
    std::map<std::string, task_executor_t> task_executor_map;
    ThreadSafeQueue<std::string> fininished_workers;
    std::future<void> finished_worker_fut;
    std::atomic<bool> stop_{false};

    std::shared_ptr<Nodelet> nodelet;
    std::string config_file_path;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODE_H_
