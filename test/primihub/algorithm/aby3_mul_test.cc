// Copyright [2021] <primihub.com>
#pragma once
#include "gtest/gtest.h"
// #include "src/primihub/algorithm/logistic.h"

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
  // A plaintext matrix can be instantiated as
  u64 rows = 3, cols = 1;
  eMatrix<i64> plainMatrix1(rows, cols);
  eMatrix<i64> plainMatrix2(rows, cols);
  // We can populate is by
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j) {
      plainMatrix1(i, j) = i + j + 1;
      plainMatrix2(i, j) = i + j + 1;
    }
  LOG(INFO) << " plainMatrix1:" << plainMatrix1;
  LOG(INFO) << " plainMatrix2:" << plainMatrix2;

  // To encrypt it, we use
  si64Matrix sharedMatrix1(rows, cols);
  si64Matrix sharedMatrix2(cols, rows);

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
  Sh3Task mulTask =
      eval.asyncDotMul(runtime, sharedMatrix1, sharedMatrix2, prod);

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
  f64<D8> fixedInt = 34.62;

  // We can encrypt this value in the similar way as an integer
  sf64<D8> sharedFixedInt;
  if (partyIdx == 0)
    enc.localFixed(runtime, fixedInt, sharedFixedInt).get();
  else
    enc.remoteFixed(runtime, sharedFixedInt).get();

  // We can add and multiply
  sf64<D8> addition = sharedFixedInt + sharedFixedInt;
  sf64<D8> prod;
  eval.asyncMul(runtime, addition, sharedFixedInt, prod).get();

  // We can also perform matrix operations.
  u64 rows = 5, cols = 1;
  f64Matrix<D20> fixedMatrix1(rows, cols);
  f64Matrix<D20> fixedMatrix2(rows, cols);
  std::vector<double> plaintext;

  plaintext.emplace_back(5762.956599043853 * 3370.387429464423);
  plaintext.emplace_back(4075.605739170733 * 9902.092442274614);
  plaintext.emplace_back(7384.0757247129095 * 2888.276563618976);
  plaintext.emplace_back(5997.638599294733 * 3930.800634388091);
  plaintext.emplace_back(7787.552299068919 * 7265.431388578868);

  fixedMatrix1(0, 0) = 5762.956599043853;
  fixedMatrix1(1, 0) = 4075.605739170733;
  fixedMatrix1(2, 0) = 7384.0757247129095;
  fixedMatrix1(3, 0) = 5997.638599294733;
  fixedMatrix1(4, 0) = 7787.552299068919;

  fixedMatrix2(0, 0) = 3370.387429464423;
  fixedMatrix2(1, 0) = 9902.092442274614;
  fixedMatrix2(2, 0) = 2888.276563618976;
  fixedMatrix2(3, 0) = 3930.800634388091;
  fixedMatrix2(4, 0) = 7265.431388578868;
  // We can populate is by
  // for (u64 i = 0; i < rows; ++i)
  //   for (u64 j = 0; j < cols; ++j) {
  //     fixedMatrix1(i, j) = double(i) / j;
  //     fixedMatrix2(i, j) = double(i) / j;
  //   }
  // To encrypt it, we use
  sf64Matrix<D20> sharedMatrix1(rows, cols);
  sf64Matrix<D20> sharedMatrix2(rows, cols);
  if (partyIdx == 0)
    enc.localFixedMatrix(runtime, fixedMatrix1, sharedMatrix1).get();
  else
    enc.remoteFixedMatrix(runtime, sharedMatrix1).get();

  if (partyIdx == 0)
    enc.localFixedMatrix(runtime, fixedMatrix2, sharedMatrix2).get();
  else
    enc.remoteFixedMatrix(runtime, sharedMatrix2).get();
  // We can add locally
  // sf64Matrix<D8> additionMtx = sharedMatrix + sharedMatrix;

  // We can multiply
  sf64Matrix<D20> prodMtx;
  Sh3Task mulTask =
      eval.asyncDotMul(runtime, sharedMatrix1, sharedMatrix2, prodMtx);

  // we can reconstruct the secret shares
  f64Matrix<D20> fixedMatrix(rows, cols);
  enc.revealAll(mulTask, prodMtx, fixedMatrix).get();
  if (partyIdx == 0) {
    LOG(INFO) << "MPC A*B:" << fixedMatrix;
    for (auto itr = plaintext.begin(); itr != plaintext.end(); itr++)
      LOG(INFO) << "PLAIN A*B:" << std::fixed << static_cast<double>(*itr);
  }
}

void fixedPointOperations1(u64 partyIdx) {
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
  f64<D8> fixedInt1 = 3370.387429464423;
  f64<D8> fixedInt2 = 5762.956599043853;
  // We can encrypt this value in the similar way as an integer
  sf64<D8> sharedFixedInt1;
  sf64<D8> sharedFixedInt2;
  if (partyIdx == 0)
    enc.localFixed(runtime, fixedInt1, sharedFixedInt1).get();
  else
    enc.remoteFixed(runtime, sharedFixedInt1).get();

  if (partyIdx == 0)
    enc.localFixed(runtime, fixedInt2, sharedFixedInt2).get();
  else
    enc.remoteFixed(runtime, sharedFixedInt2).get();

  // We can add and multiply
  sf64<D8> prod;
  f64<D8> result;
  eval.asyncMul(runtime, sharedFixedInt1, sharedFixedInt2, prod).get();
  enc.revealAll(runtime, prod, result).get();

  double prod_plain = 3370.387429464423 * 5762.956599043853;
  if (partyIdx == 0) {
    LOG(INFO) << "MPC A*B:" << result;
    LOG(INFO) << "MPC A*B:" << std::fixed << static_cast<double>(result);
    LOG(INFO) << "PLAIN A*B:" << std::fixed << prod_plain;
  }
}

TEST(add_operator, aby3_3pc_test) {
  pid_t pid = fork();
  if (pid != 0) {
    // Child proess as party 0.
    fixedPointOperations1(0);
    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    // integerOperations(1);
    // fixedPointOperations(1);
    fixedPointOperations1(1);
    return;
  }

  // Parent process as party 2.
  sleep(3);

  fixedPointOperations1(2);

  return;
}
