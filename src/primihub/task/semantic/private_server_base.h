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
#include <glog/logging.h>

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/util/log_wrapper.h"

using primihub::rpc::Params;
using primihub::service::DatasetService;

namespace primihub::task {
#define V_VLOG(level) \
  VLOG_WRAPPER(level, platform(), job_id(), task_id())

#define LOG_INFO() \
  LOG_INFO_WRAPPER(platform(), job_id(), task_id())

#define LOG_WARNING() \
  LOG_WARNING_WRAPPER(platform(), job_id(), task_id())

#define LOG_ERROR() \
  LOG_ERROR_WRAPPER(platform(), job_id(), task_id())

class ServerTaskBase {
public:
    ServerTaskBase(const Params *params,
                   std::shared_ptr<DatasetService> dataset_service);
    ~ServerTaskBase(){}

    virtual int execute() = 0;
    virtual int loadParams(Params & params) = 0;
    virtual int loadDataset(void) = 0;
    std::shared_ptr<DatasetService>& getDatasetService() {
        return dataset_service_;
    }
    void setTaskParam(const Params *params);
    Params* getTaskParam();
    std::string platform() {return primihub::typeToName(platform_type_);}
    inline std::string job_id() {return job_id_;}
    inline std::string task_id() {return task_id_;}
    inline void set_task_info(primihub::PlatFormType platform_type,
         const std::string& job_id, const std::string& task_id) {
      platform_type_ = platform_type;
      job_id_ = job_id;
      task_id_ = task_id;
    }

protected:
    int loadDatasetFromCSV(const std::string &filename, int data_col,
		           std::vector <std::string> &col_array, int64_t max_num = 0);
    int loadDatasetFromSQLite(const std::string& conn_str, int data_col,
		           std::vector<std::string>& col_array, int64_t max_num = 0);
    int loadDatasetFromTXT(std::string &filename,
		           std::vector <std::string> &col_array);

    Params params_;
    std::shared_ptr<DatasetService> dataset_service_;
    std::string job_id_;
    std::string task_id_;
    primihub::PlatFormType platform_type_;
};
} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PRIVATE_SERVER_BASE_H_
