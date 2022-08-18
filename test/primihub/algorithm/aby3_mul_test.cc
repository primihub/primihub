// Copyright [2021] <primihub.com>
#pragma once
#include "gtest/gtest.h"
// #include "src/primihub/algorithm/logistic.h"
// #include "src/primihub/service/dataset/localkv/storage_default.h"

#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"
using namespace primihub;
int setup(u64 partyIdx, IOService &ios, Sh3Encryptor &enc, Sh3Evaluator &eval,
          Sh3Runtime &runtime) {

  // A CommPkg is a pair of Channels (network sockets) to the other parties.
  // See cryptoTools\frontend_cryptoTools\Tutorials\Network.cpp
  // for detials.
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
void matrixOperations(u64 partyIdx) {
  IOService ios;
  Sh3Encryptor enc;
  Sh3Evaluator eval;
  Sh3Runtime runtime;
  setup(partyIdx, ios, enc, eval, runtime);

  // In addition to working with individual integers,
  // ABY3 directly supports performing matrix
  // multiplications. Matrix operations are more efficient
  // for several reasons. First, there is less overhead from
  // the runtime and second we can use more efficient protocols
  // in many cases. See the ABY3 paper for details.

  // A plaintext matrix can be instantiated as
  u64 rows = 2, cols = 1;
  eMatrix<i64> plainMatrix1(rows, cols);
  eMatrix<i64> plainMatrix2(cols, rows);
  // We can populate is by
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j) {
      plainMatrix1(i, j) = i + j + 1;
      plainMatrix2(j, i) = i + j + 1;
    }
  LOG(INFO) << " plainMatrix1:" << plainMatrix1;
  LOG(INFO) << " plainMatrix2:" << plainMatrix2;

  // To encrypt it, we use
  si64Matrix sharedMatrix1(rows, cols); //该结构中有两个矩阵，存放两份分享值
  si64Matrix sharedMatrix2(cols, rows); //该结构中有两个矩阵，存放两份分享值

  if (partyIdx == 0) {
    enc.localIntMatrix(runtime, plainMatrix1, sharedMatrix1).get();
  } else
    enc.remoteIntMatrix(runtime, sharedMatrix1).get();

  if (partyIdx == 1) {
    enc.localIntMatrix(runtime, plainMatrix2, sharedMatrix2).get();
  } else
    enc.remoteIntMatrix(runtime, sharedMatrix2).get();
  // if (partyIdx == 0)
  //   enc.localIntMatrix(runtime, plainMatrix, sharedMatrix).get();
  // else
  //   enc.remoteIntMatrix(runtime, sharedMatrix).get();

  // We can multiply
  si64Matrix prod;
  Sh3Task mulTask = eval.asyncMul(runtime, sharedMatrix1, sharedMatrix2, prod);

  // we can reconstruct the secret shares
  eMatrix<i64> plainMatrix(
      1, 1); //这里矩阵是什么结构不影响，下边揭露会重新设置结构
  enc.revealAll(mulTask, prod, plainMatrix).get();
  LOG(INFO) << plainMatrix;
}

TEST(add_operator, aby3_3pc_test) {
  pid_t pid = fork();
  if (pid != 0) {
    // Child proess as party 0.
    matrixOperations(0);
    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    // integerOperations(1);
    // fixedPointOperations(1);
    matrixOperations(1);
    return;
  }

  // Parent process as party 2.
  sleep(3);

  matrixOperations(2);
  return;
}
