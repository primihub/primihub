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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <set>

#include "src/primihub/protos/common.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/common/common.h"
#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "src/primihub/kernel/psi/util.h"

namespace rpc = primihub::rpc;

namespace primihub::task {
using BasePsiOperator = primihub::psi::BasePsiOperator;

class PsiTask : public TaskBase, public primihub::psi::PsiCommonUtil {
 public:
  PsiTask(const TaskParam *task_param);
  PsiTask(const TaskParam *task_param,
          std::shared_ptr<DatasetService> dataset_service);
  PsiTask(const TaskParam *task_param,
          std::shared_ptr<DatasetService> dataset_service,
          void* ra_server, void* tee_engine);
  ~PsiTask() = default;
  int execute() override;
  retcode ExecuteTask(const std::vector<std::string>& input,
                      std::vector<std::string>* result);
  primihub::psi::Options& PsiOptions() {return options_;}
 protected:
  retcode LoadParams(const rpc::Task& task);
  retcode LoadDataset();
  retcode SaveResult();
  retcode InitOperator();
  retcode ExecuteOperator();
  retcode BuildOptions(const rpc::Task& task,
                       primihub::psi::Options* option);
  bool NeedSaveResult();
  bool IsClient();
  bool IsServer();
  bool IsTeeCompute();

 private:
  std::vector<int> data_index_;
  std::vector<std::string> data_colums_name_;
  int psi_type_{rpc::PsiTag::KKRT};
  std::string dataset_path_;
  std::string dataset_id_;
  std::string result_file_path_;
  std::vector<std::string> elements_;
  std::vector<std::string> result_;
  bool broadcast_result_{false};
  std::unique_ptr<BasePsiOperator> psi_operator_{nullptr};
  primihub::psi::Options options_;
  bool unique_values_{true};
  bool load_dataset_{true};
  // for TEE
  void* ra_server_{nullptr};
  void* tee_executor_{nullptr};
};
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_
