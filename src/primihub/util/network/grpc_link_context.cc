// Copyright [2022] <primihub.com>
#include "src/primihub/util/network/grpc_link_context.h"
#include <glog/logging.h>

namespace primihub::network {
GrpcChannel::GrpcChannel(const primihub::Node& node) {
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
    std::string root_ca{""};
    std::string key{""};
    std::string cert{""};
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = root_ca;
    ssl_opts.pem_private_key = key;
    ssl_opts.pem_cert_chain = cert;
    creds = grpc::SslCredentials(ssl_opts);
  } else {
    creds = grpc::InsecureChannelCredentials();
  }
  grpc_channel_ = grpc::CreateCustomChannel(server_address, creds, channel_args);
  return grpc_channel_;
}

retcode GrpcChannel::send(const rpc::TaskRequest& data) {
  grpc::ClientContext context;
  primihub::rpc::TaskResponse task_response;
  using writer_t = grpc::ClientWriter<primihub::rpc::TaskRequest>;
  std::unique_ptr<writer_t> writer(stub_->Send(&context, &task_response));
  writer->Write(data);
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
  return retcode::SUCCESS;
}

retcode GrpcChannel::send(const std::vector<rpc::TaskRequest>& send_datas) {
  grpc::ClientContext context;
  VLOG(5) << "send_result_to_server";
  primihub::rpc::TaskResponse task_response;
  using writer_t = grpc::ClientWriter<primihub::rpc::TaskRequest>;
  std::unique_ptr<writer_t> writer(stub_->Send(&context, &task_response));
  for (const auto& data : send_datas) {
    writer->Write(data);
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
  return retcode::SUCCESS;
}

retcode GrpcChannel::recv(const rpc::TaskRequest& request, std::vector<rpc::TaskResponse>* recv_datas) {
  recv_datas->clear();
  grpc::ClientContext context;
  rpc::TaskRequest task_request;
  using reader_t = grpc::ClientReader<rpc::TaskResponse>;
  std::unique_ptr<reader_t> reader(stub_->Recv(&context, task_request));
  rpc::TaskResponse task_response;
  while (reader->Read(&task_response)) {
    recv_datas->emplace_back(std::move(task_response));
  }
  return retcode::SUCCESS;
}

retcode GrpcChannel::sendRecv(const std::vector<rpc::TaskRequest>& send_data,
                          std::vector<rpc::TaskResponse>* recv_data) {
//
  grpc::ClientContext context;
  using reader_writer_t = grpc::ClientReaderWriter<rpc::TaskRequest, rpc::TaskResponse>;
  std::shared_ptr<reader_writer_t> client_stream(stub_->SendRecv(&context));
  // send request to server
  for (const auto& data : send_data) {
    client_stream->Write(data);
  }
  client_stream->WritesDone();
  // waiting for response
  rpc::TaskResponse recv_response;
  while (client_stream->Read(&recv_response)) {
    recv_data->emplace_back(std::move(recv_response));
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

std::shared_ptr<IChannel> GrpcLinkContext::buildChannel(const primihub::Node& node) {
  return std::make_shared<GrpcChannel>(node);
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
  auto channel = buildChannel(node);
  {
    std::lock_guard<std::shared_mutex> lck(this->connection_mgr_mtx);
    connection_mgr[node_info] = channel;
  }
  return channel;
}

}  // primihub::network
