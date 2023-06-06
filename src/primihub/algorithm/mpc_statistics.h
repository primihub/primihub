#ifndef __MPC_STATISTICS_H__
#define __MPC_STATISTICS_H__

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/executor/statistics.h"
#include <string>

#ifndef MPC_SOCKET_CHANNEL
using primihub::ColumnDtype;
using MPCStatisticsType = primihub::MPCStatisticsOperator::MPCStatisticsType;
#endif

namespace primihub {
class MPCStatisticsExecutor : public AlgorithmBase {
public:
  explicit MPCStatisticsExecutor(
      PartyConfig &config, std::shared_ptr<DatasetService> dataset_service);

  ~MPCStatisticsExecutor() {}

  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset() override;
  int initPartyComm() override;
  int execute() override;
  int finishPartyComm() override;
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

  Node local_node_;
  std::string node_id_;
  uint16_t party_id_;
  std::map<uint16_t, Node> node_map_;
#ifndef MPC_SOCKET_CHANNEL
  MPCStatisticsType type_;
  std::unique_ptr<MPCStatisticsOperator> executor_;
  std::shared_ptr<MpcChannel> channel_1;
  std::shared_ptr<MpcChannel> channel_2;
#endif
  ABY3PartyConfig party_config_;
};
}; // namespace primihub
#endif
