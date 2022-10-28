#ifndef SRC_PRIMIHUB_MISSINGPROCESS_H_
#define SRC_PRIMIHUB_MISSINGPROCESS_H_

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/executor/express.h"
#include "src/primihub/service/dataset/service.h"

namespace primihub {
class MissingProcess : public AlgorithmBase {
 public:
  explicit MissingProcess(PartyConfig &config,
                          std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int initPartyComm(void) override;
  int execute() override;
  int finishPartyComm(void) override;
  int saveModel(void);

 private:
  int _LoadDatasetFromCSV(std::string &filename);
  void _spiltStr(string str, const string &split, std::vector<string> &strlist);
  MPCOperator *mpc_op_exec_;
  std::pair<std::string, uint16_t> next_addr_;
  std::pair<std::string, uint16_t> prev_addr_;
  Session ep_next_;
  Session ep_prev_;
  IOService ios_;
  std::string next_ip_, prev_ip_;
  uint16_t next_port_, prev_port_;
  std::string data_file_path_;
  std::map<std::string, uint32_t> col_and_dtype_;
  uint32_t party_id_;
  std::vector<std::string> local_col_names;
  std::shared_ptr<arrow::Table> table;
};

}  // namespace primihub
#endif
