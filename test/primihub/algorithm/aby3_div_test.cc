// Copyright [2021] <primihub.com>
#pragma once
#include "gtest/gtest.h"
// #include "src/primihub/algorithm/logistic.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/eigen_util.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"
#include <Eigen/Dense>
#include <iostream>
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
    // comm.mNext = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    // "01").addChannel(); comm.mPrev = Session(ios, "127.0.0.1:1313",
    // SessionMode::Server, "02").addChannel();
    ep_next_.start(ios, "127.0.0.1", 1313, SessionMode::Server, "01");
    ep_prev_.start(ios, "127.0.0.1", 1414, SessionMode::Server, "02");

    // comm.setPrev(Session(ios, "127.0.0.1", 1414, SessionMode::Server,
    // "02").addChannel());

    break;
  case 1:
    // comm.mNext = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    // "12").addChannel(); comm.mPrev = Session(ios, "127.0.0.1:1313",
    // SessionMode::Client, "01").addChannel();
    ep_next_.start(ios, "127.0.0.1", 1515, SessionMode::Server, "12");
    ep_prev_.start(ios, "127.0.0.1", 1313, SessionMode::Client, "01");
    // comm.setNext(Session(ios, "127.0.0.1", 1515, SessionMode::Server,
    // "12").addChannel()); comm.setPrev(Session(ios, "127.0.0.1", 1313,
    // SessionMode::Client, "01").addChannel());
    break;
  default:
    // comm.mNext = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    // "02").addChannel(); comm.mPrev = Session(ios, "127.0.0.1:1313",
    // SessionMode::Client, "12").addChannel();
    ep_next_.start(ios, "127.0.0.1", 1414, SessionMode::Client, "02");
    ep_prev_.start(ios, "127.0.0.1", 1515, SessionMode::Client, "12");
    // comm.setNext(Session(ios, "127.0.0.1", 1414, SessionMode::Client,
    // "02").addChannel()); comm.setPrev(Session(ios, "127.0.0.1", 1515,
    // SessionMode::Client, "12").addChannel());
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
  // in a real work example, where parties
  // have different IPs, you have to give the
  // Clients the IP of the server and you give
  // the servers their own IP (to listen to).

  // Establishes some shared randomness needed for the later protocols
  enc.init(partyIdx, comm, sysRandomSeed());

  // Establishes some shared randomness needed for the later protocols
  eval.init(partyIdx, comm, sysRandomSeed());

  // Copies the Channels and will use them for later protocols.
  // std::shared_ptr<CommPkgBase> commPtr(&comm);
  auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
  runtime.init(partyIdx, commPtr);
}

