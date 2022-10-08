#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/executor/express.h"
#include "src/primihub/service/dataset/service.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class ArithmeticExecutor : public AlgorithmBase {
public:
  explicit ArithmeticExecutor(PartyConfig &config,
                              std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int initPartyComm(void) override;
  int execute() override;
  int finishPartyComm(void) override;
  int saveModel(void);

private:
  // int _ConstructShares(sf64Matrix<D> &w, sf64Matrix<D> &train_data,
  //                      sf64Matrix<D> &train_label, sf64Matrix<D>
  //                      &test_data, sf64Matrix<D> &test_label);

  int _LoadDatasetFromCSV(std::string &filename);
  MPCExpressExecutor *mpc_exec_;
  std::string res_name_;
  uint16_t local_id_;
  std::pair<std::string, uint16_t> next_addr_;
  std::pair<std::string, uint16_t> prev_addr_;
  Session ep_next_;
  Session ep_prev_;
  IOService ios_;
  std::string next_ip_, prev_ip_;
  uint16_t next_port_, prev_port_;
  std::string data_file_path_;
  std::map<std::string, u32> col_and_owner_;
  std::map<std::string, bool> col_and_dtype_;
  std::vector<uint32_t> parties_;
  uint32_t party_id_;
  std::vector<double> final_val_double_;
  std::vector<int64_t> final_val_int64_;
  std::string expr_;
  std::map<std::string, std::vector<double>> col_and_val_double;
  std::map<std::string, std::vector<int64_t>> col_and_val_int;
};

} // namespace primihub