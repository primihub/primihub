#ifndef SRC_PRIMIHUB_ALGORITHM_FALCON_LENET_H
#define SRC_PRIMIHUB_ALGORITHM_FALCON_LENET_H

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

#include "src/primihub/algorithm/base.h"
#include "src/primihub/protocol/falcon-public/AESObject.h"
#include "src/primihub/protocol/falcon-public/NeuralNetConfig.h"
#include "src/primihub/protocol/falcon-public/NeuralNetwork.h"
#include "src/primihub/protocol/falcon-public/Precompute.h"
#include "src/primihub/protocol/falcon-public/connect.h"
#include "src/primihub/protocol/falcon-public/secondary.h"
#include "src/primihub/protocol/falcon-public/tools.h"

#include "Eigen/Dense"
#include "assert.h"
#include "src/primihub/common/clp.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {
namespace falcon {
class FalconLenetExecutor : public AlgorithmBase {
public:
  explicit FalconLenetExecutor(PartyConfig &config,
                               std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int initPartyComm(void) override;
  int execute() override;
  int finishPartyComm(void) override;
  int constructShares(void);
  int saveModel(void) { return 0; }

private:
  std::string model_name_;
  uint16_t local_id_;
  std::string node_id_;
  std::vector<std::pair<std::string, uint16_t>> listen_addrs_;
  std::vector<std::pair<std::string, uint16_t>> connect_addrs_;
  int batch_size_, num_iter_;
  NeuralNetConfig *config_lenet;
  NeuralNetwork *net_lenet;
  std::string Test_Input_Self_path, Test_Input_Next_path;
};

} // namespace falcon
} // namespace primihub

#endif // SRC_primihub_ALGORITHM_LOGISTIC_H_
