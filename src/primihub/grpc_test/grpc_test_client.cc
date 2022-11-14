#include "src/primihub/grpc_test/grpc_test_client.h"

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "src/primihub/protos/helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using primihub::rpc::HelloRequest;
using primihub::rpc::HelloReply;
using primihub::rpc::Greeter;

class GreeterClient {
public:
    GreeterClient(std::shared_ptr<Channel> channel) : stub_(Greeter::NewStub(channel)) {}

    //客户端请求函数
    std::string SayHello(const std::string &user) {
        //我们将要发送到服务器端的数据
        HelloRequest request;
        request.set_name(user);

        //我们从服务端收到的数据
        HelloReply reply;

        //客户端RPC上下文
        ClientContext context;

        //调用rpc函数
        Status status = stub_->SayHello(&context, request, &reply);

        //返回结果处理
        if(status.ok()) {
            return reply.message();
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return "RPC failed";
        }
    }

private:
    std::unique_ptr<Greeter::Stub> stub_;
};

std::string get_file_contents(const std::string& fpath) {
  std::ifstream finstream(fpath);
  std::string contents((std::istreambuf_iterator<char>(finstream)),
                       std::istreambuf_iterator<char>());
  return contents;
}

int main(int argc, char** argv) {
  // GreeterClient greeter(grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials()));

  // auto server_ca_pem = get_file_contents(argv[1]);
  // auto my_key_pem = get_file_contents(argv[2]);
  // auto my_cert_pem = get_file_contents(argv[3]);
  std::string cert_file_path{"data/cert/client.crt"};
  std::string key_file_path{"data/cert/client.key"};
  std::string root_file_path{"data/cert/ca.crt"};
  std::string key = get_file_contents(key_file_path);
  std::string cert = get_file_contents(cert_file_path);
  std::string root_ca = get_file_contents(root_file_path);

  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = root_ca;
  ssl_opts.pem_private_key = key;
  ssl_opts.pem_cert_chain = cert;

  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(ssl_opts);

  std::string server_address{"www.primihub.server.com:50051"};
  // std::string server_address{"127.0.0.1:50051"};
  GreeterClient greeter(grpc::CreateChannel(server_address, creds));

  std::string user{"world"};

  std::string reply = greeter.SayHello(user);

  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}