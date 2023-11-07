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
#ifndef SRC_PRIMIHUB_SERVICE_DATASTREAM_DATASTREAM_SERVICE_H_
#define SRC_PRIMIHUB_SERVICE_DATASTREAM_DATASTREAM_SERVICE_H_
#include "src/primihub/node/node_impl.h"
#include "src/primihub/protos/metadatastream.grpc.pb.h"
namespace primihub::service {
using MetaDataSetDataStream = seatunnel::rpc::MetaDataSetDataStream;
class RecvDatasetSerivce : public seatunnel::rpc::MetaStreamService::Service {
 public:
  RecvDatasetSerivce(primihub::VMNodeImpl* node_service) :
      node_service_impl_ref_(node_service) {}
  grpc::Status DataSetDataStream(::grpc::ServerContext* context,
      grpc::ServerReader<MetaDataSetDataStream>* reader,
      seatunnel::rpc::Empty* response) override;
  grpc::Status FetchData(grpc::ServerContext* context,
      const MetaDataSetDataStream* request,
      grpc::ServerWriter<MetaDataSetDataStream>* writer) override;

 private:
  primihub::VMNodeImpl* node_service_impl_ref_{nullptr};
};
}
#endif  // SRC_PRIMIHUB_SERVICE_DATASTREAM_DATASTREAM_SERVICE_H_
