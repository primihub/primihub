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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
#include <glog/logging.h>
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/task_context.h"

#define CHECK_RETCODE_WITH_RETVALUE(ret_code, retvalue)  \
    do {                                                \
        if (ret_code != retcode::SUCCESS) {             \
            return retvalue;                            \
        }                                               \
    } while(0);

#define CHECK_RETCODE(ret_code)                 \
    do {                                        \
        if (ret_code != retcode::SUCCESS) {     \
            return retcode::FAIL;               \
        }                                       \
    } while(0);

#define CHECK_RETCODE_WITH_ERROR_MSG(ret_code, error_msg)   \
    do {                                                    \
        if (ret_code != retcode::SUCCESS) {                 \
            LOG(ERROR) << error_msg;                        \
            return retcode::FAIL;                           \
        }                                                   \
    } while(0);

#define CHECK_NULLPOINTER(ptr, ret_code)          \
    do {                                          \
        if (ptr == nullptr) {                     \
            return ret_code;                      \
        }                                         \
    } while(0);

#define CHECK_NULLPOINTER_WITH_ERROR_MSG(ptr, error_msg)    \
    do {                                                    \
        if (ret_code == nullptr) {                          \
            LOG(ERROR) << error_msg;                        \
            return retcode::FAIL;                           \
        }                                                   \
    } while(0);

#define CHECK_TASK_STOPPED(ret_data)                        \
    do {                                                    \
        if (this->has_stopped()) {                          \
            LOG(ERROR) << "task has been set stopped";      \
            return ret_data;                                \
        }                                                   \
    } while(0);

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
  // using task_context_t = TaskContext<primihub::rpc::TaskRequest, primihub::rpc::TaskResponse>;
  using task_context_t = TaskContext<std::string, std::string>;
  TaskBase(const TaskParam *task_param,
          std::shared_ptr<DatasetService> dataset_service);

  virtual ~TaskBase() = default;
  virtual int execute() = 0;
  virtual void kill_task() {
    LOG(WARNING) << "task receives kill task request and stop stauts";
    stop_.store(true);
    task_context_.clean();
  };
  bool has_stopped() {
    return stop_.load(std::memory_order_relaxed);
  }
  void setTaskInfo(const std::string& node_id, const std::string& job_id ,
      const std::string& task_id, const std::string& submit_client_id) {
    job_id_ = job_id;
    task_id_ = task_id;
    node_id_ = node_id;
    submit_client_id_ = submit_client_id;
    task_context_.setTaskInfo(job_id, task_id);
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
  inline std::string submit_client_id() {
    return submit_client_id_;
  }
  inline task_context_t& getTaskContext() {
    return task_context_;
  }
  inline task_context_t* getMutableTaskContext() {
    return &task_context_;
  }
  void setTaskParam(const TaskParam *task_param);
  TaskParam* getTaskParam();
  std::shared_ptr<DatasetService>& getDatasetService() {
    return dataset_service_;
  }
  retcode send(const std::string& key, const Node& dest_node,const std::string& send_buff);
  retcode send(const std::string& key, const Node& dest_node, std::string_view send_buff);
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
   std::string submit_client_id_;
};

} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TASK_H_
