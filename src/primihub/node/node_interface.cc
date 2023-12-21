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

#include "src/primihub/node/node_interface.h"
#include <glog/logging.h>
#include <algorithm>
#include <utility>

#include "src/primihub/util/util.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/util/log.h"

namespace primihub {
VMNodeInterface::VMNodeInterface(std::unique_ptr<VMNodeImpl> node_impl) :
    server_impl_(std::move(node_impl)) {}

VMNodeInterface::~VMNodeInterface() {
  server_impl_.reset();
}

Status VMNodeInterface::SubmitTask(ServerContext *context,
                                   const rpc::PushTaskRequest *pushTaskRequest,
                                   rpc::PushTaskReply *pushTaskReply) {
  if (VLOG_IS_ON(5)) {
    auto reqeust_str = proto::util::TaskRequestToString(*pushTaskRequest);
    LOG(INFO) << reqeust_str;
  }
  ServerImpl()->DispatchTask(*pushTaskRequest, pushTaskReply);

  auto task_info = pushTaskReply->mutable_task_info();
  task_info->CopyFrom(pushTaskRequest->task().task_info());

  return Status::OK;
}

Status VMNodeInterface::ExecuteTask(ServerContext* context,
                                    const rpc::PushTaskRequest* request,
                                    rpc::PushTaskReply* response) {
  const auto& task_info = request->task().task_info();
  std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "enter VMNodeImpl::ExecuteTask";
  auto ret = ServerImpl()->ExecuteTask(*request, response);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "ExecuteTask encountes error";
  }
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "exit VMNodeImpl::ExecuteTask";
  response->mutable_task_info()->CopyFrom(request->task().task_info());
  return Status::OK;
}
Status VMNodeInterface::StopTask(ServerContext* context,
                                 const rpc::TaskContext* request,
                                 rpc::Empty* response) {
  std::string TASK_INFO_STR = proto::util::TaskInfoToString(*request);
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "enter VMNodeImpl::StopTask";
  auto ret = ServerImpl()->StopTask(*request);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "execute StopTask method encountes error";
  }
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "exit VMNodeImpl::StopTask";
  return Status::OK;
}
Status VMNodeInterface::KillTask(ServerContext* context,
                                 const rpc::KillTaskRequest* request,
                                 rpc::KillTaskResponse* response) {
  auto ret = ServerImpl()->KillTask(*request, response);
  if (ret != retcode::SUCCESS) {
    const auto& task_info = request->task_info();
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "KillTask encountes error";
  }
  return Status::OK;
}

// task status operation
Status VMNodeInterface::FetchTaskStatus(ServerContext* context,
                                        const rpc::TaskContext* request,
                                        rpc::TaskStatusReply* response) {
  auto ret = ServerImpl()->FetchTaskStatus(*request, response);
  if (ret != retcode::SUCCESS) {
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(*request);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "FetchTaskStatus encountes error";
  }
  return Status::OK;
}

Status VMNodeInterface::UpdateTaskStatus(ServerContext* context,
                                         const rpc::TaskStatus* request,
                                         rpc::Empty* response) {
  auto ret = this->ServerImpl()->UpdateTaskStatus(*request);
  if (ret != retcode::SUCCESS) {
    const auto& task_info = request->task_info();
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "UpdateTaskStatus encountes error";
  }
  return Status::OK;
}

// communication interface
Status VMNodeInterface::Send(ServerContext* context,
                             ServerReader<rpc::TaskRequest>* reader,
                             rpc::TaskResponse* response) {
  bool recv_meta_info{false};
  std::vector<std::string> recv_data;
  rpc::TaskContext task_info;
  std::string key;
  std::string received_data;

  rpc::TaskRequest request;
  std::string TASK_INFO_STR;
  while (reader->Read(&request)) {
    if (!recv_meta_info) {
      task_info.CopyFrom(request.task_info());
      TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
      key = request.role();
      if (key.empty()) {
        PH_LOG(WARNING, LogType::kTask)
            << TASK_INFO_STR << "recv_key is not set";
      }
      size_t data_len = request.data_len();
      received_data.reserve(data_len);
      recv_meta_info = true;
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR << "recv key: " << key;
    }
    received_data.append(request.data());
  }
  size_t data_size = received_data.size();
  auto ret = this->ServerImpl()->ProcessReceivedData(task_info, key,
                                                     std::move(received_data));
  if (ret != retcode::SUCCESS) {
    response->set_ret_code(rpc::retcode::FAIL);
  }
  PH_VLOG(5, LogType::kTask)
      << TASK_INFO_STR
      << "end of VMNodeImpl::Send, data total received size:" << data_size;
  response->set_ret_code(rpc::retcode::SUCCESS);
  return Status::OK;
}

