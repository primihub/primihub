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

#ifndef SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_
#define SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

#include "Eigen/Dense"

#include "src/primihub/algorithm/aby3ML.h"
#include "src/primihub/algorithm/base.h"
#include "src/primihub/algorithm/linear_model_gen.h"
#include "src/primihub/algorithm/plainML.h"
#include "src/primihub/algorithm/regression.h"
#include "src/primihub/common/clp.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {
eMatrix<double>
logistic_main(sf64Matrix<D> &train_data_0_1, sf64Matrix<D> &train_label_0_1,
              sf64Matrix<D> &W2_0_1, sf64Matrix<D> &test_data_0_1,
              sf64Matrix<D> &test_label_0_1, aby3ML &p, int B, int IT, int pIdx,
              bool print, Session &chlPrev, Session &chlNext);

class LogisticRegressionExecutor : public AlgorithmBase {
public:
  explicit LogisticRegressionExecutor(
      PartyConfig &config, std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int initPartyComm(void) override;
  int execute() override;
  int finishPartyComm(void) override;

  int constructShares(void);
  int saveModel(void);

private:
  int _ConstructShares(sf64Matrix<D> &w, sf64Matrix<D> &train_data,
                       sf64Matrix<D> &train_label, sf64Matrix<D> &test_data,
                       sf64Matrix<D> &test_label);

  int _LoadDatasetFromCSV(std::string &filename);

  std::string model_file_name_;
  std::string model_name_;
  uint16_t local_id_;
  eMatrix<double> train_input_;
  eMatrix<double> test_input_;
  eMatrix<double> model_;
  std::pair<std::string, uint16_t> next_addr_;
  std::pair<std::string, uint16_t> prev_addr_;
  aby3ML engine_;
  Session ep_next_;
  Session ep_prev_;
  IOService ios_;

  // Logistic regression parameters
  std::string train_input_filepath_, test_input_filepath_;
  int batch_size_, num_iter_;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_ALGORITHM_LOGISTIC_H_
