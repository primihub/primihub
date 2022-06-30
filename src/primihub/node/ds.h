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

#ifndef SRC_PRIMIHUB_NODE_DS_H_
#define SRC_PRIMIHUB_NODE_DS_H_

#include <glog/logging.h>
#include <memory>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "src/primihub/protos/service.grpc.pb.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/service/dataset/service.h"

using grpc::ServerContext;
using primihub::rpc::DataService;
using primihub::rpc::NewDatasetRequest;
using primihub::rpc::NewDatasetResponse;
using primihub::service::DatasetService;

namespace primihub {

class DataServiceImpl final: public DataService::Service {
    public:
        explicit DataServiceImpl(std::shared_ptr<primihub::service::DatasetService> dataset_service,
                                 std::string nodelet_addr)
        : dataset_service_(dataset_service), nodelet_addr_(nodelet_addr) {
            
        }
        
        grpc::Status NewDataset(grpc::ServerContext *context, const NewDatasetRequest *request,
                                NewDatasetResponse *response) override;

    private:
        std::shared_ptr<primihub::service::DatasetService> dataset_service_;
        std::string nodelet_addr_;
};

} // namespace primihub
#endif // SRC_PRIMIHUB_NODE_DS_H_
