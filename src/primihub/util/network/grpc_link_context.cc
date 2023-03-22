// Copyright [2022] <primihub.com>
#include "src/primihub/util/network/grpc_link_context.h"
#include <glog/logging.h>

namespace primihub::network {
GrpcChannel::GrpcChannel(const primihub::Node& node, LinkContext* link_ctx) : IChannel(link_ctx) {
  dest_node_ = node;
  std::string address_ = node.ip_ + ":" + std::to_string(node.port_);
  auto channel = buildChannel(address_, node.use_tls_);
  stub_ = rpc::VMNode::NewStub(channel);
}

std::shared_ptr<grpc::Channel> GrpcChannel::buildChannel(std::string& server_address, bool use_tls) {
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
  grpc_channel_ = grpc::CreateCustomChannel(server_address, creds, channel_args);
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
  using reader_writer_t = grpc::ClientReaderWriter<rpc::TaskRequest, rpc::TaskResponse>;
  std::shared_ptr<reader_writer_t> client_stream(stub_->SendRecv(&context));
  std::vector<rpc::TaskRequest> send_requests;
  buildTaskRequest(role, send_data, &send_requests);
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
    LOG(ERROR) << "recv data encountes error, detail: "
      << status.error_code() << ": " << status.error_message();
    return retcode::FAIL;
  }
  VLOG(5) << "recv data success, data size: " << recv_data->size();
  return retcode::SUCCESS;
}

retcode GrpcChannel::sendRecv(const std::string& role,
    const std::string& send_data, std::string* recv_data) {
  std::string_view data_sv{send_data.c_str(), send_data.length()};
  return sendRecv(role, data_sv, recv_data);
}

bool GrpcChannel::send_wrapper(const std::string& role, const std::string& data) {
    auto ret = this->send(role, data);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send data encountes error";
        return false;
    }
    return true;
}

bool GrpcChannel::send_wrapper(const std::string& role, std::string_view sv_data) {
    auto ret = this->send(role, sv_data);
    if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "send data encountes error";
        return false;
    }
    return true;
}
retcode GrpcChannel::send(const std::string& role, const std::string& data) {
  std::string_view data_sv(data.c_str(), data.length());
  return send(role, data_sv);
}

retcode GrpcChannel::send(const std::string& role, std::string_view data_sv) {
  VLOG(5) << "execute GrpcChannel::send";
  grpc::ClientContext context;
  auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  if (send_tiemout_ms > 0) {
    auto deadline = std::chrono::system_clock::now() +
      std::chrono::milliseconds(send_tiemout_ms);
    context.set_deadline(deadline);
  }
  rpc::TaskResponse task_response;
  using writer_t = grpc::ClientWriter<rpc::TaskRequest>;
  std::unique_ptr<writer_t> writer(stub_->Send(&context, &task_response));
  std::vector<rpc::TaskRequest> send_requests;
  buildTaskRequest(role, data_sv, &send_requests);
  for (const auto& request : send_requests) {
    writer->Write(request);
  }
  writer->WritesDone();
  grpc::Status status = writer->Finish();
  if (status.ok()) {
    auto ret_code = task_response.ret_code();
    if (ret_code) {
        LOG(ERROR) << "send data to [" << dest_node_.to_string()
          << "] return failed error code: " << ret_code;
        return retcode::FAIL;
    }
  } else {
    LOG(ERROR) << "send data to [" << dest_node_.to_string()
              << "] failed. error_code: "
              << status.error_code() << ": " << status.error_message();
    return retcode::FAIL;
  }
  VLOG(5) << "end of execute GrpcChannel::send";
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
  constexpr size_t max_package_size = 1 << 21;  // limit data size 4M
  size_t total_length = data_sv.size();
  char* send_buf = const_cast<char*>(data_sv.data());
  std::string job_id = this->getLinkContext()->job_id();
  std::string task_id = this->getLinkContext()->task_id();
  VLOG(5) << "job_id: " << job_id << " "
      << "task_id: " << task_id << " "
      << "role: " << role << " "
      << "send data length: " << total_length;
  do {
    rpc::TaskRequest task_request;
    task_request.set_job_id(job_id);
    task_request.set_task_id(task_id);
    task_request.set_role(role);
    task_request.set_data_len(total_length);
    auto data_ptr = task_request.mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    VLOG(5) << "sended_size: " << sended_size << " data_len: " << data_len;
    send_pb_data->emplace_back(std::move(task_request));
    if (sended_size < total_length) {
      continue;
    } else {
      break;
    }
  } while(true);
  return retcode::SUCCESS;
}

std::string GrpcChannel::forwardRecv(const std::string& role) {
    grpc::ClientContext context;
    auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
    if (send_tiemout_ms > 0) {
      auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(send_tiemout_ms);
      context.set_deadline(deadline);
    }
    rpc::TaskRequest send_request;
    send_request.set_task_id(this->getLinkContext()->task_id());
    send_request.set_job_id(this->getLinkContext()->job_id());
    send_request.set_role(role);
    VLOG(5) << "send request info: job_id: " << this->getLinkContext()->job_id()
            << " task_id: " << this->getLinkContext()->task_id()
            << " role: " << role
            << " nodeinfo: " << this->dest_node_.to_string();
    // using reader_t = grpc::ClientReader<rpc::TaskRequest>;
    auto client_reader = this->stub_->ForwardRecv(&context, send_request);

    // waiting for response
    std::string tmp_buff;
    rpc::TaskRequest recv_response;
    bool init_flag{false};
    while (client_reader->Read(&recv_response)) {
        auto data = recv_response.data();
        if (!init_flag) {
          size_t data_len = recv_response.data_len();
          tmp_buff.reserve(data_len);
          init_flag = true;
        }
        VLOG(5) << "data: " << data;
        tmp_buff.append(data);
    }

    grpc::Status status = client_reader->Finish();
    VLOG(5) << "received data: " << tmp_buff;
    if (!status.ok()) {
        LOG(ERROR) << "recv data encountes error, detail: "
            << status.error_code() << ": " << status.error_message();
        return std::string("");
    }
    VLOG(5) << "recv data success, data size: " << tmp_buff.size();
    return tmp_buff;
}

retcode GrpcChannel::submitTask(const rpc::PushTaskRequest& request, rpc::PushTaskReply* reply) {
    grpc::ClientContext context;
    grpc::Status status =
        stub_->SubmitTask(&context, request, reply);
    if (status.ok()) {
        VLOG(5) << "submitTask to node: ["
                <<  dest_node_.to_string() << "] rpc succeeded.";
    } else {
        LOG(ERROR) << "submitTask to Node ["
                  <<  dest_node_.to_string() << "] rpc failed. "
                  << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

retcode GrpcChannel::killTask(const rpc::KillTaskRequest& request, rpc::KillTaskResponse* reply) {
    grpc::ClientContext context;
    grpc::Status status =
        stub_->KillTask(&context, request, reply);
    if (status.ok()) {
        VLOG(5) << "killTask to node: ["
                <<  dest_node_.to_string() << "] rpc succeeded.";
    } else {
        LOG(ERROR) << "killTask to Node ["
                  <<  dest_node_.to_string() << "] rpc failed. "
                  << status.error_code() << ": " << status.error_message();
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

std::shared_ptr<IChannel> GrpcLinkContext::buildChannel(const primihub::Node& node, LinkContext* link_ctx) {
  return std::make_shared<GrpcChannel>(node, link_ctx);
}

std::shared_ptr<IChannel> GrpcLinkContext::getChannel(const primihub::Node& node) {
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

}  // primihub::network
