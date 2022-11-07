#ifndef SRC_PRIMIHUB_GRPC_TEST_GRPC_TEST_SERVER_H_H_
#define SRC_PRIMIHUB_GRPC_TEST_GRPC_TEST_SERVER_H_H_

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "src/primihub/protos/helloworld.grpc.pb.h"

namespace ns_rpc = primihub::rpc;
using primihub::rpc::Greeter;
namespace primihub::grpc_test {
class GreeterServiceImpl final : public ns_rpc::Greeter::Service {
 public:
  GreeterServiceImpl() = default;
  grpc::Status SayHello(grpc::ServerContext* context,
                        const ns_rpc::HelloRequest* request,
                        ns_rpc::HelloReply* response);
};
}  //  namespace primihub::grpc_test
#endif  // SRC_PRIMIHUB_GRPC_TEST_GRPC_TEST_SERVER_H_H_