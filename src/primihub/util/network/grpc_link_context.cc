// Copyright [2022] <primihub.com>
#include "src/primihub/util/network/grpc_link_context.h"
#include <glog/logging.h>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>

#include "src/primihub/util/util.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/util/arrow_wrapper_util.h"

namespace pb_util = primihub::proto::util;
namespace arrow_util = primihub::arrow_wrapper::util;
namespace primihub::network {
GrpcChannel::GrpcChannel(const primihub::Node& node, LinkContext* link_ctx) :
    IChannel(link_ctx) {
  dest_node_ = node;
  std::string address_ = node.ip_ + ":" + std::to_string(node.port_);
  auto channel = buildChannel(address_, node.use_tls_);
  stub_ = rpc::VMNode::NewStub(channel);
  dataset_stub_ = rpc::DataSetService::NewStub(channel);
  seatunnel_stub_ = seatunnel::rpc::MetaStreamService::NewStub(channel);
}

retcode GrpcChannel::BuildTaskInfo(rpc::TaskContext* task_info) {
  auto link_ctx = this->getLinkContext();
  if (link_ctx == nullptr) {
    return retcode::FAIL;
  }
  task_info->set_job_id(link_ctx->job_id());
  task_info->set_task_id(link_ctx->task_id());
  task_info->set_sub_task_id(link_ctx->sub_task_id());
  task_info->set_request_id(link_ctx->request_id());
  return retcode::SUCCESS;
}

std::shared_ptr<grpc::Channel> GrpcChannel::buildChannel(
    std::string& server_address,
    bool use_tls) {
  std::shared_ptr<grpc::ChannelCredentials> creds{nullptr};
  grpc::ChannelArguments channel_args;
  // channel_args.SetMaxReceiveMessageSize(128*1024*1024);
  if (use_tls) {
    auto link_context = this->getLinkContext();
    auto& cert_config = link_context->getCertificateConfig();
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = cert_config.rootCAContent();
    ssl_opts.pem_private_key = cert_config.keyContent();
    ssl_opts.pem_cert_chain = cert_config.certContent();
    creds = grpc::SslCredentials(ssl_opts);
  } else {
    creds = grpc::InsecureChannelCredentials();
  }
  grpc_channel_ = grpc::CreateCustomChannel(server_address,
                                            creds, channel_args);
  return grpc_channel_;
}

retcode GrpcChannel::sendRecv(const std::string& role,
    std::string_view send_data, std::string* recv_data) {
  grpc::ClientContext context;
  auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  if (send_tiemout_ms > 0) {
    auto deadline = std::chrono::system_clock::now() +
      std::chrono::milliseconds(send_tiemout_ms);
    context.set_deadline(deadline);
  }
  using reader_writer_t =
      grpc::ClientReaderWriter<rpc::TaskRequest, rpc::TaskResponse>;
  std::shared_ptr<reader_writer_t> client_stream(stub_->SendRecv(&context));
  std::vector<rpc::TaskRequest> send_requests;
  buildTaskRequest(role, send_data, &send_requests);
  std::string TASK_INFO_STR;
  if (!send_requests.empty()) {
    auto& task_info = send_requests[0].task_info();
    TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  }
  for (const auto& request : send_requests) {
    client_stream->Write(request);
  }
  client_stream->WritesDone();
  // waiting for response
  rpc::TaskResponse recv_response;
  while (client_stream->Read(&recv_response)) {
    auto data = recv_response.data();
    recv_data->append(data);
  }
  grpc::Status status = client_stream->Finish();
  if (!status.ok()) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR
        << "recv data encountes error, detail: "
        << status.error_code() << ": " << status.error_message();
    return retcode::FAIL;
  }
  PH_VLOG(5, LogType::kTask)
      << TASK_INFO_STR
      << "recv data success, data size: " << recv_data->size();
  return retcode::SUCCESS;
}

retcode GrpcChannel::sendRecv(const std::string& role,
    const std::string& send_data, std::string* recv_data) {
  std::string_view data_sv{send_data.c_str(), send_data.length()};
  return sendRecv(role, data_sv, recv_data);
}

bool GrpcChannel::send_wrapper(const std::string& role,
                               const std::string& data) {
    auto ret = this->send(role, data);
    if (ret != retcode::SUCCESS) {
      rpc::TaskContext task_info;
      BuildTaskInfo(&task_info);
      std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
      PH_LOG(ERROR, LogType::kTask) << "send data encountes error";
      return false;
    }
    return true;
}

bool GrpcChannel::send_wrapper(const std::string& role,
                               std::string_view sv_data) {
    auto ret = this->send(role, sv_data);
    if (ret != retcode::SUCCESS) {
        rpc::TaskContext task_info;
        BuildTaskInfo(&task_info);
        PH_LOG(ERROR, LogType::kTask)
            << pb_util::TaskInfoToString(task_info)
            << "send data encountes error";
        return false;
    }
    return true;
}
retcode GrpcChannel::send(const std::string& role, const std::string& data) {
  std::string_view data_sv(data.c_str(), data.length());
  return send(role, data_sv);
}

