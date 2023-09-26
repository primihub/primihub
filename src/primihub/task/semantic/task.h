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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#include <glog/logging.h>
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/task_context.h"
#include "src/primihub/common/value_check_util.h"

using primihub::rpc::Task;
using primihub::service::DatasetService;

namespace primihub::task {
using TaskParam = primihub::rpc::Task;

/**
 * @brief Basic task class
 *
 */
class TaskBase {
 public:
  using task_context_t = TaskContext;
  TaskBase() = default;
  TaskBase(const TaskParam* task_param,
          std::shared_ptr<DatasetService> dataset_service);

  virtual ~TaskBase() = default;
  virtual int execute() {return 0;};
  virtual void kill_task() {
    LOG(WARNING) << "task receives kill task request and stop stauts";
    stop_.store(true);
    task_context_.clean();
  };

  bool has_stopped() {
    return stop_.load(std::memory_order_relaxed);
  }

  void setTaskInfo(const std::string& node_id, const std::string& job_id ,
                   const std::string& task_id, const std::string& request_id,
                   const std::string& sub_task_id) {
    job_id_ = job_id;
    task_id_ = task_id;
    request_id_ = request_id;
    node_id_ = node_id;
    sub_task_id_ = sub_task_id;
    task_context_.setTaskInfo(job_id, task_id, request_id, sub_task_id);
  }

  inline std::string job_id() {
    return job_id_;
  }
  inline std::string task_id() {
    return task_id_;
  }
  inline std::string node_id() {
    return node_id_;
  }
  inline std::string request_id() {
    return request_id_;
  }
  inline std::string sub_task_id() {
    return sub_task_id_;
  }
  inline std::string party_name() {
    return party_name_;
  }
  inline task_context_t& getTaskContext() {
    return task_context_;
  }
  inline task_context_t* getMutableTaskContext() {
    return &task_context_;
  }
  void setTaskParam(const TaskParam& task_param);
  TaskParam* getTaskParam();
  std::shared_ptr<DatasetService>& getDatasetService() {
    return dataset_service_;
  }
  retcode ExtractProxyNode(const rpc::Task& task_config, Node* proxy_node);

  retcode send(const std::string& key,
               const Node& dest_node,
               const std::string& send_buff);
  retcode send(const std::string& key,
               const Node& dest_node,
               std::string_view send_buff);
  retcode recv(const std::string& key, std::string* recv_buff);
  retcode recv(const std::string& key, char* recv_buff, size_t length);
  retcode sendRecv(const std::string& key, const Node& dest_node,
      const std::string& send_buff, std::string* recv_buff);
  retcode sendRecv(const std::string& key, const Node& dest_node,
      std::string_view send_buff, std::string* recv_buff);
  /**
   * prepare data for sendRecv interface called by peer,
   * the server just prepare data and push into send queue
  */
  retcode pushDataToSendQueue(const std::string& key, std::string&& send_data);

 protected:
   std::atomic<bool> stop_{false};
   TaskParam task_param_;
   std::shared_ptr<DatasetService> dataset_service_;
   task_context_t task_context_;
   std::string job_id_;
   std::string task_id_;
   std::string node_id_;
   std::string sub_task_id_;
   std::string request_id_;
   std::string party_name_;
   bool is_dataset_detail_{false};
};

} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
