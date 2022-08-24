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

static void importColumnOwner(ColumnConfig *col_config) {
  col_config->importColumnOwner("A", "node0");
  col_config->importColumnOwner("B", "node1");
  col_config->importColumnOwner("C", "node2");
  return;
}

static void importColumnDtype(ColumnConfig *col_config) {
  col_config->importColumnDtype("A", ColumnConfig::ColDtype::FP64);
  col_config->importColumnDtype("B", ColumnConfig::ColDtype::FP64);
  col_config->importColumnDtype("C", ColumnConfig::ColDtype::FP64);
  return;
}

static void runFirstParty(std::vector<double> &col_val) {
  ColumnConfig *col_config = new ColumnConfig("node0");
  importColumnOwner(col_config);
  importColumnDtype(col_config);
  col_config->resolveLocalColumn();

  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  mpc_exec->importExpress("A+B+C");
  mpc_exec->setColumnConfig(col_config);
  mpc_exec->resolveRunMode();

  FeedDict *feed_dict = new FeedDict(col_config, mpc_exec->isFP64RunMode());
  feed_dict->importColumnValues("A", col_val);
  mpc_exec->setFeedDict(feed_dict);

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

  delete col_config;
  delete feed_dict;
  delete mpc_exec;

  return;
}

static void runSecondParty(std::vector<double> &col_val) {
  ColumnConfig *col_config = new ColumnConfig("node1");
  importColumnOwner(col_config);
  importColumnDtype(col_config);
  col_config->resolveLocalColumn();

  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  mpc_exec->importExpress("A+B+C");
  mpc_exec->setColumnConfig(col_config);
  mpc_exec->resolveRunMode();

  FeedDict *feed_dict = new FeedDict(col_config, mpc_exec->isFP64RunMode());
  feed_dict->importColumnValues("B", col_val);
  mpc_exec->setFeedDict(feed_dict);

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

  delete col_config;
  delete feed_dict;
  delete mpc_exec;

  return;
}

static void runThirdParty(std::vector<double> &col_val) {
  ColumnConfig *col_config = new ColumnConfig("node2");
  importColumnOwner(col_config);
  importColumnDtype(col_config);
  col_config->resolveLocalColumn();

  MPCExpressExecutor *mpc_exec = new MPCExpressExecutor();
  mpc_exec->importExpress("A+B+C");
  mpc_exec->setColumnConfig(col_config);
  mpc_exec->resolveRunMode();

  FeedDict *feed_dict = new FeedDict(col_config, mpc_exec->isFP64RunMode());
  feed_dict->importColumnValues("C", col_val);
  mpc_exec->setFeedDict(feed_dict);

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

  delete col_config;
  delete feed_dict;
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
