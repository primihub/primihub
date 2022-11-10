#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "src/primihub/executor/express.h"
#include "gtest/gtest.h"

using namespace primihub;

// std::string expr = "A+B-2";
// std::string expr = "A+B-2";
// std::string expr = "A*B-C+A+2.2";
// std::string expr = "(A*B-C+A+2)";
// std::string expr = "A*B-C+A+2";
// std::string expr = "(5.3-(A*B-C+A+2.2))*2.0";
// std::string expr = "1/B";

// std::string expr = "(5-(A*B-C+A+2-1)*2)/2";
std::string expr = "-100/A";
// std::string expr = "-100/B";
// std::string expr = "-100/(5-(A*B-C+A+2-1)*2)/2";
// std::string expr = "A*B-C+A*2.2";
// std::string expr = "A*B-C+A*2";
f64<D32 > bit_flag = 1.0;
template <Decimal Dbit>
static void importColumnOwner(MPCExpressExecutor<Dbit> *mpc_exec,
                              std::map<std::string, u32> &col_and_owner) {
  for (auto &pair : col_and_owner)
    mpc_exec->importColumnOwner(pair.first, pair.second);
  return;
}

template <Decimal Dbit>
static void importColumnDtype(MPCExpressExecutor<Dbit> *mpc_exec,
                              std::map<std::string, bool> &col_and_dtype) {
  for (auto &pair : col_and_dtype)
    mpc_exec->importColumnDtype(pair.first, pair.second);
  return;
}

template <typename T, Decimal Dbit>
static void
importColumnValues(MPCExpressExecutor<Dbit> *mpc_exec,
                   std::map<std::string, std::vector<T>> &col_and_val) {
  for (auto &pair : col_and_val)
    mpc_exec->importColumnValues(pair.first, pair.second);
  return;
}

template <typename T, Decimal Dbit>
static MPCExpressExecutor<Dbit> *
runParty(std::map<std::string, std::vector<T>> &col_and_val,
         std::map<std::string, bool> &col_and_dtype,
         std::map<std::string, u32> &col_and_owner, u32 party_id,
         std::string next_ip, std::string prev_ip, uint16_t next_port,
         uint16_t prev_port, f64<Dbit> bit_flag) {
  MPCExpressExecutor<Dbit> *mpc_exec = new MPCExpressExecutor<Dbit>();

  mpc_exec->initColumnConfig(party_id);
  importColumnOwner(mpc_exec, col_and_owner);
  importColumnDtype(mpc_exec, col_and_dtype);

  mpc_exec->importExpress(expr);
  mpc_exec->resolveRunMode();

  mpc_exec->InitFeedDict();
  importColumnValues(mpc_exec, col_and_val);

  // std::vector<double> final_val;
  std::vector<T> final_val;
  std::vector<uint32_t> parties = {0, 1};

  try {
    mpc_exec->initMPCRuntime(party_id, next_ip, prev_ip, next_port, prev_port);
    // ASSERT_EQ(mpc_exec->runMPCEvaluate(), 0);
    mpc_exec->runMPCEvaluate();
    mpc_exec->revealMPCResult(parties, final_val);
    for (auto itr = final_val.begin(); itr != final_val.end(); itr++)
      LOG(INFO) << *itr;
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }

  // delete mpc_exec;

  return mpc_exec;
}

