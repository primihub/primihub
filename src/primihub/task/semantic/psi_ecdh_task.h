/*
 Copyright 2022 Primihub

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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_ECDH_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_ECDH_TASK_H_

// #include <grpc/grpc.h>
// #include <grpcpp/channel.h>
// #include <grpcpp/create_channel.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <set>

#include "private_set_intersection/cpp/psi_client.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/psi.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/semantic/psi_task.h"

namespace rpc = primihub::rpc;
namespace openminded_psi = private_set_intersection;
namespace primihub::task {
class PSIEcdhTask : public TaskBase, public PsiCommonOperator {
 public:
  explicit PSIEcdhTask(const TaskParam* task_param,
                        std::shared_ptr<DatasetService> dataset_service);
  ~PSIEcdhTask() = default;

  int execute() override;

  bool runAsClient() {
    return run_as_client_;
  }
  // client method
  int executeAsClient();
  int saveResult();
  int sendRequetToServer(psi_proto::Request&& psi_request);
  retcode broadcastResultToServer();
  retcode buildInitParam(std::string* init_param);
  retcode sendInitParam(const std::string& init_param);
  retcode sendPSIRequestAndWaitResponse(const psi_proto::Request& request, rpc::PsiResponse* response);
  retcode sendPSIRequestAndWaitResponse(psi_proto::Request&& request, rpc::PsiResponse* response);
  retcode parsePsiResponseFromeString(const std::string& res_str, rpc::PsiResponse* response);
  // server method
  int executeAsServer();
  int initRequest(psi_proto::Request* psi_request);
  retcode preparePSIResponse(psi_proto::Response&& psi_response, psi_proto::ServerSetup&& setup);
  int recvPSIResult();
  int recvInitParam(size_t* client_dataset_size, bool* reveal_intersection);
  void set_fpr(double fpr) {
    fpr_ = fpr;
  }

 private:
  int LoadParams(Task& task);
  retcode LoadDataset();
  int GetIntsection(const std::unique_ptr<openminded_psi::PsiClient>& client,
                    rpc::PsiResponse& taskResponse);
  std::vector<int> data_index_;
  int psi_type_;
  std::string dataset_path_;
  std::string dataset_id_;
  std::string result_file_path_;
  bool reveal_intersection_;
  std::vector<std::string> elements_;
  std::vector<std::string> result_;
  std::string server_address_;
  std::string server_dataset_;
  rpc::ParamValue server_index_;
  bool sync_result_to_server{false};
  std::string server_result_path;
  double fpr_{0.0001};
  bool run_as_client_{false};
  primihub::Node peer_node;
  std::string key{"default"};
};

} // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_ECDH_TASK_H_
