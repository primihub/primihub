//  "Copyright [2023] <Primihub>"
#ifndef SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
#define SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <map>

#include "src/primihub/algorithm/base.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/executor/statistics.h"
#include "src/primihub/operator/aby3_operator.h"

#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

using primihub::ColumnDtype;
using MPCStatisticsType = primihub::MPCStatisticsOperator::MPCStatisticsType;

namespace primihub {
class MPCStatisticsExecutor : public AlgorithmBase {
 public:
  explicit MPCStatisticsExecutor(
      PartyConfig &config, std::shared_ptr<DatasetService> dataset_service);

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
  std::unique_ptr<MPCStatisticsOperator> executor_;
  MPCStatisticsType type_;

  std::shared_ptr<primihub::Dataset> input_value_;
  std::string new_ds_id_;
  std::string output_path_;
  std::string ds_name_;
  std::string statistics_type_;
  std::map<std::string, ColumnDtype> col_type_;
  // std::shared_ptr<MpcChannel> channel_1;
  // std::shared_ptr<MpcChannel> channel_2;


  std::string job_id_;
  std::string task_id_;
  std::string request_id_;

  Node local_node_;
  std::string node_id_;
  uint16_t party_id_;
  std::map<uint16_t, Node> node_map_;

  // communication session
  Session ep_next_;
  Session ep_prev_;
  IOService ios_;
  std::pair<std::string, uint16_t> next_addr_;
  std::pair<std::string, uint16_t> prev_addr_;
};
};  // namespace primihub
#endif  // SRC_PRIMIHUB_ALGORITHM_MPC_STATISTICS_H_