TEST(mpc_express_executor, fp64_executor_test) {
  LOG(INFO)<<"TEST!!!!!!!!!!!!!!!";
  std::vector<double> col_val_a;
  std::vector<double> col_val_b;
  std::vector<double> col_val_c;

  srand(time(nullptr));
  for (int i = 0; i < 2; i++)
    // col_val_a.emplace_back(-i - 3.0);
    col_val_a.emplace_back(i + 3.0);
  // col_val_a.emplace_back(rand() % 10000);

  for (int i = 0; i < 2; i++)
    // col_val_b.emplace_back(i + 2.0);
    col_val_b.emplace_back(-i - 2.0);
  // col_val_b.emplace_back(rand() % 10000);

  for (int i = 0; i < 2; i++)
    col_val_c.emplace_back(i + 1.0);
  // col_val_c.emplace_back(rand() % 10000);

  std::map<std::string, u32> col_and_owner;
  col_and_owner.insert(std::make_pair("A", 0));
  col_and_owner.insert(std::make_pair("B", 0));
  col_and_owner.insert(std::make_pair("C", 1));
  col_and_owner.insert(std::make_pair("D", 2));

  std::map<std::string, bool> col_and_dtype;
  col_and_dtype.insert(std::make_pair("A", false));
  col_and_dtype.insert(std::make_pair("B", true));
  col_and_dtype.insert(std::make_pair("C", false));
  col_and_dtype.insert(std::make_pair("D", true));

  std::map<std::string, std::vector<double>> col_and_val_0;
  std::map<std::string, std::vector<double>> col_and_val_1;
  std::map<std::string, std::vector<double>> col_and_val_2;

  col_and_val_0.insert(std::make_pair("A", col_val_a));
  col_and_val_0.insert(std::make_pair("B", col_val_b));
  col_and_val_1.insert(std::make_pair("C", col_val_c));
  col_and_val_2.insert(std::make_pair("D", col_val_c));

  bool single_terminal = false;
  if (single_terminal) {
    pid_t pid = fork();
    if (pid != 0) {
      // This subprocess is party 1.
      runParty(col_and_val_1, col_and_dtype, col_and_owner, 1, "127.0.0.1",
               "127.0.0.1", 10030, 10010, bit_flag);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // This subprocess is party 2.
      runParty(col_and_val_2, col_and_dtype, col_and_owner, 2, "127.0.0.1",
               "127.0.0.1", 10020, 10030, bit_flag);
      return;
    }

    // The parent process is party 0.
    runParty(col_and_val_0, col_and_dtype, col_and_owner, 0, "127.0.0.1",
             "127.0.0.1", 10010, 10020, bit_flag);

    // Wait for subprocess exit.
    int status;
    waitpid(-1, &status, 0);
  } else {
    if (std::string(std::getenv("MPC_PARTY")) == "PARTY_0") {
      MPCExpressExecutor<D32 > *mpc_exec =
          runParty(col_and_val_0, col_and_dtype, col_and_owner, 0, "127.0.0.1",
                   "127.0.0.1", 10010, 10020, bit_flag);

      std::map<std::string, std::vector<double>> col_and_val_n;
      col_and_val_n.insert(std::make_pair("A", col_val_a));
      col_and_val_n.insert(std::make_pair("B", col_val_b));
      col_and_val_n.insert(std::make_pair("C", col_val_c));

      LocalExpressExecutor<D32 > *local_exec =
          new LocalExpressExecutor<D32 >(mpc_exec);
      local_exec->init(col_and_val_n);
      local_exec->runLocalEvaluate();
      std::vector<double> final_val;
      local_exec->getFinalVal<double>(final_val);
      for (auto itr = final_val.begin(); itr != final_val.end(); itr++)
        LOG(INFO) << *itr;
      delete local_exec;
    } else if (std::string(std::getenv("MPC_PARTY")) == "PARTY_1") {
      MPCExpressExecutor<D32 > *mpc_exec =
          runParty(col_and_val_1, col_and_dtype, col_and_owner, 1, "127.0.0.1",
                   "127.0.0.1", 10030, 10010, bit_flag);
      delete mpc_exec;
    } else if (std::string(std::getenv("MPC_PARTY")) == "PARTY_2") {
      MPCExpressExecutor<D32 > *mpc_exec =
          runParty(col_and_val_2, col_and_dtype, col_and_owner, 2, "127.0.0.1",
                   "127.0.0.1", 10020, 10030, bit_flag);
      delete mpc_exec;
    }
  }
}

