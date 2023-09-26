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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_MPC_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_MPC_TASK_H_

#include <map>
#include <memory>
#include <string>

#include "src/primihub/algorithm/base.h"
#include "src/primihub/task/semantic/task.h"

using primihub::AlgorithmBase;

namespace primihub::task {

class MPCTask : public TaskBase {
 public:
  MPCTask(const std::string &node_id,
          const std::string &function_name,
          const TaskParam *task_param,
          std::shared_ptr<DatasetService> dataset_service);
  MPCTask(const std::string& funcaton_name, const TaskParam *task_param);
  ~MPCTask() = default;
  int execute() override;
  retcode ExecuteTask(const std::vector<double>& input_data,
                      const std::vector<int64_t>& col_rows,
                      std::vector<double>* result);
  retcode ExecuteImpl();

 protected:
  std::shared_ptr<Dataset> MakeDataset(const std::vector<double>& input_data,
                                       int64_t colum_num);
  std::shared_ptr<Dataset> MakeDataset();
  retcode MakeAuxiliaryComputeData(const std::vector<int64_t>& shape,
                                   std::vector<double>* input,
                                   std::vector<int64_t>* col_rows);
  retcode RecvShapeFromLauncher(std::vector<int64_t>* shape);

 private:
    std::shared_ptr<AlgorithmBase> algorithm_{nullptr};
};

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_MPC_TASK_H_
