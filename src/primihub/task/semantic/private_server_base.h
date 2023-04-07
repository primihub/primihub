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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PRIVATE_SERVER_BASE_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PRIVATE_SERVER_BASE_H_

#include <map>
#include <memory>
#include <string>
#include <atomic>
#include <glog/logging.h>

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/semantic/task.h"

using primihub::rpc::Params;
using primihub::service::DatasetService;

namespace primihub::task {
class ServerTaskBase {
 public:
    using task_context_t = TaskContext<primihub::rpc::ExecuteTaskRequest, primihub::rpc::ExecuteTaskResponse>;
    ServerTaskBase(const Params *params,
                   std::shared_ptr<DatasetService> dataset_service);
    ~ServerTaskBase(){}

    virtual int execute() = 0;
    virtual int loadParams(Params & params) = 0;
    virtual int loadDataset(void) = 0;
    virtual void kill_task() {
        LOG(WARNING) << "task receives kill task request and stop stauts";
        stop_.store(true);
        task_context_.clean();
    }
    bool has_stopped() {
        return stop_.load(std::memory_order_relaxed);
    }
    std::shared_ptr<DatasetService>& getDatasetService() {
        return dataset_service_;
    }
    void setTaskParam(const Params *params);
    Params* getTaskParam();
    inline task_context_t& getTaskContext() {
        return task_context_;
    }
    inline task_context_t* getMutableTaskContext() {
        return &task_context_;
    }

protected:
    int loadDatasetFromCSV(const std::string &filename, int data_col,
		           std::vector <std::string> &col_array, int64_t max_num = 0);
    int loadDatasetFromSQLite(const std::string& conn_str, int data_col,
		           std::vector<std::string>& col_array, int64_t max_num = 0);
    int loadDatasetFromTXT(std::string &filename,
		           std::vector <std::string> &col_array);
    std::atomic<bool> stop_{false};
    Params params_;
    std::shared_ptr<DatasetService> dataset_service_;
    task_context_t task_context_;
};
} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PRIVATE_SERVER_BASE_H_
