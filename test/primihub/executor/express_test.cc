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

std::string expr = "A*B-C+A";

static void
importColumnOwner(MPCExpressExecutor *mpc_exec,
                  std::map<std::string, std::string> &col_and_owner) {
  for (auto &pair : col_and_owner)
    mpc_exec->importColumnOwner(pair.first, pair.second);
  return;
}

static void importColumnDtype(MPCExpressExecutor *mpc_exec,
                              std::map<std::string, bool> &col_and_dtype) {
  for (auto &pair : col_and_dtype)
    mpc_exec->importColumnDtype(pair.first, pair.second);
  return;
}

template <typename T>
static void
importColumnValues(MPCExpressExecutor *mpc_exec,
                   std::map<std::string, std::vector<T>> &col_and_val) {
  for (auto &pair : col_and_val)
    mpc_exec->importColumnValues(pair.first, pair.second);
  return;
}

template <typename T>
static void runParty(std::map<std::string, std::vector<T>> &col_and_val,
                     std::map<std::string, bool> &col_and_dtype,
                     std::map<std::string, std::string> &col_and_owner,
                     std::string node_id, uint8_t party_id, std::string ip,
                     uint16_t next_port, uint16_t prev_port) {
  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();

  mpc_exec->initColumnConfig(node_id);
  importColumnOwner(mpc_exec, col_and_owner);
  importColumnDtype(mpc_exec, col_and_dtype);

  mpc_exec->importExpress(expr);
  mpc_exec->resolveRunMode();

  mpc_exec->InitFeedDict();
  importColumnValues(mpc_exec, col_and_val);

  std::vector<double> final_val;
  std::vector<uint8_t> parties = {0, 1};

  try {
    mpc_exec->initMPCRuntime(party_id, ip, next_port, prev_port);
    ASSERT_EQ(mpc_exec->runMPCEvaluate(), 0);
    mpc_exec->revealMPCResult(parties, final_val);
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }

  delete mpc_exec;

  return;
}

TEST(mpc_express_executor, fp64_executor_test) {
  std::vector<double> col_val_a;
  std::vector<double> col_val_b;
  std::vector<double> col_val_c;

  srand(time(nullptr));
  for (int i = 0; i < 100; i++)
    col_val_a.emplace_back(rand() % 10000);

  for (int i = 0; i < 100; i++)
    col_val_b.emplace_back(rand() % 10000);

  for (int i = 0; i < 100; i++)
    col_val_c.emplace_back(rand() % 10000);

  std::map<std::string, std::string> col_and_owner;
  col_and_owner.insert(std::make_pair("A", "node0"));
  col_and_owner.insert(std::make_pair("B", "node0"));
  col_and_owner.insert(std::make_pair("C", "node1"));
  col_and_owner.insert(std::make_pair("D", "node2"));

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
      runParty(col_and_val_1, col_and_dtype, col_and_owner, "node1", 1,
               "127.0.0.1", 10030, 10010);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // This subprocess is party 2.
      runParty(col_and_val_2, col_and_dtype, col_and_owner, "node2", 2,
               "127.0.0.1", 10020, 10030);
      return;
    }

    // The parent process is party 0.
    runParty(col_and_val_0, col_and_dtype, col_and_owner, "node0", 0,
             "127.0.0.1", 10010, 10020);

    // Wait for subprocess exit.
    int status;
    waitpid(-1, &status, 0);
  } else {
    if (std::string(std::getenv("MPC_PARTY")) == std::string("PARTY_0")) {
      runParty(col_and_val_0, col_and_dtype, col_and_owner, "node0", 0,
               "127.0.0.1", 10010, 10020);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_1")) {
      runParty(col_and_val_1, col_and_dtype, col_and_owner, "node1", 1,
               "127.0.0.1", 10030, 10010);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_2")) {
      runParty(col_and_val_2, col_and_dtype, col_and_owner, "node2", 2,
               "127.0.0.1", 10020, 10030);
    }
  }
}

TEST(mpc_express_executor, i64_executor_test) {
  std::vector<int64_t> col_val_a;
  std::vector<int64_t> col_val_b;
  std::vector<int64_t> col_val_c;

  srand(time(nullptr));
  for (int i = 0; i < 100; i++)
    col_val_a.emplace_back(rand() % 10000);

  for (int i = 0; i < 100; i++)
    col_val_b.emplace_back(rand() % 10000);

  for (int i = 0; i < 100; i++)
    col_val_c.emplace_back(rand() % 10000);

  std::map<std::string, std::string> col_and_owner;
  col_and_owner.insert(std::make_pair("A", "node0"));
  col_and_owner.insert(std::make_pair("B", "node0"));
  col_and_owner.insert(std::make_pair("C", "node1"));
  col_and_owner.insert(std::make_pair("D", "node2"));

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
      runParty(col_and_val_1, col_and_dtype, col_and_owner, "node1", 1,
               "127.0.0.1", 10030, 10010);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // This subprocess is party 2.
      runParty(col_and_val_2, col_and_dtype, col_and_owner, "node2", 2,
               "127.0.0.1", 10020, 10030);
      return;
    }

    // The parent process is party 0.
    runParty(col_and_val_0, col_and_dtype, col_and_owner, "node0", 0,
             "127.0.0.1", 10010, 10020);

    // Wait for subprocess exit.
    int status;
    waitpid(-1, &status, 0);
  } else {
    if (std::string(std::getenv("MPC_PARTY")) == std::string("PARTY_0")) {
      runParty(col_and_val_0, col_and_dtype, col_and_owner, "node0", 0,
               "127.0.0.1", 10010, 10020);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_1")) {
      runParty(col_and_val_1, col_and_dtype, col_and_owner, "node1", 1,
               "127.0.0.1", 10030, 10010);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_2")) {
      runParty(col_and_val_2, col_and_dtype, col_and_owner, "node2", 2,
               "127.0.0.1", 10020, 10030);
    }
  }
}
