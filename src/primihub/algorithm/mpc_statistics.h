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
  std::string job_id_;
  std::string task_id_;
  std::string statistics_type_;
  std::map<std::string, ColumnDtype> col_type_;

  MPCStatisticsType type_;
  std::unique_ptr<MPCStatisticsOperator> executor_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
