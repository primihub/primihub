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

#ifndef SRC_PRIMIHUB_ALGORITHM_CRYPTFLOW2_MAXPOOL_H_
#define SRC_PRIMIHUB_ALGORITHM_CRYPTFLOW2_MAXPOOL_H_

#include "Eigen/Dense"

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/clp.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/protocol/cryptflow2/NonLinear/maxpool.h"
#include "src/primihub/protocol/cryptflow2/globals.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/common/clp.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"

#include <fstream>
#include <thread>

using namespace std;
using namespace primihub::sci;

#define MAX_THREADS 4

namespace primihub::cryptflow2
{
  class MaxPoolExecutor : public AlgorithmBase
  {
  public:
    explicit MaxPoolExecutor(PartyConfig &config,
                             std::shared_ptr<DatasetService> dataset_service);
    int loadParams(primihub::rpc::Task &task) override;
    int loadDataset(void) override;
    int initPartyComm(void) override;
    int execute() override;
    int finishPartyComm(void) override;
    int saveModel(void);

  private:
    int num_rows = 35;     // Row num of maxpool
    int num_cols = 1 << 6; // Col num of maxpool
    int b = 4;             // Radix Base
    int batch_size = 0;    // Batch size

    string node_id;         // node id
    string input_filepath_; // input data file directory(path)
    uint64_t mask_l;        // mask
    uint64_t *x;            // input
    uint64_t *z;            // output

    IOPack *iopackArr[MAX_THREADS];
    OTPack *otpackArr[MAX_THREADS];
    void ring_maxpool_thread(int, uint64_t *, uint64_t *, int, int);

    int __party = 0;                // __party ID
    int __bitlength = 32;           // __bitlength of input
    int __num_threads = 1;          // thread_number
    string __address = "127.0.0.1"; // network __address
    int __port = 32000;             // network ports
  };
} // namespace primihub

#endif