TEST(add_operator, aby3_3pc_test) {

  pid_t pid = fork();
  if (pid != 0) {
    MPCOperator mpc(0, "01", "02");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1313, (u32)1414);
    // u64 rows = 4,
    // 	cols = 4;
    // // input data
    // eMatrix<i64> plainMatrix(rows, cols);
    // for (u64 i = 0; i < rows; ++i)
    // 	for (u64 j = 0; j < cols; ++j)
    // 		plainMatrix(i, j) = -10 + i + j;

    // sfmatrix
    u64 rows = 4, cols = 1;
    f64Matrix<D20> f64fixedMatrix(rows, cols);
    double divisor[4] = {6.5, -15.0, 23.2, -33.0};
    vector<double> test_number(divisor, divisor + 4);
    f64Matrix<D20> f64fixedMatrix_B(rows, cols);
    for (u64 i = 0; i < rows; ++i) {
      for (u64 j = 0; j < cols; ++j) {
        f64fixedMatrix(i, j) = 2 + double(i) + j;
        f64fixedMatrix_B(i, j) = test_number[i + j];
      }
    }
    f64fixedMatrix(2, 0) = -4;
    f64fixedMatrix(3, 0) = -5;
    sf64Matrix<D20> sf64fixedMatrix(rows, cols);
    sf64Matrix<D20> sf64fixedMatrix_B(rows, cols);
    // To encrypt it, we use
    // sf64Matrix<D20> sharedMatrix(rows, cols);

    if (mpc.partyIdx == 0) {
      mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix, sf64fixedMatrix)
          .get();
      mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix_B, sf64fixedMatrix_B)
          .get();
    } else {
      mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix).get();
      mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix_B).get();
    }

    // division
    sf64Matrix<D20> div_result(sf64fixedMatrix.rows(), sf64fixedMatrix.cols());
    // mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B, div_result);
    div_result = mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B);

    std::cout << "div_result result "
              << mpc.revealAll(div_result).format(CSVFormat) << std::endl;
    mpc.fini();
    return;
  }

  pid = fork();
  if (pid != 0) {
    MPCOperator mpc(1, "12", "01");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1515, (u32)1313);
    u64 rows = 4, cols = 1;
    f64Matrix<D20> f64fixedMatrix(rows, cols);
    double divisor[4] = {-6.5, -15.0, 23.2, 33.0};
    vector<double> test_number(divisor, divisor + 4);
    f64Matrix<D20> f64fixedMatrix_B(rows, cols);
    for (u64 i = 0; i < rows; ++i) {
      for (u64 j = 0; j < cols; ++j) {
        f64fixedMatrix(i, j) = 2 + double(i) + j;
        f64fixedMatrix_B(i, j) = test_number[i + j];
      }
    }

    sf64Matrix<D20> sf64fixedMatrix(rows, cols);
    sf64Matrix<D20> sf64fixedMatrix_B(rows, cols);
    // To encrypt it, we use
    // sf64Matrix<D20> sharedMatrix(rows, cols);

    if (mpc.partyIdx == 0) {
      mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix, sf64fixedMatrix)
          .get();
      mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix_B, sf64fixedMatrix_B)
          .get();
    } else {
      mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix).get();
      mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix_B).get();
    }

    // division
    sf64Matrix<D20> div_result(sf64fixedMatrix.rows(), sf64fixedMatrix.cols());
    // mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B, div_result);
    div_result = mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B);
    mpc.revealAll(div_result);
    mpc.fini();
    return;
  }

  // Parent process as party 2.
  sleep(3);

  MPCOperator mpc(2, "02", "12");
  mpc.setup("127.0.0.1", "127.0.0.1", (u32)1414, (u32)1515);
  u64 rows = 4, cols = 1;
  f64Matrix<D20> f64fixedMatrix(rows, cols);
  double divisor[4] = {-6.5, -15.0, 23.2, 33.0};
  vector<double> test_number(divisor, divisor + 4);
  f64Matrix<D20> f64fixedMatrix_B(rows, cols);
  for (u64 i = 0; i < rows; ++i) {
    for (u64 j = 0; j < cols; ++j) {
      // f64fixedMatrix(i, j) = 2.3343 + double(i) + j;
      f64fixedMatrix(i, j) = 2 + double(i) + j;
      f64fixedMatrix_B(i, j) = test_number[i + j];
    }
  }

  sf64Matrix<D20> sf64fixedMatrix(rows, cols);
  sf64Matrix<D20> sf64fixedMatrix_B(rows, cols);
  // To encrypt it, we use
  // sf64Matrix<D20> sharedMatrix(rows, cols);

  if (mpc.partyIdx == 0) {
    mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix, sf64fixedMatrix)
        .get();
    mpc.enc.localFixedMatrix(mpc.runtime, f64fixedMatrix_B, sf64fixedMatrix_B)
        .get();
  } else {
    mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix).get();
    mpc.enc.remoteFixedMatrix(mpc.runtime, sf64fixedMatrix_B).get();
  }

  // division
  sf64Matrix<D20> div_result(sf64fixedMatrix.rows(), sf64fixedMatrix.cols());
  // mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B, div_result);
  div_result = mpc.MPC_Div(sf64fixedMatrix, sf64fixedMatrix_B);

  mpc.revealAll(div_result);
  mpc.fini();
  return;
}
