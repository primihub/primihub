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
#include "src/primihub/service/datastream/datastream_service.h"
#include "src/primihub/util/threadsafe_queue.h"
#include "src/primihub/util/log.h"
namespace primihub::service {
using MetaDataSetDataStream = seatunnel::rpc::MetaDataSetDataStream;
grpc::Status RecvDatasetSerivce::DataSetDataStream(
    grpc::ServerContext* context,
    grpc::ServerReader<MetaDataSetDataStream>* reader,
    seatunnel::rpc::Empty* response) {
  MetaDataSetDataStream request;
  bool recv_meta_info{false};
  primihub::ThreadSafeQueue<std::string>* recv_queue_ptr;
  std::string request_id;
  std::string data_set_id;
  size_t count{0};
  while (reader->Read(&request)) {
    if (!recv_meta_info) {
      data_set_id = request.data_set_id();
      request_id = request.trace_id();
      rpc::TaskContext task_info;
      task_info.set_request_id(request_id);
      auto worker_ptr = node_service_impl_ref_->GetWorker(task_info);
      if (worker_ptr == nullptr) {
        LOG(ERROR) << "get worker " << request_id <<  "failed";
      }
      auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
      if (link_ctx == nullptr) {
        std::string err_msg;
        err_msg.append(request_id)
              .append(" LinkContext is empty");
        LOG(ERROR) << err_msg;
        break;
      }
      recv_queue_ptr = &(link_ctx->GetRecvQueue(data_set_id));
      recv_meta_info = true;
    }
    std::string recv_data;
    request.SerializeToString(&recv_data);
    recv_queue_ptr->push(recv_data);
  }
  {
    MetaDataSetDataStream request;
    std::string recv_data;
    request.SerializeToString(&recv_data);
    recv_queue_ptr->push(recv_data);
  }
  LOG(INFO) << "end of recv data from seatunnel for: " << request_id;
  return grpc::Status::OK;
}

grpc::Status RecvDatasetSerivce::FetchData(grpc::ServerContext* context,
    const MetaDataSetDataStream* request,
    grpc::ServerWriter<MetaDataSetDataStream>* writer) {
  std::string request_id = request->trace_id();
  std::string data_set_id = request->data_set_id();
  rpc::TaskContext task_info;
  task_info.set_request_id(request_id);
  VLOG(5) << "begin to FetchData get worker: " << request_id
          << " dataset: " << data_set_id;
  auto worker_ptr = node_service_impl_ref_->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    LOG(ERROR) << "get worker for " << request_id << " failed";
    return grpc::Status::OK;
  }
  VLOG(5) << "begin to FetchData: " << request_id
          << " dataset: " << data_set_id;
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg;
    err_msg.append(request_id)
          .append(" LinkContext is empty");
    LOG(ERROR) << err_msg;
    return grpc::Status::OK;
  }
  size_t count = 0;
  auto& recv_queue = link_ctx->GetRecvQueue(data_set_id);
  do {
    std::string recv_data;
    recv_queue.wait_and_pop(recv_data);
    MetaDataSetDataStream result;
    result.ParseFromString(recv_data);
    if (result.data_set_id().empty()) {
      break;
    }
    writer->Write(result);
    count++;
  } while (true);
  VLOG(5) << "FetchData " << request_id << " "
          << "send package number: " << count;
  return grpc::Status::OK;
}

}  // namespace primihub::service