retcode GrpcChannel::send(const std::string& role, std::string_view data_sv) {
  // VLOG(5) << "GrpcChannel::send begin to send, use key: " << role;
  std::vector<rpc::TaskRequest> send_requests;
  buildTaskRequest(role, data_sv, &send_requests);
  auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  int retry_time{0};
  std::string TASK_INFO_STR;
  if (!send_requests.empty()) {
    const auto& task_info = send_requests[0].task_info();
    TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  }
  do {
    grpc::ClientContext context;
    if (send_tiemout_ms > 0) {
      auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(send_tiemout_ms);
      context.set_deadline(deadline);
    }
    rpc::TaskResponse task_response;
    using writer_t = grpc::ClientWriter<rpc::TaskRequest>;
    std::unique_ptr<writer_t> writer(stub_->Send(&context, &task_response));
    for (const auto& request : send_requests) {
      writer->Write(request);
    }
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (status.ok()) {
      auto ret_code = task_response.ret_code();
      if (ret_code) {
          PH_LOG(ERROR, LogType::kTask)
              << "send data to [" << dest_node_.to_string()
              << "] return failed error code: " << ret_code;
          return retcode::FAIL;
      }
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << "send data to [" << dest_node_.to_string()
          << "] failed. error_code: " << status.error_code() << " "
          << "error message: " << status.error_message() << " "
          << "retry: " << retry_time;
      retry_time++;
      if (retry_time < retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send data to [" << dest_node_.to_string()
            << "] failed. error_code: " << status.error_code() << " "
            << "error message: " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  // VLOG(5) << "GrpcChannel::send end of execute, use key: " << role;
  return retcode::SUCCESS;
}

retcode GrpcChannel::buildTaskRequest(const std::string& role,
    const std::string& data, std::vector<rpc::TaskRequest>* send_pb_data) {
  std::string_view data_sv(data.c_str(), data.length());
  return buildTaskRequest(role, data_sv, send_pb_data);
}

retcode GrpcChannel::buildTaskRequest(const std::string& role,
    std::string_view data_sv, std::vector<rpc::TaskRequest>* send_pb_data) {
  size_t sended_size = 0;
  size_t max_package_size = LIMITED_PACKAGE_SIZE;  // limit data size 4M
  size_t total_length = data_sv.size();
  char* send_buf = const_cast<char*>(data_sv.data());
  // VLOG(5) << "job_id: " << job_id << " "
  //     << "task_id: " << task_id << " "
  //     << "request id: " << request_id << " "
  //     << "key: " << role << " "
  //     << "send data length: " << total_length;
  do {
    rpc::TaskRequest task_request;
    auto task_info = task_request.mutable_task_info();
    BuildTaskInfo(task_info);
    task_request.set_role(role);
    task_request.set_data_len(total_length);
    auto data_ptr = task_request.mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    // VLOG(5) << "sended_size: " << sended_size << " data_len: " << data_len;
    send_pb_data->emplace_back(std::move(task_request));
    if (sended_size < total_length) {
      continue;
    } else {
      break;
    }
  } while (true);
  return retcode::SUCCESS;
}

std::string GrpcChannel::forwardRecv(const std::string& role) {
  SCopedTimer timer;
  grpc::ClientContext context;
  auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  if (send_tiemout_ms > 0) {
    auto deadline = std::chrono::system_clock::now() +
      std::chrono::milliseconds(send_tiemout_ms);
    context.set_deadline(deadline);
  }
  rpc::TaskRequest send_request;
  auto task_info = send_request.mutable_task_info();
  BuildTaskInfo(task_info);
  send_request.set_role(role);
  // VLOG(5) << "forwardRecv request info: job_id: "
  //         << this->getLinkContext()->job_id()
  //         << " task_id: " << this->getLinkContext()->task_id()
  //         << " request id: " << this->getLinkContext()->request_id()
  //         << " recv key: " << role
  //         << " nodeinfo: " << this->dest_node_.to_string();
  // using reader_t = grpc::ClientReader<rpc::TaskRequest>;
  auto client_reader = this->stub_->ForwardRecv(&context, send_request);

  // waiting for response
  std::string tmp_buff;
  rpc::TaskRequest recv_response;
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(*task_info);
  bool init_flag{false};
  while (client_reader->Read(&recv_response)) {
    auto data = recv_response.data();
    if (!init_flag) {
      size_t data_len = recv_response.data_len();
      tmp_buff.reserve(data_len);
      init_flag = true;
    }
    PH_VLOG(5, LogType::kTask)
        << "data length: " << data.size();
    tmp_buff.append(data);
  }

  grpc::Status status = client_reader->Finish();
  if (!status.ok()) {
    PH_LOG(ERROR, LogType::kTask)
        << "recv data encountes error, detail: "
        << status.error_code() << ": " << status.error_message();
    return std::string("");
  }
  // VLOG(5) << "recv data success, data size: " << tmp_buff.size();
  auto time_cost = timer.timeElapse();
  PH_VLOG(5, LogType::kTask)
      << "forwardRecv time cost(ms): " << time_cost;
  return tmp_buff;
}

retcode GrpcChannel::submitTask(const rpc::PushTaskRequest& request,
                                rpc::PushTaskReply* reply) {
  int retry_time{0};
  const auto& task_info = request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    grpc::Status status = stub_->SubmitTask(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kScheduler)
          << TASK_INFO_STR
          << "send submitTask to node: ["
          <<  dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kScheduler)
          << TASK_INFO_STR
          << "send submitTask to Node ["
          << dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kScheduler)
            << TASK_INFO_STR
            << "send submitTask to Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (0);

  return retcode::SUCCESS;
}

retcode GrpcChannel::executeTask(const rpc::PushTaskRequest& request,
                                 rpc::PushTaskReply* reply) {
  int retry_time{0};
  const auto& task_info = request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    auto task_info = request.task().task_info();
    grpc::Status status = stub_->ExecuteTask(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR
          << "send ExecuteTask to node: ["
          << dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << TASK_INFO_STR
          << "send ExecuteTask to Node ["
          << dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send ExecuteTask to Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcChannel::StopTask(const rpc::TaskContext& request,
                              rpc::Empty* reply) {
  int retry_time{0};
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(request);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    grpc::Status status = stub_->StopTask(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR
          << "send StopTask to node: ["
          <<  dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << TASK_INFO_STR
          << "send StopTask to Node ["
          <<  dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send StopTask to Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcChannel::killTask(const rpc::KillTaskRequest& request,
                              rpc::KillTaskResponse* reply) {
  int retry_time{0};
  const auto& task_info = request.task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    grpc::Status status = stub_->KillTask(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR
          << "send killTask to node: ["
          << dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << TASK_INFO_STR
          << "send KillTask to Node ["
          << dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send KillTask to Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcChannel::updateTaskStatus(const rpc::TaskStatus& request,
                                      rpc::Empty* reply) {
  int retry_time{0};
  const auto& task_info = request.task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    grpc::Status status = stub_->UpdateTaskStatus(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR
          << "send UpdateTaskStatus to node: ["
          << dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << TASK_INFO_STR
          << "send UpdateTaskStatus to Node ["
          << dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send UpdateTaskStatus to Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcChannel::fetchTaskStatus(const rpc::TaskContext& request,
                                     rpc::TaskStatusReply* reply) {
  int retry_time{0};
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(request);
  do {
    grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
    context.set_deadline(deadline);
    grpc::Status status = stub_->FetchTaskStatus(&context, request, reply);
    if (status.ok()) {
      PH_VLOG(5, LogType::kTask)
          << TASK_INFO_STR
          << "send FetchTaskStatus from node: ["
          << dest_node_.to_string() << "] rpc succeeded.";
      break;
    } else {
      PH_LOG(WARNING, LogType::kTask)
          << TASK_INFO_STR
          << "send FetchTaskStatus from Node ["
          << dest_node_.to_string() << "] rpc failed. "
          << status.error_code() << ": " << status.error_message() << " "
          << "retry times: " << retry_time;
      retry_time++;
      if (retry_time < this->retry_max_times_) {
        continue;
      } else {
        PH_LOG(ERROR, LogType::kTask)
            << TASK_INFO_STR
            << "send FetchTaskStatus from Node ["
            << dest_node_.to_string() << "] rpc failed. "
            << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
      }
    }
  } while (true);
  return retcode::SUCCESS;
}

std::shared_ptr<IChannel> GrpcLinkContext::buildChannel(
    const primihub::Node& node,
    LinkContext* link_ctx) {
  return std::make_shared<GrpcChannel>(node, link_ctx);
}

std::shared_ptr<IChannel> GrpcLinkContext::getChannel(
    const primihub::Node& node) {
  std::string node_info = node.to_string();
  {
    std::shared_lock<std::shared_mutex> lck(this->connection_mgr_mtx);
    auto it = connection_mgr.find(node_info);
    if (it != connection_mgr.end()) {
      return it->second;
    }
  }
  // create channel
  auto channel = buildChannel(node, this);
  {
    std::lock_guard<std::shared_mutex> lck(this->connection_mgr_mtx);
    connection_mgr[node_info] = channel;
  }
  return channel;
}

// dataset related operation
retcode GrpcChannel::DownloadData(const rpc::DownloadRequest& request,
                                  std::vector<std::string>* data) {
  grpc::ClientContext context;
  auto deadline = std::chrono::system_clock::now() +
      std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
  context.set_deadline(deadline);
  auto client_reader = this->dataset_stub_->DownloadData(&context, request);

  const auto& request_id = request.request_id();
  rpc::DownloadRespone response;
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(request_id);
  bool has_error{false};
  std::string err_msg;
  while (client_reader->Read(&response)) {
    if (response.code() != rpc::Status::SUCCESS) {
      has_error = true;
      err_msg = response.info();
      break;
    }
    auto block_data = response.mutable_data();
    data->push_back(std::move(*block_data));
  }

  grpc::Status status = client_reader->Finish();
  if (!status.ok()) {
    LOG(ERROR) << TASK_INFO_STR
               << "recv data encountes error, detail: "
               << status.error_code() << ": " << status.error_message();
    return retcode::FAIL;
  }
  if (has_error) {
    LOG(ERROR) << "download data encountes error: " << err_msg;
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

std::shared_ptr<arrow::Table> GrpcChannel::FetchData(
    const std::string& request_id,
    const std::string& dataset_id,
    std::shared_ptr<arrow::Schema> schema) {
  grpc::ClientContext context;
  auto deadline = std::chrono::system_clock::now() +
      std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
  seatunnel::rpc::MetaDataSetDataStream request;
  request.set_data_set_id(dataset_id);
  request.set_trace_id(request_id);
  seatunnel::rpc::MetaDataSetDataStream response;
  context.set_deadline(deadline);
  VLOG(5) << "client begin to fetch data";
  auto client_reader = this->seatunnel_stub_->FetchData(&context, request);
  std::map<std::string, std::shared_ptr<arrow::ArrayBuilder>> builder_map;
  size_t loop = 0;
  while (client_reader->Read(&response)) {
    VLOG(5) << "client_reader->Read done for seq: " << loop << " "
            << "content: " << primihub::proto::util::TypeToString(response);
    const auto& data_info = response.data();
    for (const auto& [file_name, value] : data_info) {
      auto type = value.var_type();
      auto field_ptr = schema->GetFieldByName(file_name);
      auto field_type = field_ptr->type()->id();
      if (builder_map.find(file_name) == builder_map.end()) {
        builder_map[file_name] = arrow_util::CreateBuilder(field_type);
        LOG(ERROR) << "create builder for: " << file_name;
      }
      LOG(INFO) << "AddDataToBuilder: " << field_type
                << "file_name: " << file_name;
      AddDataToBuilder(value, field_type, builder_map[file_name]);
    }
    loop++;
  }
  VLOG(5) << "builder_map: " << builder_map.size();
  // build data
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  for (const auto& name : schema->field_names()) {
    VLOG(5) << "name: " << name;
    std::shared_ptr<arrow::Array> arr;
    auto it = builder_map.find(name);
    if (it == builder_map.end()) {
      LOG(ERROR) << "no data for field name: " << name;
      return nullptr;
    }
    auto& builder = it->second;
    builder->Finish(&arr);
    array_data.push_back(std::move(arr));
  }
  return arrow::Table::Make(schema, array_data);
}

retcode GrpcChannel::AddDataToBuilder(const seatunnel::rpc::Project& value,
    int expected_type, std::shared_ptr<arrow::ArrayBuilder> builder) {
  retcode ret{retcode::SUCCESS};
  auto type = value.var_type();
  switch (type) {
  case seatunnel::rpc::INT32: {
    ret = arrow_util::AddIntValue(value.value_int32(),
                                  expected_type, builder);
    break;
  }
  case seatunnel::rpc::INT64: {
    ret = arrow_util::AddIntValue(value.value_int64(),
                                  expected_type, builder);
    break;
  }
  case seatunnel::rpc::STRING: {
    ret = arrow_util::AddStringValue(value.value_string(),
                                     expected_type, builder);
    break;
  }
  case seatunnel::rpc::BYTE: {
    ret = arrow_util::AddStringValue(value.value_bytes(),
                                     expected_type, builder);
    break;
  }
  case seatunnel::rpc::FLOAT: {
    ret = arrow_util::AddDoubleValue(value.value_float(),
                                     expected_type, builder);
    break;
  }
  case seatunnel::rpc::DOUBLE: {
    ret = arrow_util::AddDoubleValue(value.value_double(),
                                     expected_type, builder);
    break;
  }
  case seatunnel::rpc::BOOL: {
    ret = arrow_util::AddBoolValue(value.value_bool(), expected_type, builder);
    break;
  }
  default: {
    LOG(WARNING) << "unknown type: " << static_cast<int>(type)
                 << " using string instead";
    ret = arrow_util::AddStringValue(value.value_string(),
                                     expected_type, builder);
    break;
  }
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::network
