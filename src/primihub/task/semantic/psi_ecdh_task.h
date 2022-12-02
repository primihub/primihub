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

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <set>

#include "private_set_intersection/cpp/psi_client.h"

#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/protos/psi.grpc.pb.h"
#include "src/primihub/protos/worker.grpc.pb.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/common/defines.h"

// using grpc::ClientContext;
using grpc::Status;
using grpc::Channel;
namespace rpc = primihub::rpc;
using primihub::rpc::Task;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
using primihub::rpc::ExecuteTaskRequest;
using primihub::rpc::ExecuteTaskResponse;
using primihub::rpc::TaskRequest;
using primihub::rpc::TaskResponse;
using primihub::rpc::PsiRequest;
using primihub::rpc::PsiResponse;
using primihub::rpc::VMNode;
namespace openminded_psi = private_set_intersection;
// using private_set_intersection::PsiClient;
// using private_set_intersection::PsiServer;
namespace primihub::task {

class PSIECDHTask : public TaskBase {
 public:
  explicit PSIECDHTask(const TaskParam* task_param,
                        std::shared_ptr<DatasetService> dataset_service);
  ~PSIECDHTask() = default;

  int execute() override;
  void setTaskInfo(const std::string& node_id,
      const std::string& job_id,
      const std::string& task_id,
      const std::string& submit_client_id);

  bool runAsClient() {
    return run_as_client_;
  }
  int saveDataToCSVFile(const std::vector<std::string>& data,
      const std::string& file_path, const std::string& col_title);
  // client method
  int executeAsClient();
  int saveResult();
  int sendRequetToServer(psi_proto::Request&& psi_request);
  int send_result_to_server();
  int sendClientInfo();
  int buildInitParam(rpc::TaskRequest* request);
  int sendInitParam(const rpc::TaskRequest& request);
  int sendPSIRequestAndWaitResponse(const psi_proto::Request& request, TaskResponse* response);

  // server method
  int executeAsServer();
  int initRequest(psi_proto::Request* psi_request);
  int preparePSIResponse(psi_proto::Response&& psi_response, psi_proto::ServerSetup&& setup);
  int recvPSIResult();
  int recvClientInfo(size_t* client_dataset_size, bool* reveal_intersection);
  void set_fpr(double fpr) {
    fpr_ = fpr;
  }

 private:
  std::unique_ptr<rpc::VMNode::Stub>& getStub(const std::string& dest_address, bool use_tls);
  int LoadParams(Task& task);
  int LoadDataset();
  int LoadDatasetFromCSV(const std::string& filename, int data_col,
                          std::vector<std::string>& col_array);
  int LoadDatasetFromSQLite(const std::string &conn_str, int data_col,
                              std::vector <std::string>& col_array);
  int GetIntsection(const std::unique_ptr<openminded_psi::PsiClient>& client,
                    TaskResponse& taskResponse);
  std::string node_id_;
  std::string job_id_;
  std::string task_id_;
  std::string submit_client_id_;
  int data_index_;
  int psi_type_;
  std::string dataset_path_;
  std::string result_file_path_;
  bool reveal_intersection_;
  std::vector<std::string> elements_;
  std::vector<std::string> result_;
  std::string server_address_;
  std::string server_dataset_;
  ParamValue server_index_;
  bool sync_result_to_server{false};
  std::string server_result_path;
  double fpr_{0.0001};
  bool run_as_client_{false};
  std::unique_ptr<rpc::VMNode::Stub> peer_connection{nullptr};
};

} // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_ECDH_TASK_H_