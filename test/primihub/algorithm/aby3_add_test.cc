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
  // for details.
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

  // Copies the Channels and will use them for later protocols.
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

  // We can add locally
  si64Matrix addition = sharedMatrix1 + sharedMatrix1;

  // We can multiply
  si64Matrix prod;
  Sh3Task mulTask = eval.asyncMul(runtime, sharedMatrix1, addition, prod);

  // we can reconstruct the secret shares
  eMatrix<i64> plainMatrix(1, 1);
  enc.revealAll(mulTask, prod, plainMatrix).get();
  LOG(INFO) << plainMatrix;
}

void fixedPointOperations(u64 partyIdx) {
  IOService ios;
  Sh3Encryptor enc;
  Sh3Evaluator eval;
  Sh3Runtime runtime;
  setup(partyIdx, ios, enc, eval, runtime);

  // The framework also supports the ability to perform
  // fixed point computation. This is similar to the
  // double or float type in c++. The key difference is
  // that it is implemented as an integer where a fixed
  // number of the bits represent decimal/fraction part.

  // This represent a plain 64-bit value where the bottom
  // 8-bit of the integer represent the fractional part.
  f64<D16> fixedInt = 34.62;
  f64<D16> constfixed = 2.5;
  // We can encrypt this value in the similar way as an integer
  sf64<D16> sharedFixedInt;
  if (partyIdx == 0)
    enc.localFixed(runtime, fixedInt, sharedFixedInt).get();
  else
    enc.remoteFixed(runtime, sharedFixedInt).get();

  if (partyIdx == 0)
    sharedFixedInt[0] = sharedFixedInt[0] + constfixed.mValue;
  if (partyIdx == 1)
    sharedFixedInt[1] = sharedFixedInt[1] + constfixed.mValue;
  LOG(INFO) << "party:" << partyIdx << "======" << sharedFixedInt[0] << "    "
            << sharedFixedInt[1];
  f64<D16> fixedVal;
  enc.revealAll(runtime, sharedFixedInt, fixedVal).get();
  LOG(INFO) << "party:" << partyIdx << "======" << fixedVal;

  // We can add and multiply
  // sf64<D8> addition = sharedFixedInt + sharedFixedInt;
  // sf64<D8> prod;
  // eval.asyncMul(runtime, addition, sharedFixedInt, prod).get();

  // We can also perform matrix operations.
  u64 rows = 3, cols = 2;
  f64Matrix<D16> fixedMatrix(rows, cols);

  // We can populate is by
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j)
      fixedMatrix(i, j) = 2.52;

  // To encrypt it, we use
  sf64Matrix<D16> sharedMatrix(rows, cols);
  if (partyIdx == 0)
    enc.localFixedMatrix(runtime, fixedMatrix, sharedMatrix).get();
  else
    enc.remoteFixedMatrix(runtime, sharedMatrix).get();

  if (partyIdx == 0)
    for (i64 i = 0; i < sharedMatrix.rows(); i++)
      for (i64 j = 0; j < sharedMatrix.cols(); j++)
        sharedMatrix[0](i, j) = sharedMatrix[0](i, j) + constfixed.mValue;
  else if (partyIdx == 1)
    // sharedFixed[1] = sharedFixed[1] + constfixed.mValue;
    for (i64 i = 0; i < sharedMatrix.rows(); i++)
      for (i64 j = 0; j < sharedMatrix.cols(); j++)
        sharedMatrix[1](i, j) = sharedMatrix[1](i, j) + constfixed.mValue;

  f64Matrix<D16> constFixedMatrix(rows, cols);
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j)
      constFixedMatrix(i, j) = 5;
  if (partyIdx == 0)
    sharedMatrix[0] = sharedMatrix[0] + constFixedMatrix.i64Cast();
  else if (partyIdx == 1)
    sharedMatrix[1] = sharedMatrix[1] + constFixedMatrix.i64Cast();
  // // We can add locally
  // sf64Matrix<D8> additionMtx = sharedMatrix + sharedMatrix;

  // // We can multiply
  // sf64Matrix<D8> prodMtx;
  // Sh3Task mulTask = eval.asyncMul(runtime, sharedMatrix, additionMtx,
  // prodMtx);
  f64<D16> constfixed1 = 10;
  sf64Matrix<D16> sharedMatrix1(rows, cols);
  eval.asyncConstFixedMul(runtime, constfixed1, sharedMatrix, sharedMatrix1)
      .get();
  // we can reconstruct the secret shares
  f64Matrix<D16> finalMatrix(rows, cols);
  enc.revealAll(runtime, sharedMatrix1, finalMatrix).get();
  LOG(INFO) << "party:" << partyIdx << "======" << finalMatrix;
}

