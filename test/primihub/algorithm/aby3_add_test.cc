// Copyright [2021] <primihub.com>
#pragma once
#include "gtest/gtest.h"

#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

using namespace primihub;

TEST(add_operator, aby3_3pc_test) {
  // input data
  u64 rows = 1, cols = 10;
  eMatrix<i64> plainMatrix(rows, cols);
  for (u64 i = 0; i < rows; ++i)
    for (u64 j = 0; j < cols; ++j)
      plainMatrix(i, j) = i + j;

  std::vector<si64Matrix> sharedMatrix;
  sharedMatrix.emplace_back(si64Matrix(rows, cols));
  sharedMatrix.emplace_back(si64Matrix(rows, cols));
  sharedMatrix.emplace_back(si64Matrix(rows, cols));

  pid_t pid = fork();
  if (pid != 0) {
    MPCOperator mpc(0, "01", "02");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1313, (u32)1414);

    // construct shares
    for (u64 i = 0; i < 3; i++) {
      if (i == mpc.partyIdx) {
        mpc.createShares(plainMatrix, sharedMatrix[i]);
      } else {
        mpc.createShares(sharedMatrix[i]);
      }
    }

    // Add
    si64Matrix sum;
    sum = mpc.MPC_Add(sharedMatrix);
    i64Matrix sumVal = mpc.revealAll(sum);

    mpc.fini();
    return;
  }

  pid = fork();
  if (pid != 0) {
    sleep(1);
    MPCOperator mpc(1, "12", "01");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1515, (u32)1313);

    // construct shares
    for (u64 i = 0; i < 3; i++) {
      if (i == mpc.partyIdx) {
        mpc.createShares(plainMatrix, sharedMatrix[i]);
      } else {
        mpc.createShares(sharedMatrix[i]);
      }
    }

    si64Matrix sum;
    sum = mpc.MPC_Add(sharedMatrix);
    i64Matrix sumVal = mpc.revealAll(sum);

    mpc.fini();
    return;
  }

  // Parent process as party 2.
  sleep(2);
  MPCOperator mpc(2, "02", "12");
  mpc.setup("127.0.0.1", "127.0.0.1", (u32)1414, (u32)1515);

  // construct shares
  for (u64 i = 0; i < 3; i++) {
    if (i == mpc.partyIdx) {
      mpc.createShares(plainMatrix, sharedMatrix[i]);
    } else {
      mpc.createShares(sharedMatrix[i]);
    }
  }

  si64Matrix sum;
  sum = mpc.MPC_Add(sharedMatrix);
  i64Matrix sumVal = mpc.revealAll(sum);

  mpc.fini();
  return;
}
