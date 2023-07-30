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
#include <ctime>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <future>

#include "absl/strings/str_cat.h"
#include "src/primihub/common/config/config.h"
// #include "src/primihub/node/nodelet.h"
#include "src/primihub/node/worker/worker.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
// #include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/util.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/service.h"

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
    // VMNodeImpl(const std::string &node_id_,
    //           const std::string &node_ip_, int service_port_,
    //           bool singleton_, const std::string &config_file_path_);

    VMNodeImpl(const std::string& node_id_,
              const std::string& config_file_path_,
              std::shared_ptr<service::DatasetService> service);
    ~VMNodeImpl();

    Status SubmitTask(ServerContext *context,
                      const PushTaskRequest *pushTaskRequest,
                      PushTaskReply *pushTaskReply) override;
    Status ExecuteTask(::grpc::ServerContext* context,
                      const rpc::PushTaskRequest* request,
                      rpc::PushTaskReply* response) override;

    Status KillTask(::grpc::ServerContext* context,
                    const ::primihub::rpc::KillTaskRequest* request,
                    ::primihub::rpc::KillTaskResponse* response) override;
    // task status operation
    Status FetchTaskStatus(::grpc::ServerContext* context,
                          const ::primihub::rpc::TaskContext* request,
                          ::primihub::rpc::TaskStatusReply* response) override;
    Status UpdateTaskStatus(::grpc::ServerContext* context,
                            const ::primihub::rpc::TaskStatus* request,
                            ::primihub::rpc::Empty* response);

    Status Send(ServerContext* context,
                ServerReader<TaskRequest>* reader,
                TaskResponse* response) override;
    Status Recv(::grpc::ServerContext* context,
                const TaskRequest* request,
                grpc::ServerWriter<::primihub::rpc::TaskResponse>* writer) override;
    Status SendRecv(::grpc::ServerContext* context,
                    grpc::ServerReaderWriter<::primihub::rpc::TaskResponse,
                    ::primihub::rpc::TaskRequest>* stream) override;

    // for communication between different process
    Status ForwardSend(::grpc::ServerContext* context,
                      ::grpc::ServerReader<::primihub::rpc::ForwardTaskRequest>* reader,
                      ::primihub::rpc::TaskResponse* response) override;

    // for communication between different process
    Status ForwardRecv(::grpc::ServerContext* context,
                      const ::primihub::rpc::TaskRequest* request,
                      ::grpc::ServerWriter<::primihub::rpc::TaskRequest>* writer) override;

    std::shared_ptr<Worker> CreateWorker();
    std::shared_ptr<Worker> CreateWorker(const std::string& worker_id);

    std::string get_node_address() {
        return absl::StrCat(this->node_id, this->node_ip, this->service_port);
    }

    std::string get_node_id() { return this->node_id; }

    std::shared_ptr<Nodelet> getNodelet() { return this->nodelet;}

 protected:
  std::unique_ptr<VMNode::Stub> get_stub(const std::string& dest_address, bool use_tls);
    retcode DispatchTask(const PushTaskRequest& task_request, PushTaskReply* reply);
    retcode ExecuteTask(const PushTaskRequest& task_request, PushTaskReply* reply);
    retcode getSchedulerNodeCfg(const PushTaskRequest& request, Node* scheduler_node);
    retcode notifyTaskStatus(const PushTaskRequest& request,
        const rpc::TaskStatus::StatusCode& status, const std::string& message);
    retcode notifyTaskStatus(const Node& scheduler_node,
        const std::string& task_id, const std::string& job_id,
        const std::string& request_id, const std::string& role,
        const rpc::TaskStatus::StatusCode& status, const std::string& message);
    retcode updateTaskStatusInternal(const rpc::TaskStatus& task_status);
    void buildTaskResponse(const std::string& data, std::vector<rpc::TaskResponse>* response);
    void buildTaskRequest(const std::string& job_id, const std::string& task_id,
        const std::string& request_id, const std::string& role,
        const std::string& data, std::vector<rpc::TaskRequest>* requests);

    std::shared_ptr<Worker> getWorker(const std::string& job_id,
                                      const std::string& task_id,
                                      const std::string& request_id);

    std::shared_ptr<Worker> getSchedulerWorker(const std::string& job_id,
                                              const std::string& task_id,
                                              const std::string& request_id);

    inline std::string getWorkerId(const std::string& job_id,
                                  const std::string& task_id,
                                  const std::string& request_id) {
      // return job_id + "_" + task_id;
      return request_id;
    }

    inline std::string getWorkerId(const std::string& request_id) {
      return request_id;
    }

    retcode waitUntilWorkerReady(const std::string& worker_id, grpc::ServerContext* context, int timeout = -1);
    /**
     * task status may not received by client
    */
    void cacheLastTaskStatus(const std::string& worker_id,
        const std::string& submited_client_id, const rpc::TaskStatus::StatusCode& status);
    /**
     * input: worker_id
     * output tuple<bool, std::string>
     * bool: worker_id related task finished or not
     * std::string: who submited this task
     * rpc::TaskStatus::StatusCode: task status
    */
    std::tuple<bool, std::string, std::string> isFinishedTask(const std::string& worker_id);

  void CleanFinishedTaskThread();
  void CleanTimeoutCachedTaskStatusThread();
  void CleanFinishedSchedulerWorkerThread();

  std::shared_ptr<service::DatasetService> GetDatasetService() {
    return dataset_service_;
  }

 private:
  retcode Init();
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
    bool singleton{false};
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
    std::unique_ptr<VMNode::Stub> stub_;
    int wait_worker_ready_timeout_ms{WAIT_TASK_WORKER_READY_TIMEOUT_MS};
    std::shared_mutex finished_task_status_mtx_;
    // key: worker id
    // value: submited_client_id, task_status, lastupdate timestamp
    std::map<std::string, std::tuple<std::string, std::string, time_t>> finished_task_status_;
    std::future<void> clean_cached_task_status_fut_;
    int cached_task_status_timeout_{CACHED_TASK_STATUS_TIMEOUT_S};

    std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};

    std::shared_mutex task_scheduler_mtx_;
    std::map<std::string, task_executor_t> task_scheduler_map_;
    ThreadSafeQueue<std::string> fininished_scheduler_workers_;
    std::future<void> finished_scheduler_worker_fut;
    int scheduler_worker_timeout_s{SCHEDULE_WORKER_TIMEOUT_S};
    std::shared_ptr<service::DatasetService> dataset_service_{nullptr};
};

} // namespace primihub

#endif // SRC_PRIMIHUB_NODE_NODE_H_
