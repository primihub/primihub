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

#ifndef SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_
#define SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_

#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <memory>

#include "Eigen/Dense"
#include "src/primihub/algorithm/aby3ML.h"
#include "src/primihub/algorithm/base.h"
#include "src/primihub/algorithm/linear_model_gen.h"
#include "src/primihub/algorithm/plainML.h"
#include "src/primihub/algorithm/regression.h"
#include "src/primihub/data_store/driver.h"
#include "cryptoTools/Common/Defines.h"
#include "aby3/sh3/Sh3FixedPoint.h"

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
using Session = oc::Session;
using IOService = oc::IOService;
using SessionMode = oc::SessionMode;
#endif
using Decimal = aby3::Decimal;
const Decimal D = Decimal::D20;
eMatrix<double> logistic_main(sf64Matrix<D> &train_data_0_1,
                              sf64Matrix<D> &train_label_0_1,
                              sf64Matrix<D> &W2_0_1,
                              sf64Matrix<D> &test_data_0_1,
                              sf64Matrix<D> &test_label_0_1, aby3ML &p, int B,
                              int IT, int pIdx);

class LogisticRegressionExecutor : public AlgorithmBase {
 public:
  explicit LogisticRegressionExecutor(
      PartyConfig &config, std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int execute() override;

  int constructShares(void);
  int saveModel(void);
  retcode InitEngine() override;

 protected:
  retcode ParseExcludeColumns(primihub::rpc::Task &task_config);

 private:
  int _ConstructShares(sf64Matrix<D> &w, sf64Matrix<D> &train_data,
                       sf64Matrix<D> &train_label, sf64Matrix<D> &test_data,
                       sf64Matrix<D> &test_label);

  int _LoadDataset(const std::string& filename);
  uint16_t NextPartyId() {return (local_id_ + 1) % 3;}
  uint16_t PrevPartyId() {return (local_id_ + 2) % 3;}

  std::string model_file_name_;
  std::string model_name_;
  uint16_t local_id_;
  eMatrix<double> train_input_;
  eMatrix<double> test_input_;
  eMatrix<double> model_;
  aby3ML engine_;

  // Logistic regression parameters
  std::string train_input_filepath_;
  std::string test_input_filepath_;
  std::string train_dataset_id_;
  bool is_dataset_detail_{false};
  std::vector<std::string> columns_exclude_;
  int batch_size_;
  int num_iter_;
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_
