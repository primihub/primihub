#include <time.h>

#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

using namespace primihub;

int setup(u64 partyIdx, IOService &ios, Sh3Encryptor &enc, Sh3Evaluator &eval,
          Sh3Runtime &runtime) {
  CommPkg comm = CommPkg();
  Session ep_next_;
  Session ep_prev_;

  switch (partyIdx) {
  case 0:
    ep_next_.start(ios, "127.0.0.1", 1313, SessionMode::Server, "01");
    ep_prev_.start(ios, "127.0.0.1", 1414, SessionMode::Server, "02");
    break;
  case 1:
    ep_next_.start(ios, "127.0.0.1", 1515, SessionMode::Server, "12");
    ep_prev_.start(ios, "127.0.0.1", 1313, SessionMode::Client, "01");
    break;
  default:
    ep_next_.start(ios, "127.0.0.1", 1414, SessionMode::Client, "02");
    ep_prev_.start(ios, "127.0.0.1", 1515, SessionMode::Client, "12");
    break;
  }

  comm.setNext(ep_next_.addChannel());
  comm.setPrev(ep_prev_.addChannel());
  comm.mNext().waitForConnection();
  comm.mPrev().waitForConnection();
  comm.mNext().send(partyIdx);
  comm.mPrev().send(partyIdx);

  u64 prev_party = 0;
  u64 next_party = 0;
  comm.mNext().recv(next_party);
  comm.mPrev().recv(prev_party);
  if (next_party != (partyIdx + 1) % 3) {
    LOG(ERROR) << "Party " << partyIdx << ", expect next party id "
               << (partyIdx + 1) % 3 << ", but give " << next_party << ".";
    return -3;
  }

  if (prev_party != (partyIdx + 2) % 3) {
    LOG(ERROR) << "Party " << partyIdx << ", expect prev party id "
               << (partyIdx + 2) % 3 << ", but give " << prev_party << ".";
    return -3;
  }

  // Establishes some shared randomness needed for the later protocols
  enc.init(partyIdx, comm, sysRandomSeed());

  // Establishes some shared randomness needed for the later protocols
  eval.init(partyIdx, comm, sysRandomSeed());

  // Copies the Channels and will use them for later protcols.
  auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
  runtime.init(partyIdx, commPtr);
  return 1;
}

double matrixOperations(u64 partyIdx, eMatrix<i64> &plainMatrix1,
                        eMatrix<i64> &plainMatrix2, eMatrix<i64> &outMatrix,
                        u64 rows, u64 cols) {
  IOService ios;
  Sh3Encryptor enc;
  Sh3Evaluator eval;
  Sh3Runtime runtime;

  setup(partyIdx, ios, enc, eval, runtime);

  si64Matrix sharedMatrix1(rows, cols);
  si64Matrix sharedMatrix2(rows, cols);

  LOG(INFO) << "Begin to create shares.";
  if (partyIdx == 0) {
    enc.localIntMatrix(runtime, plainMatrix1, sharedMatrix1).get();
  } else
    enc.remoteIntMatrix(runtime, sharedMatrix1).get();

  if (partyIdx == 1) {
    enc.localIntMatrix(runtime, plainMatrix2, sharedMatrix2).get();
  } else
    enc.remoteIntMatrix(runtime, sharedMatrix2).get();

  LOG(INFO) << "Finish.";
  
  si64Matrix prod;
  clock_t st_clock = 0, ed_clock = 0;

  LOG(INFO) << "Begin to run MPC Mul.";
  st_clock = clock();
  Sh3Task mulTask = eval.asyncMul(runtime, sharedMatrix1, sharedMatrix2, prod);
  mulTask.get();
  ed_clock = clock();
  LOG(INFO) << "Finish.";

  double mpc_time = (double)(ed_clock - st_clock) / CLOCKS_PER_SEC;
  std::cerr << "MPC Mul use " << mpc_time << " seconds." << std::endl;

  enc.revealAll(mulTask, prod, outMatrix).get();

  return mpc_time;
}

TEST(mpc_mul, aby3_3pc_test) {
  u64 rows = 1000000, cols = 1;

  eMatrix<i64> plainMatrix1(rows, cols);
  eMatrix<i64> plainMatrix2(rows, cols);
  eMatrix<i64> outMatrix(rows, cols);

  double matmul_time = 0;
  {
    clock_t st_clock = 0, ed_clock = 0;
    eMatrix<i64> outMatrix(rows, cols);
    st_clock = clock();
    outMatrix = plainMatrix1 * plainMatrix2;
    ed_clock = clock();
    matmul_time = (double)(ed_clock - st_clock) / CLOCKS_PER_SEC;
    std::cerr << "MatMul use " << matmul_time << " seconds." << std::endl;
  }

  srand(time(nullptr));
  for (u64 i = 0; i < rows; ++i) {
    for (u64 j = 0; j < cols; ++j) {
      plainMatrix1(i, j) = rand() % 100;
      plainMatrix2(i, j) = rand() % 100;
    }
  }

  bool standalone = true;
  if (standalone) {
    pid_t pid = fork();
    if (pid != 0) {
      // Child proess as party 0.
      matrixOperations(0, plainMatrix1, plainMatrix2, outMatrix, rows, cols);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // Child process as party 1.
      sleep(2);
      matrixOperations(1, plainMatrix1, plainMatrix2, outMatrix, rows, cols);
      return;
    }

    // Parent process as party 2.
    sleep(3);
    double mpc_time =
        matrixOperations(2, plainMatrix1, plainMatrix2, outMatrix, rows, cols);

    LOG(INFO) << "Record MPC time: " << mpc_time;
    LOG(INFO) << "Record Plain time: " << matmul_time;
  } else {
    double mpc_time = 0;
    if (std::string(std::getenv("MPC_PARTY")) == "MPC_PARTY_0") {
      mpc_time = matrixOperations(0, plainMatrix1, plainMatrix2, outMatrix,
                                  rows, cols);
    } else if (std::string(std::getenv("MPC_PARTY")) == "MPC_PARTY_1") {
      mpc_time = matrixOperations(1, plainMatrix1, plainMatrix2, outMatrix,
                                  rows, cols);
    } else if (std::string(std::getenv("MPC_PARTY")) == "MPC_PARTY_2") {
      mpc_time = matrixOperations(2, plainMatrix1, plainMatrix2, outMatrix,
                                  rows, cols);
    }

    LOG(INFO) << "Record MPC time: " << mpc_time;
    LOG(INFO) << "Record Plain time: " << matmul_time;
  }

  return;
}
