// "Copyright [2023] <PrimiHub>"
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

#include "src/primihub/common/common.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/node/nodelet.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/node/worker/worker.h"

namespace primihub {
class VMNodeImpl {
 public:
  explicit VMNodeImpl(const std::string& config_file,
                      std::shared_ptr<service::DatasetService> service);
  ~VMNodeImpl();
  retcode DispatchTask(const rpc::PushTaskRequest& task_request,
                       rpc::PushTaskReply* reply);
  retcode ExecuteTask(const rpc::PushTaskRequest& task_request,
                      rpc::PushTaskReply* reply);
  retcode KillTask(const rpc::KillTaskRequest& request,
                   rpc::KillTaskResponse* response);
  retcode GetSchedulerNodeCfg(const rpc::PushTaskRequest& request,
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

 protected:
  retcode Init();
  std::shared_ptr<Worker> CreateWorker();
  std::shared_ptr<Worker> CreateWorker(const std::string& worker_id);
  std::shared_ptr<Nodelet> GetNodelet() { return this->nodelet_;}

 private:
  const std::string node_id_;
  bool singleton_{false};
  // key; job_id+task_id value: tasker
  using task_executor_t =
      std::tuple<std::shared_ptr<Worker>, std::future<void>>;
  std::mutex task_executor_mtx_;
  std::map<std::string, task_executor_t> task_executor_map_;
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
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_NODE_IMPL_H_