Status VMNodeInterface::Recv(ServerContext* context,
                             const rpc::TaskRequest* request,
                             ServerWriter<rpc::TaskResponse>* writer) {
  std::string send_data;
  const auto& task_info = request->task_info();
  std::string key = request->role();
  auto ret = this->ServerImpl()->ProcessSendData(task_info, key, &send_data);
  if (ret != retcode::SUCCESS) {
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "no data is available for key: " << key;
    rpc::TaskResponse response;
    std::string err_msg = "no data is available for key:" + key;
    response.set_ret_code(rpc::retcode::FAIL);
    response.set_msg_info(std::move(err_msg));
    writer->Write(response);
    return grpc::Status::OK;
  }
  std::vector<std::unique_ptr<rpc::TaskResponse>> response_data;
  BuildTaskResponse(send_data, &response_data);
  for (const auto& res : response_data) {
    writer->Write(*res);
  }
  return grpc::Status::OK;
}

Status VMNodeInterface::SendRecv(ServerContext* context,
    ServerReaderWriter<rpc::TaskResponse, rpc::TaskRequest>* stream) {
  bool recv_meta_info{false};
  std::vector<std::string> recv_data;
  rpc::TaskContext task_info;
  std::string key;
  std::string received_data;
  rpc::TaskRequest request;
  std::string TASK_INFO_STR;
  while (stream->Read(&request)) {
    if (!recv_meta_info) {
      task_info.CopyFrom(request.task_info());
      TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
      key = request.role();
      if (key.empty()) {
        PH_LOG(WARNING, LogType::kTask)
            << TASK_INFO_STR << "send key is empty";
      }
      size_t data_len = request.data_len();
      received_data.reserve(data_len);
      recv_meta_info = true;
      VLOG(5) << TASK_INFO_STR << "send key: " << key;
    }
    received_data.append(request.data());
  }
  size_t data_size = received_data.size();
  auto ret = this->ServerImpl()->ProcessReceivedData(task_info, key,
                                                     std::move(received_data));
  if (ret != retcode::SUCCESS) {
    rpc::TaskResponse response;
    std::string err_msg = "ProcessReceivedData encountes error";
    response.set_ret_code(rpc::retcode::FAIL);
    response.set_msg_info(std::move(err_msg));
    stream->Write(response);
    return Status::OK;
  }
  // process send data
  std::string send_data;
  ret = this->ServerImpl()->ProcessSendData(task_info, key, &send_data);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "no data is available for key: " << key;
    rpc::TaskResponse response;
    std::string err_msg = "no data is available for key:" + key;
    response.set_ret_code(rpc::retcode::FAIL);
    response.set_msg_info(std::move(err_msg));
    stream->Write(response);
    return Status::OK;
  }
  std::vector<std::unique_ptr<rpc::TaskResponse>> response_data;
  BuildTaskResponse(send_data, &response_data);
  for (const auto& res : response_data) {
    stream->Write(*res);
  }
  return Status::OK;
}

// for communication between different process
Status VMNodeInterface::ForwardSend(ServerContext* context,
    ServerReader<rpc::ForwardTaskRequest>* reader,
    rpc::TaskResponse* response) {
  // // send data to destination node specified by request
  // VLOG(5) << "enter VMNodeImpl::ForwardSend";
  // primihub::rpc::ForwardTaskRequest forward_request;
  // std::unique_ptr<VMNode::Stub> stub{nullptr};
  // grpc::ClientContext client_context;
  // primihub::rpc::TaskResponse task_response;
  // using client_writer_t = grpc::ClientWriter<primihub::rpc::TaskRequest>;
  // std::unique_ptr<client_writer_t> writer{nullptr};
  // bool is_initialized{false};
  // bool write_success{false};
  // std::string dest_address;
  // while (reader->Read(&forward_request)) {
  //   if (!is_initialized) {
  //     const auto& dest_node = forward_request.dest_node();
  //     const auto& task_request = forward_request.task_request();
  //     dest_address = dest_node.ip() + ":" + std::to_string(dest_node.port());
  //     bool use_tls = false;
  //     stub = get_stub(dest_address, use_tls);
  //     writer = stub->Send(&client_context, &task_response);
  //     is_initialized = true;
  //     write_success = writer->Write(task_request);
  //     if (!write_success) {
  //       LOG(ERROR) << "forward data to " << dest_address << "failed";
  //       break;
  //     }
  //   } else {
  //     const auto& task_request = forward_request.task_request();
  //     write_success = writer->Write(task_request);
  //     if (!write_success) {
  //       LOG(ERROR) << "forward data to " << dest_address << "failed";
  //       break;
  //     }
  //   }
  // }
  // writer->WritesDone();
  // Status status = writer->Finish();
  // if (status.ok()) {
  //   auto ret_code = task_response.ret_code();
  //   if (ret_code) {
  //       LOG(ERROR) << "client Node send result data to server: "
  //                  << dest_address
  //                  << " return failed error code: " << ret_code;
  //   }
  //   response->set_ret_code(rpc::retcode::SUCCESS);
  //   response->set_msg_info(task_response.msg_info());
  // } else {
  //   LOG(ERROR) << "client Node send result data to server "  << dest_address
  //           << " failed. error_code: "
  //           << status.error_code() << ": " << status.error_message();
  //   response->set_ret_code(rpc::retcode::FAIL);
  //   response->set_msg_info(status.error_message());
  // }
  return Status::OK;
}