TEST(mpc_express_executor, i64_executor_test) {
  std::vector<int64_t> col_val_a;
  std::vector<int64_t> col_val_b;
  std::vector<int64_t> col_val_c;

  srand(time(nullptr));
  for (int i = 0; i < 1; i++)
    col_val_a.emplace_back(i + 3);
  // col_val_a.emplace_back(rand() % 10000);

  for (int i = 0; i < 1; i++)
    col_val_b.emplace_back(i + 2);
  // col_val_b.emplace_back(rand() % 10000);

  for (int i = 0; i < 1; i++)
    col_val_c.emplace_back(i + 1);
  // col_val_c.emplace_back(rand() % 10000);

  std::map<std::string, u32> col_and_owner;
  col_and_owner.insert(std::make_pair("A", 0));
  col_and_owner.insert(std::make_pair("B", 0));
  col_and_owner.insert(std::make_pair("C", 1));
  col_and_owner.insert(std::make_pair("D", 2));

  std::map<std::string, bool> col_and_dtype;
  col_and_dtype.insert(std::make_pair("A", false));
  col_and_dtype.insert(std::make_pair("B", false));
  col_and_dtype.insert(std::make_pair("C", false));
  col_and_dtype.insert(std::make_pair("D", false));

  std::map<std::string, std::vector<int64_t>> col_and_val_0;
  std::map<std::string, std::vector<int64_t>> col_and_val_1;
  std::map<std::string, std::vector<int64_t>> col_and_val_2;

  col_and_val_0.insert(std::make_pair("A", col_val_a));
  col_and_val_0.insert(std::make_pair("B", col_val_b));
  col_and_val_1.insert(std::make_pair("C", col_val_c));
  col_and_val_2.insert(std::make_pair("D", col_val_c));

  bool single_terminal = false;
  if (single_terminal) {
    pid_t pid = fork();
    if (pid != 0) {
      // This subprocess is party 1.
      runParty(col_and_val_1, col_and_dtype, col_and_owner, 1, "127.0.0.1",
               "127.0.0.1", 10030, 10010, bit_flag);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // This subprocess is party 2.
      runParty(col_and_val_2, col_and_dtype, col_and_owner, 2, "127.0.0.1",
               "127.0.0.1", 10020, 10030, bit_flag);
      return;
    }

    // The parent process is party 0.
    runParty(col_and_val_0, col_and_dtype, col_and_owner, 0, "127.0.0.1",
             "127.0.0.1", 10010, 10020, bit_flag);

    // Wait for subprocess exit.
    int status;
    waitpid(-1, &status, 0);
  } else {
    if (std::string(std::getenv("MPC_PARTY")) == std::string("PARTY_0")) {
      MPCExpressExecutor<D32 > *mpc_exec =
          runParty(col_and_val_0, col_and_dtype, col_and_owner, 0, "127.0.0.1",
                   "127.0.0.1", 10010, 10020, bit_flag);

      std::map<std::string, std::vector<int64_t>> col_and_val_n;

      col_and_val_n.insert(std::make_pair("A", col_val_a));
      col_and_val_n.insert(std::make_pair("B", col_val_b));
      col_and_val_n.insert(std::make_pair("C", col_val_c));

      LocalExpressExecutor<D32 > *local_exec =
          new LocalExpressExecutor<D32 >(mpc_exec);
      local_exec->init(col_and_val_n);
      // local_exec->creatNewFeedDict();
      // local_exec->importColumnValues(col_and_val_n);
      local_exec->runLocalEvaluate();
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_1")) {
      runParty(col_and_val_1, col_and_dtype, col_and_owner, 1, "127.0.0.1",
               "127.0.0.1", 10030, 10010, bit_flag);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_2")) {
      runParty(col_and_val_2, col_and_dtype, col_and_owner, 2, "127.0.0.1",
               "127.0.0.1", 10020, 10030, bit_flag);
    }
  }
}