TEST(add_operator, aby3_3pc_test) {
  pid_t pid = fork();
  if (pid != 0) {
    // Child process as party 0.
    // integerOperations(0);
    fixedPointOperations(0);
    // matrixOperations(0);
    MPCOperator mpc(0, "01", "02");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1313, (u32)1414);
    u64 rows = 2, cols = 2;
    // input data
    eMatrix<i64> plainMatrix(rows, cols);
    for (u64 i = 0; i < rows; ++i)
      for (u64 j = 0; j < cols; ++j)
        plainMatrix(i, j) = i + j;

    std::vector<si64Matrix> sharedMatrix;
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    // construct shares
    for (u64 i = 0; i < 3; i++) {
      if (i == mpc.partyIdx) {
        mpc.createShares(plainMatrix, sharedMatrix[i]);
      } else {
        mpc.createShares(sharedMatrix[i]);
      }
    }
    // Add
    //  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
    si64Matrix sum;
    sum = mpc.MPC_Add(sharedMatrix);
    si64Matrix prod;
    prod = mpc.MPC_Mul(sharedMatrix);
    LOG(INFO) << "starting reveal";

    i64Matrix sumVal = mpc.reveal(sum);
    i64Matrix prodVal = mpc.reveal(prod);
    LOG(INFO) << "Sum:" << sumVal;
    LOG(INFO) << "Prod:" << prodVal;
    mpc.fini();

    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    // integerOperations(1);
    fixedPointOperations(1);
    // matrixOperations(1);

    MPCOperator mpc(1, "12", "01");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1515, (u32)1313);
    u64 rows = 2, cols = 2;
    // input data
    eMatrix<i64> plainMatrix(rows, cols);
    for (u64 i = 0; i < rows; ++i)
      for (u64 j = 0; j < cols; ++j)
        plainMatrix(i, j) = i + j;
    std::vector<si64Matrix> sharedMatrix;
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    sharedMatrix.emplace_back(si64Matrix(rows, cols));
    // construct shares
    for (u64 i = 0; i < 3; i++) {
      if (i == mpc.partyIdx) {
        mpc.createShares(plainMatrix, sharedMatrix[i]);
      } else {
        mpc.createShares(sharedMatrix[i]);
      }
    }
    // Add
    //  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
    si64Matrix sum;
    sum = mpc.MPC_Add(sharedMatrix);
    si64Matrix prod;
    prod = mpc.MPC_Mul(sharedMatrix);
    mpc.reveal(sum, 0);
    mpc.reveal(prod, 0);
    mpc.fini();

    return;
  }

  // Parent process as party 2.
  sleep(3);

  // integerOperations(2);
  fixedPointOperations(2);
  // matrixOperations(2);
  MPCOperator mpc(2, "02", "12");
  mpc.setup("127.0.0.1", "127.0.0.1", (u32)1414, (u32)1515);
  u64 rows = 2, cols = 2;
  // input data
  eMatrix<i64> plainMatrix(rows, cols);
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j)
      plainMatrix(i, j) = i + j + 1;

  std::vector<si64Matrix> sharedMatrix;
  sharedMatrix.emplace_back(si64Matrix(rows, cols));
  sharedMatrix.emplace_back(si64Matrix(rows, cols));
  sharedMatrix.emplace_back(si64Matrix(rows, cols));
  // construct shares
  for (u64 i = 0; i < 3; i++) {
    if (i == mpc.partyIdx) {
      mpc.createShares(plainMatrix, sharedMatrix[i]);
    } else {
      mpc.createShares(sharedMatrix[i]);
    }
  }
  // Add
  //  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
  si64Matrix sum;
  sum = mpc.MPC_Add(sharedMatrix);
  si64Matrix prod;
  prod = mpc.MPC_Mul(sharedMatrix);
  mpc.reveal(sum, 0);
  mpc.reveal(prod, 0);
  mpc.fini();
  return;
}