// for communication between different process
Status VMNodeInterface::ForwardRecv(ServerContext* context,
                                    const rpc::TaskRequest* request,
                                    ServerWriter<rpc::TaskRequest>* writer) {
  // waiting for peer node send data
  const auto& task_info = request->task_info();
  std::string key = request->role();
  std::string recv_data;
  auto ret = this->ServerImpl()->ProcessForwardData(task_info, key, &recv_data);
  if (ret != retcode::SUCCESS) {
    rpc::TaskRequest response;
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    std::string err_msg = "no data is available for key:" + key;
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << err_msg;
    writer->Write(response);
    return Status::OK;
  }
  std::vector<std::unique_ptr<rpc::TaskRequest>> forward_recv_datas;
  this->BuildTaskRequest(task_info, key, recv_data, &forward_recv_datas);
  for (const auto& data_ : forward_recv_datas) {
    writer->Write(*data_);
  }
  return grpc::Status::OK;
}

retcode VMNodeInterface::WaitUntilWorkerReady(const std::string& worker_id,
                                              grpc::ServerContext* context,
                                              int timeout_ms) {
  std::string TASK_INFO_STR = proto::util::TaskInfoToString(worker_id);
  if (context->IsCancelled()) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "context is cancelled by client";
    return retcode::FAIL;
  }
  auto _start = std::chrono::high_resolution_clock::now();
  SCopedTimer timer;
  do {
    // try to get lock
    if (ServerImpl()->IsTaskWorkerReady(worker_id)) {
      break;
    }
    if (context->IsCancelled()) {
      PH_LOG(ERROR, LogType::kTask)
         << TASK_INFO_STR << "context is cancelled by client";
      return retcode::FAIL;
    }
    PH_VLOG(5, LogType::kTask)
        << TASK_INFO_STR
        << "sleep and wait for worker ready ......";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (timeout_ms == -1) {
      continue;
    }
    auto time_elapse_ms = timer.timeElapse();
    if (time_elapse_ms > timeout_ms) {
      PH_LOG(ERROR, LogType::kTask)
          << TASK_INFO_STR
          << "wait for worker ready is timeout";
      return retcode::FAIL;
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode VMNodeInterface::BuildTaskResponse(const std::string& data,
    std::vector<std::unique_ptr<rpc::TaskResponse>>* response) {
  size_t sended_size = 0;
  size_t max_package_size = LIMITED_PACKAGE_SIZE;
  size_t total_length = data.size();
  size_t split_number = total_length / max_package_size;
  if (total_length % max_package_size) {
    split_number++;
  }
  response->reserve(split_number);
  char* send_buf = const_cast<char*>(data.c_str());
  do {
    auto task_response = std::make_unique<rpc::TaskResponse>();
    task_response->set_ret_code(rpc::retcode::SUCCESS);
    task_response->set_data_len(total_length);
    auto data_ptr = task_response->mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    response->emplace_back(std::move(task_response));
    if (sended_size < total_length) {
      continue;
    } else {
      break;
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode VMNodeInterface::BuildTaskRequest(const rpc::TaskContext& task_info,
    const std::string& key,
    const std::string& data,
    std::vector<std::unique_ptr<rpc::TaskRequest>>* requests) {
  size_t sended_size = 0;
  size_t max_package_size = LIMITED_PACKAGE_SIZE;
  size_t total_length = data.size();
  size_t split_num = total_length / max_package_size;
  if (total_length % max_package_size) {
    split_num++;
  }
  requests->reserve(split_num);
  char* send_buf = const_cast<char*>(data.c_str());
  do {
    // rpc::TaskRequest task_request;
    auto task_request = std::make_unique<rpc::TaskRequest>();
    auto task_info_ptr = task_request->mutable_task_info();
    task_info_ptr->CopyFrom(task_info);
    task_request->set_role(key);
    task_request->set_data_len(total_length);
    auto data_ptr = task_request->mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    requests->emplace_back(std::move(task_request));
    if (sended_size < total_length) {
      continue;
    } else {
      break;
    }
  } while (true);
  return retcode::SUCCESS;
}

}  // namespace primihub
