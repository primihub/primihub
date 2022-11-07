#include "src/primihub/grpc_test/grpc_test_server.h"

#include <glog/logging.h>
#include <fstream>
namespace primihub::grpc_test {
grpc::Status GreeterServiceImpl::SayHello(grpc::ServerContext* context,
    const ns_rpc::HelloRequest* request, ns_rpc::HelloReply* response) {
//
  const auto& name = request->name();
  std::string reply_msg = "recvived from: " + name;
  VLOG(0) << reply_msg;
  response->set_message(reply_msg);
  return grpc::Status::OK;
}
std::string get_file_contents(const std::string& fpath) {
  std::ifstream finstream(fpath);
  std::string contents((std::istreambuf_iterator<char>(finstream)),
                       std::istreambuf_iterator<char>());
  return contents;
}

void runServer() {
  std::string key;
  std::string cert;
  std::string root;
  std::string cert_file_path{"data/cert/server.crt"};
  std::string key_file_path{"data/cert/server.key"};
  std::string root_file_path{"data/cert/ca.crt"};
  key = get_file_contents(key_file_path);
  cert = get_file_contents(cert_file_path);
  root = get_file_contents(root_file_path);

  grpc::SslServerCredentialsOptions::PemKeyCertPair keycert{key, cert};

  // grpc::SslServerCredentialsOptions ssl_opts;
  grpc::SslServerCredentialsOptions ssl_opts(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
  ssl_opts.pem_root_certs = root;
  ssl_opts.pem_key_cert_pairs.push_back(std::move(keycert));

  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl greeter_service;
  grpc::ServerBuilder builder;

  std::shared_ptr<grpc::ServerCredentials> creds = grpc::SslServerCredentials(ssl_opts);
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, creds);
  // builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // registe service
  builder.RegisterService(&greeter_service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  VLOG(0) << "Server listening on " << server_address;

  //等待服务
  server->Wait();
}

}  // namespace primihub::grpc_test

int main() {
  primihub::grpc_test::runServer();
  return 0;
}