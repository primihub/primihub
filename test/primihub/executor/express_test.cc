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

class GLogHelper {
public:
  GLogHelper(char *program) {
    google::InitGoogleLogging(program);
    FLAGS_colorlogtostderr = true;
    google::InstallFailureSignalHandler();
  }
  ~GLogHelper() { google::ShutdownGoogleLogging(); }
};

std::string expr = "A+B+C";

static void importColumnOwner(MPCExpressExecutor *mpc_exec) {
  std::string col_name, col_owner;

  col_name = "A";
  col_owner = "node0";
  mpc_exec->importColumnOwner(col_name, col_owner);

  col_name = "B";
  col_owner = "node1";
  mpc_exec->importColumnOwner(col_name, col_owner);

  col_name = "C";
  col_owner = "node2";
  mpc_exec->importColumnOwner(col_name, col_owner);

  return;
}

static void importColumnDtype(MPCExpressExecutor *mpc_exec) {
  std::string col_name;

  col_name = "A";
  mpc_exec->importColumnDtype(col_name, true);

  col_name = "B";
  mpc_exec->importColumnDtype(col_name, true);

  col_name = "C";
  mpc_exec->importColumnDtype(col_name, true);
  return;
}

static void runFirstParty(std::vector<double> &col_val) {
  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  
  std::string node_id = "node0";
  mpc_exec->initColumnConfig(node_id);
  importColumnOwner(mpc_exec);
  importColumnDtype(mpc_exec);
  
  mpc_exec->importExpress(expr);
  mpc_exec->resolveRunMode();

  mpc_exec->InitFeedDict();
  std::string col_name = "A";
  mpc_exec->importColumnValues(col_name, col_val);

  std::vector<double> final_val;
  std::vector<uint8_t> parties = {0, 1};

  try {
    mpc_exec->initMPCRuntime(0, "127.0.0.1", 10090, 10091);
    mpc_exec->runMPCEvaluate();
    mpc_exec->revealMPCResult(parties, final_val);
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }

  delete mpc_exec;

  return;
}

static void runSecondParty(std::vector<double> &col_val) {
  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  
  std::string node_id = "node1";
  mpc_exec->initColumnConfig(node_id);
  importColumnOwner(mpc_exec);
  importColumnDtype(mpc_exec);

  mpc_exec->importExpress(expr);
  mpc_exec->resolveRunMode();

  mpc_exec->InitFeedDict();
  
  std::string col_name = "B";
  mpc_exec->importColumnValues(col_name, col_val);

  std::vector<double> final_val;
  std::vector<uint8_t> parties = {0, 1};

  try {
    mpc_exec->initMPCRuntime(1, "127.0.0.1", 10092, 10090);
    mpc_exec->runMPCEvaluate();
    mpc_exec->revealMPCResult(parties, final_val);
  } catch (const std::exception &e) {
    std::string msg = "In party 1, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }

  delete mpc_exec;

  return;
}

static void runThirdParty(std::vector<double> &col_val) {
  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  
  std::string node_id = "node2";
  mpc_exec->initColumnConfig(node_id);
  importColumnOwner(mpc_exec);
  importColumnDtype(mpc_exec);

  mpc_exec->importExpress(expr);
  mpc_exec->resolveRunMode();

  mpc_exec->InitFeedDict();

  std::string col_name = "C";
  mpc_exec->importColumnValues(col_name, col_val);

  std::vector<double> final_val;
  std::vector<uint8_t> parties = {0, 1};

  try {
    mpc_exec->initMPCRuntime(2, "127.0.0.1", 10091, 10092);
    mpc_exec->runMPCEvaluate();
    mpc_exec->revealMPCResult(parties, final_val);
  } catch (const std::exception &e) {
    std::string msg = "In party 2, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }

  delete mpc_exec;

  return;
}

TEST(mpc_express_executor, aby3_executor_test) {
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

  bool single_terminal = false;
  if (single_terminal) {
    pid_t pid = fork();
    if (pid != 0) {
      // This subprocess is party 1.
      GLogHelper gh("Party_1");
      runSecondParty(col_val_b);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // This subprocess is party 2.
      GLogHelper gh("Party_2");
      runThirdParty(col_val_c);
      return;
    }

    // The parent process is party 0.
    GLogHelper gh("Party_0");
    runFirstParty(col_val_a);

    // Wait for subprocess exit.
    int status;
    waitpid(-1, &status, 0);
  } else {
    if (std::string(std::getenv("MPC_PARTY")) == std::string("PARTY_0")) {
      GLogHelper gh("Party_0");
      runFirstParty(col_val_a);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_1")) {
      GLogHelper gh("Party_1");
      runSecondParty(col_val_b);
    } else if (std::string(std::getenv("MPC_PARTY")) ==
               std::string("PARTY_2")) {
      GLogHelper gh("Party_2");
      runThirdParty(col_val_c);
    }
  }
}
