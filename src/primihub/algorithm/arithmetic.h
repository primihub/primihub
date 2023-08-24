#include "src/primihub/algorithm/base.h"

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "src/primihub/data_store/driver.h"
#include "src/primihub/executor/express.h"
// #include "src/primihub/service/dataset/service.h"
#include "src/primihub/common/type.h"

namespace primihub {

template <Decimal Dbit>
class ArithmeticExecutor : public AlgorithmBase {
public:
  explicit ArithmeticExecutor(PartyConfig &config,
                              std::shared_ptr<DatasetService> dataset_service);
  int loadParams(primihub::rpc::Task &task) override;
  int loadDataset(void) override;
  int execute() override;
  int saveModel(void);
  retcode InitEngine() override;

private:
  int _LoadDatasetFromCSV(std::string &filename);

  std::string res_name_;
  std::string data_file_path_;
  std::string dataset_id_;
  bool is_dataset_detail_{false};

  // Reveal result to which party.
  std::vector<uint32_t> parties_;
  uint32_t party_id_;

  std::unique_ptr<MPCOperator> mpc_op_exec_;
  std::unique_ptr<MPCExpressExecutor<Dbit>> mpc_exec_;

  std::string task_id_;
  std::string job_id_;

  // For MPC compare task.
  bool is_cmp{false};
  std::vector<bool> cmp_res_;

  // For MPC express task.
  std::string expr_;
  std::map<std::string, u32> col_and_owner_;
  std::map<std::string, bool> col_and_dtype_;
  std::vector<double> final_val_double_;
  std::vector<int64_t> final_val_int64_;
  std::map<std::string, std::vector<double>> col_and_val_double;
  std::map<std::string, std::vector<int64_t>> col_and_val_int;
};

// #ifndef MPC_SOCKET_CHANNEL
// // This class just run send and recv many type of value with MPC channel.
// class MPCSendRecvExecutor : public AlgorithmBase {
// public:
//   explicit MPCSendRecvExecutor(PartyConfig &config,
//                                std::shared_ptr<DatasetService> dataset_service);
//   using TaskGetChannelFunc =
//       std::function<std::shared_ptr<network::IChannel>(primihub::Node &node)>;
//   using TaskGetRecvQueueFunc =
//       std::function<ThreadSafeQueue<std::string> &(const std::string &key)>;

//   int loadParams(rpc::Task &task) override;
//   int loadDataset(void) override;
//   int initPartyComm(void) override;
//   int execute() override;
//   int finishPartyComm(void) override;
//   int saveModel(void);

// private:
//   std::string job_id_;
//   std::string task_id_;

//   std::unique_ptr<MPCOperator> mpc_op_;

//   // std::shared_ptr<network::IChannel> base_channel_next_;
//   // std::shared_ptr<network::IChannel> base_channel_prev_;

//   // std::shared_ptr<MpcChannel> mpc_channel_next_;
//   // std::shared_ptr<MpcChannel> mpc_channel_prev_;

//   // std::map<uint16_t, primihub::Node> partyid_node_map_;

//   // uint16_t local_party_id_;
//   // uint16_t next_party_id_;
//   // uint16_t prev_party_id_;

//   // primihub::Node local_node_;
// };
// #endif

} // namespace primihub
