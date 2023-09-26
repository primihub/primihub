/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
#define SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/type.h"
#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/executor/statistics.h"

using primihub::ColumnDtype;
using MPCStatisticsType = primihub::MPCStatisticsOperator::MPCStatisticsType;

namespace primihub {
class MPCStatisticsExecutor : public AlgorithmBase {
 public:
  explicit MPCStatisticsExecutor(
      PartyConfig &config, std::shared_ptr<DatasetService> dataset_service);

  ~MPCStatisticsExecutor() {}

  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset() override;
  int execute() override;
  retcode execute(const eMatrix<double>& input_data_info,
                  const std::vector<std::string>& col_names,
                  std::vector<double>* result) override;
  retcode InitEngine() override;
  int saveModel() override;

 private:
  retcode _parseColumnName(const std::string &json_str);
  retcode _parseColumnDtype(const std::string &json_str);

  bool do_nothing_ = false;

  eMatrix<double> result_;
  std::vector<std::string> target_columns_;

  std::shared_ptr<primihub::Dataset> input_value_;


  std::string new_ds_id_;
  std::string output_path_;
  std::string ds_name_;
  std::string dataset_id_;
  bool is_dataset_detail_{false};
  std::string job_id_;
  std::string task_id_;
  std::string statistics_type_;
  std::map<std::string, ColumnDtype> col_type_;

  MPCStatisticsType type_{MPCStatisticsType::UNKNOWN};
  std::unique_ptr<MPCStatisticsOperator> executor_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
