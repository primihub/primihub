// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_GRPC_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_GRPC_LINK_CONTEXT_H_
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "arrow/api.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/common.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/protos/service.grpc.pb.h"
#include "src/primihub/protos/metadatastream.grpc.pb.h"

namespace primihub::network {
namespace rpc = primihub::rpc;
class GrpcChannel : public IChannel {
 public:
  GrpcChannel(const primihub::Node& node, LinkContext* link_ctx);
  virtual ~GrpcChannel() = default;
  retcode send(const std::string& role, const std::string& data) override;
  retcode send(const std::string& role, std::string_view sv_data) override;
  bool send_wrapper(const std::string& role, const std::string& data) override;
  bool send_wrapper(const std::string& role, std::string_view sv_data) override;
  retcode sendRecv(const std::string& role,
                   const std::string& send_data,
                   std::string* recv_data) override;
  retcode sendRecv(const std::string& role,
                   std::string_view send_data, std::string* recv_data) override;
  retcode submitTask(const rpc::PushTaskRequest& request,
                     rpc::PushTaskReply* reply) override;
  retcode executeTask(const rpc::PushTaskRequest& request,
                      rpc::PushTaskReply* reply) override;
  retcode StopTask(const rpc::TaskContext& request,
                   rpc::Empty* reply) override;
  retcode killTask(const rpc::KillTaskRequest& request,
                   rpc::KillTaskResponse* reply) override;

  retcode updateTaskStatus(const rpc::TaskStatus& request,
                           rpc::Empty* reply) override;
  retcode fetchTaskStatus(const rpc::TaskContext& request,
                          rpc::TaskStatusReply* reply) override;
  std::string forwardRecv(const std::string& role) override;
  retcode buildTaskRequest(const std::string& role,
                           const std::string& data,
                           std::vector<rpc::TaskRequest>* send_pb_data);
  retcode buildTaskRequest(const std::string& role,
                           std::string_view sv_data,
                           std::vector<rpc::TaskRequest>* send_pb_data);
  std::shared_ptr<grpc::Channel> buildChannel(std::string& server_addr,
                                              bool use_tls);
  // data set related operation
  retcode DownloadData(const rpc::DownloadRequest& request,
                       std::vector<std::string>* data) override;
  std::shared_ptr<arrow::Table>
  FetchData(const std::string& request_id,
            const std::string& dataset_id,
            std::shared_ptr<arrow::Schema> schema) override;

 protected:
  retcode BuildTaskInfo(rpc::TaskContext* task_info);
  retcode AddDataToBuilder(const seatunnel::rpc::Project& value,
                           int expected_type,
                           std::shared_ptr<arrow::ArrayBuilder> builder);

 private:
  std::unique_ptr<rpc::VMNode::Stub> stub_{nullptr};
  std::unique_ptr<rpc::DataSetService::Stub> dataset_stub_{nullptr};
  std::shared_ptr<grpc::Channel> grpc_channel_{nullptr};
  std::unique_ptr<seatunnel::rpc::MetaStreamService::Stub> seatunnel_stub_{nullptr};
  primihub::Node dest_node_;
  int retry_max_times_{3};
};

class GrpcLinkContext : public LinkContext {
 public:
  GrpcLinkContext() = default;
  virtual ~GrpcLinkContext() = default;
  std::shared_ptr<IChannel> buildChannel(const primihub::Node& node,
                                         LinkContext* link_ctx);
  std::shared_ptr<IChannel> getChannel(const primihub::Node& node) override;
};

}  // namespace primihub::network
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_GRPC_LINK_CONTEXT_H_
