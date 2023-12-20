/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_PRIMIHUB_NODE_NODE_IMPL_H_
#define SRC_PRIMIHUB_NODE_NODE_IMPL_H_
#include <memory>
#include <mutex>
#include <utility>
#include <map>
#include <unordered_map>
#include <set>
#include <future>
#include <atomic>
#include <tuple>
#include <string>
#include <queue>
#include <vector>

#include "src/primihub/common/common.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/node/nodelet.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/node/worker/worker.h"

namespace primihub {
enum class OperateTaskType {
  kUnknown = 0,
  kAdd,
  kDel,
  kKill,
};
class VMNodeImpl {
 public:
  using task_executor_t
      = std::tuple<std::shared_ptr<Worker>, std::future<void>>;
  using task_executor_container_t = std::queue<task_executor_t>;
  using task_manage_t =
      std::tuple<std::string, task_executor_t, OperateTaskType>;

  explicit VMNodeImpl(const std::string& config_file,
                      std::shared_ptr<service::DatasetService> service);
  ~VMNodeImpl();
  retcode DispatchTask(const rpc::PushTaskRequest& task_request,
                       rpc::PushTaskReply* reply);
  retcode ExecuteTask(const rpc::PushTaskRequest& task_request,
                      rpc::PushTaskReply* reply);
  retcode StopTask(const rpc::TaskContext& request);
  retcode KillTask(const rpc::KillTaskRequest& request,
                   rpc::KillTaskResponse* response);
  retcode GetSchedulerNode(const rpc::PushTaskRequest& request,
                           Node* scheduler_node);
  retcode FetchTaskStatus(const rpc::TaskContext& request,
                          rpc::TaskStatusReply* response);
  retcode NotifyTaskStatus(const rpc::PushTaskRequest& request,
                           const rpc::TaskStatus::StatusCode status,
                           const std::string& message);

  retcode NotifyTaskStatus(const Node& scheduler_node,
                           const rpc::TaskContext& task_info,
                           const std::string& party_name,
                           const rpc::TaskStatus::StatusCode status,
                           const std::string& message);

  retcode UpdateTaskStatus(const rpc::TaskStatus& task_status);

  std::shared_ptr<Worker> GetWorker(const rpc::TaskContext& task_info);

  std::shared_ptr<Worker> GetSchedulerWorker(const rpc::TaskContext& task_info);

  inline std::string GetWorkerId(const rpc::TaskContext& task_info) {
    return task_info.request_id();
  }

  bool IsTaskWorkerReady(const std::string& worker_id);
  /**
   * task status may not received by client
  */
  void CacheLastTaskStatus(const std::string& worker_id,
                           const rpc::TaskStatus::StatusCode status);
  /**
   * input: worker_id
   * output tuple<bool, std::string>
   * bool: worker_id related task finished or not
   * rpc::TaskStatus::StatusCode: task status
  */
  auto IsFinishedTask(const std::string& worker_id) ->
      std::tuple<bool, rpc::TaskStatus::StatusCode>;

  void CleanFinishedTaskThread();
  void CleanTimeoutCachedTaskStatusThread();
  void CleanFinishedSchedulerWorkerThread();
  void ManageTaskOperatorThread();
  void ProcessKillTaskThread();
  void ReportAliveInfoThread();

  std::shared_ptr<service::DatasetService> GetDatasetService() {
    return dataset_service_;
  }

  // data process related
  retcode ProcessReceivedData(const rpc::TaskContext& task_info,
                              const std::string& key,
                              std::string&& data_buffer);
  retcode ProcessSendData(const rpc::TaskContext& task_info,
                          const std::string& key,
                          std::string* data_buffer);
  retcode ProcessForwardData(const rpc::TaskContext& task_info,
                             const std::string& key,
                             std::string* data_buffer);
  retcode WaitUntilWorkerReady(const std::string& worker_id,
                               int timeout_ms = -1);
  std::shared_ptr<Nodelet> GetNodelet() { return this->nodelet_;}

 protected:
  retcode Init();
  std::shared_ptr<Worker> CreateWorker();
  std::shared_ptr<Worker> CreateWorker(const std::string& worker_id);
  std::shared_ptr<Worker> CreateWorker(const rpc::TaskContext& task_info);

  void CleanDuplicateTaskIdFilter();
  bool IsDuplicateTask(const rpc::TaskContext& task_info);
  retcode GetAllParties(const rpc::Task& task_config,
                        std::vector<Node>* all_party);
  retcode ExecuteAddTaskOperation(task_manage_t&& task_detail);
  retcode ExecuteDelTaskOperation(task_manage_t&& task_detail);
  retcode ExecuteKillTaskOperation(task_manage_t&& task_detail);
  std::string GenerateUUID();
  auto GetDeviceInfo() -> std::tuple<std::string, std::string>;

 private:
  std::string node_id_;
  int task_id_timeout_s_{30};
  std::unordered_map<std::string, time_t> duplicate_task_id_filter_{1000};
  std::mutex duplicate_task_filter_mtx_;
  bool singleton_{false};

  // key; job_id+task_id value: tasker

  std::shared_mutex task_executor_mtx_;
  std::map<std::string, task_executor_container_t> task_executor_map_;
  ThreadSafeQueue<std::string> fininished_workers_;
  std::future<void> finished_worker_fut_;
  std::atomic<bool> stop_{false};
  std::shared_ptr<Nodelet> nodelet_;
  std::string config_file_path_;
  int wait_worker_ready_timeout_ms_{WAIT_TASK_WORKER_READY_TIMEOUT_MS};
  std::shared_mutex finished_task_status_mtx_;
  // key: worker id
  // value: task_status, lastupdate timestamp
  using status_info_t = std::tuple<rpc::TaskStatus::StatusCode, time_t>;
  std::map<std::string, status_info_t> finished_task_status_;
  std::future<void> clean_cached_task_status_fut_;
  int cached_task_status_timeout_{CACHED_TASK_STATUS_TIMEOUT_S};
  std::unique_ptr<primihub::network::LinkContext> link_ctx_{nullptr};
  std::shared_mutex task_scheduler_mtx_;
  std::map<std::string, task_executor_t> task_scheduler_map_;
  ThreadSafeQueue<std::string> fininished_scheduler_workers_;
  std::future<void> finished_scheduler_worker_fut_;
  int scheduler_worker_timeout_s_{SCHEDULE_WORKER_TIMEOUT_S};
  std::shared_ptr<service::DatasetService> dataset_service_{nullptr};
  std::future<void> manage_task_worker_fut_;

  ThreadSafeQueue<task_manage_t> task_manage_queue_;
  std::atomic<bool> write_flag_{false};
  ThreadSafeQueue<task_executor_container_t> finished_task_queue_;
  ThreadSafeQueue<task_executor_container_t> kill_task_queue_;
  std::future<void> kill_task_queue_fut_;
  std::future<void> report_alive_info_fut_;
  bool disable_report_{false};
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_NODE_IMPL_H_
