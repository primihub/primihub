/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */



#ifndef SRC_PRIMIHUB_CLI_CLI_H_
#define SRC_PRIMIHUB_CLI_CLI_H_

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/util/util.h"
#include "src/primihub/protos/service.grpc.pb.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/common.h"

using primihub::rpc::VMNode;
using primihub::rpc::PushTaskRequest;
using primihub::rpc::PushTaskReply;
using primihub::rpc::Task;
using primihub::rpc::TaskType;
using primihub::rpc::Language;
using primihub::rpc::Params;
using primihub::rpc::VarType;

namespace primihub {
class SDKClient {
 public:
  explicit SDKClient(std::shared_ptr<primihub::network::IChannel> channel,
        primihub::network::LinkContext* link_ctx)
      : channel_(channel), link_ctx_ref_(link_ctx) {
  }

  retcode SubmitTask(const rpc::PushTaskRequest& task_request,
                     rpc::PushTaskReply* task_info);
  retcode DownloadData(const rpc::TaskContext& request_id,
                       const std::vector<std::string>& file_list,
                       std::vector<std::string>* recv_data);
  retcode SaveData(const std::string& file_name,
                   const std::vector<std::string>& recv_data);
  retcode CheckTaskStauts(const rpc::PushTaskReply& task_reply_info);
  retcode RegisterDataset(const rpc::NewDatasetRequest& req,
                          rpc::NewDatasetResponse* reply);
 private:
  std::shared_ptr<primihub::network::IChannel> channel_{nullptr};
  primihub::network::LinkContext* link_ctx_ref_{nullptr};
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_CLI_CLI_H_
