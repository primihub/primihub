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

#ifndef SRC_PRIMIHUB_ALGORITHM_BASE_H_
#define SRC_PRIMIHUB_ALGORITHM_BASE_H_

#include <string>
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/protos/common.grpc.pb.h"

using primihub::rpc::Task;
using primihub::service::DatasetService;

namespace primihub {
class AlgorithmBase {
  public:
    explicit AlgorithmBase(std::shared_ptr<DatasetService> dataset_service)
        : dataset_service_(dataset_service) {};
    virtual ~AlgorithmBase(){};

    virtual int loadParams(primihub::rpc::Task &task) = 0;
    virtual int loadDataset() = 0;
    virtual int initPartyComm() = 0;
    virtual int execute() = 0;
    virtual int finishPartyComm() = 0;
    virtual int saveModel() = 0;
    std::shared_ptr<DatasetService>& datasetService() {
      return dataset_service_;
    }

  protected:
    std::shared_ptr<DatasetService> dataset_service_;
    std::string algorithm_name_;
};
} // namespace primihub

#endif // SRC_PRIMIHUB_ALGORITHM_BASE_H_
