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

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"

#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/service.grpc.pb.h"

using primihub::rpc::DataSetService;
using primihub::rpc::NewDatasetRequest;
using primihub::rpc::NewDatasetResponse;

namespace primihub {

class RegClient {
 public:
  explicit RegClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(DataSetService::NewStub(channel)) {
  }

  int RegDataSet(const std::string& uid,
                const std::string& data_type,
                const std::string& meta_info);

 private:
  std::unique_ptr<DataSetService::Stub> stub_;
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_CLI_CLI_H_
