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
#include "src/primihub/util/log_wrapper.h"

namespace primihub {
#define LOG_INFO() LOG_INFO_WRAPPER(platform(), job_id(), task_id())
#define LOG_WARNING() LOG_WARNING_WRAPPER(platform(), job_id(), task_id())
#define LOG_ERROR() LOG_ERROR_WRAPPER(platform(), job_id(), task_id())

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
  int set_task_info(std::string platform_type, std::string job_id,
                    std::string task_id);
  inline std::string platform() { return platform_type_; }
  inline std::string job_id() { return job_id_; }
  inline std::string task_id() { return task_id_; }

private:
  using NestedVectorI32 = std::vector<std::vector<uint32_t>>;

  inline int _strToInt64(const std::string &str, int64_t &i64_val);
  inline int _strToDouble(const std::string &str, double &d_val);
  inline int _avoidStringArray(std::shared_ptr<arrow::Array> array);
  inline void _buildNewColumn(std::shared_ptr<arrow::Table> table,
                              int col_index, const std::string &replace,
                              NestedVectorI32 &abnormal_index,
                              std::shared_ptr<arrow::Array> &new_array);

  int _LoadDatasetFromCSV(std::string &filename);
  void _spiltStr(string str, const string &split, std::vector<string> &strlist);

  MPCOperator *mpc_op_exec_;

  IOService ios_;
  Session ep_next_;
  Session ep_prev_;
  std::string next_ip_, prev_ip_;
  uint16_t next_port_, prev_port_;

  std::map<std::string, uint32_t> col_and_dtype_;
  std::vector<std::string> local_col_names;

  std::string data_file_path_;
  std::shared_ptr<arrow::Table> table;

  std::string node_id_;
  uint32_t party_id_;

  std::string new_dataset_id_;
  std::string platform_type_ = "";
  std::string job_id_ = "";
  std::string task_id_ = "";
};

} // namespace primihub
#endif
