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
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <memory>
#include <string>

#include "src/primihub/protos/service.grpc.pb.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/node/server_config.h"

namespace primihub {
class DataServiceImpl final: public rpc::DataSetService::Service {
 public:
  explicit DataServiceImpl(service::DatasetService* service) :
      dataset_service_ref_(service) {
    auto& ins = ServerConfig::getInstance();
    auto& server_info = ins.getServiceConfig();
    location_info_ = server_info.to_string();
  }

  grpc::Status NewDataset(grpc::ServerContext *context,
                          const rpc::NewDatasetRequest *request,
                          rpc::NewDatasetResponse *response) override;

 protected:
  std::string DatasetLocation() {
    return location_info_;
  }
  service::DatasetService* GetDatasetService() {
    return dataset_service_ref_;
  }
  retcode RegisterDatasetProcess(const DatasetMetaInfo& meta_info,
                                rpc::NewDatasetResponse* reply);
  retcode UnRegisterDatasetProcess(const DatasetMetaInfo& meta_info,
                                rpc::NewDatasetResponse* reply);
  retcode UpdateDatasetProcess(const DatasetMetaInfo& meta_info,
                              rpc::NewDatasetResponse* reply);
  template<typename T>
  void SetResponseErrorMsg(std::string&& err_msg, T* reply);

  template<typename T>
  void SetResponseErrorMsg(const std::string& err_msg, T* reply);

  retcode ConvertToDatasetMetaInfo(const rpc::MetaInfo& meta_info_pb,
                                  DatasetMetaInfo* meta_info);
 private:
  service::DatasetService* dataset_service_ref_{nullptr};
  std::string location_info_;
};

}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_DS_H_
