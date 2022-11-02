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


using primihub::rpc::Params;
using primihub::service::DatasetService;

namespace primihub::task {
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

protected:
    int loadDatasetFromCSV(std::string &filename, int data_col,
		           std::vector <std::string> &col_array,
			   int64_t max_num = 0);

    int loadDatasetFromTXT(std::string &filename,
		           std::vector <std::string> &col_array);

    Params params_;
    std::shared_ptr<DatasetService> dataset_service_;
};
} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_PRIVATE_SERVER_BASE_H_
