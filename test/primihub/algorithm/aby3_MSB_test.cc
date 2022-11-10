#pragma once
#include "src/primihub/operator/aby3_operator.h"
#include "src/primihub/primitive/ppa/kogge_stone.h"
#include "src/primihub/util/log.h"
#include "gtest/gtest.h"
using namespace primihub;
#define GCOUT std::cerr << "[          ] [ INFO ]"

void Sh3_BinaryEngine_test(
    BetaCircuit *cir,
    std::function<fp<i64, D8>(fp<i64, D8>, fp<i64, D8>)> binOp, bool debug,
    std::string opName, u64 pIdx, Sh3Encryptor &enc, Sh3Runtime &rt,
    Sh3BinaryEvaluator &binEval, u64 valMask = ~0ull) {
  cir->levelByAndDepth();
  bool failed = false;

  // u64 width = 1 << 8;
  u64 width = 2;
  // std::array<std::vector<Matrix<i64>>, 3> CC;
  // std::array<std::vector<Matrix<i64>>, 3> CC2;
  std::array<std::atomic<int>, 3> ac;
  auto aSize = cir->mInputs[0].size(); // 64
  auto bSize = cir->mInputs[1].size();
  auto cSize = cir->mOutputs[0].size(); //为1，有1根线
  LOG(INFO) << "aSize: " << aSize;
  LOG(INFO) << "bSize: " << bSize;
  LOG(INFO) << "cSize: " << cSize;
  //   PRNG prng(ZeroBlock);
  // for (u64 i = 0; i < a.size(); ++i) {
  //   a(i) = prng.get<i64>() & valMask;
  //   b(i) = prng.get<i64>() & valMask;
  // }
  f64Matrix<D8> a(width, 1), b(width, 1); // 2行1列
  // f64Matrix<D8> c(width, 1);
  i64Matrix c(width, 1);
  for (u64 i = 0; i < a.size(); ++i) {
    a(i) = 2.1 + 10 * i;
    b(i) = -10.5;
  }
  //   ar(0) = prng.get<i64>();
  LOG(INFO) << "partyID: " << pIdx << "============> a: " << a << a.i64Cast();
  LOG(INFO) << "partyID: " << pIdx << "============> b: " << b << b.i64Cast();
  // LOG(INFO) << "partyID: " << pIdx << "============> ar: " << ar;
  //   Sh3Runtime rt(pIdx, comms[pIdx]);

  sbMatrix A(width, aSize), B(width, bSize), // A是2行
      C(width, cSize);                       //分享值矩阵，2行,1列
                                             //   sbMatrix Ar(1, aSize);
  LOG(INFO) << "pIdx: " << pIdx << "==============>C.i64Size: " << C.i64Size();
  LOG(INFO) << "pIdx: " << pIdx << "==============>C.rows: " << C.rows();
  LOG(INFO) << "pIdx: " << pIdx << "==============>C.i64Cols: " << C.i64Cols();
  LOG(INFO) << "pIdx: " << pIdx
            << "==============>C.bitCount: " << C.bitCount();
  //   Sh3Encryptor enc;
  //   enc.init(pIdx, toBlock(pIdx), toBlock((pIdx + 1) % 3));

  auto task = rt.noDependencies();
  //通过此处构造分享
  if (pIdx == 0)
    enc.localBinMatrix(rt.noDependencies(), a.i64Cast(), A).get();
  else
    enc.remoteBinMatrix(rt.noDependencies(), A).get();
  if (pIdx == 1)
    enc.localBinMatrix(rt.noDependencies(), b.i64Cast(), B).get();
  else
    enc.remoteBinMatrix(rt.noDependencies(), B).get();

  binEval.mPrng.SetSeed(toBlock(pIdx));
  Sh3ShareGen gen;
  gen.init(toBlock(pIdx), toBlock((pIdx + 1) % 3));

  C.mShares[0](0) = 0;
  C.mShares[1](0) = 0;
  task.get();
  binEval.setCir(cir, width, gen);
  binEval.setInput(0, A); //设置第一个输入
  binEval.setInput(1, B); //设置第二个输入
  binEval.asyncEvaluate(rt.noDependencies()).get();
  binEval.getOutput(0, C);
  task.get();
  for (u64 i = 0; i < C.mShares[0].size(); i++)
    LOG(INFO) << "pIdx: " << pIdx << "==============> C[0][" << i
              << "]: " << C.mShares[0](i);
  for (u64 i = 0; i < C.mShares[1].size(); i++)
    LOG(INFO) << "pIdx: " << pIdx << "==============> C[1][" << i
              << "]: " << C.mShares[1](i);

  // auto ccc0 = C.mShares[0](0);
  // CC[pIdx].push_back(C.mShares[0]);
  // CC2[pIdx].push_back(C.mShares[1]);
  // ++ac[pIdx];
  // auto ccc1 = C.mShares[0](0);
  // auto ccc2 = CC[pIdx].back()(0);

  if (pIdx) {
    enc.reveal(task, 0, C).get();
    // auto CC = getShares(C, rt, debugComm[pIdx]);
  } else {
    enc.reveal(task, C, c).get();
    // auto ci = CC[pIdx].size() - 1;
    LOG(INFO) << "ID: " << pIdx << "==============> c:" << c;

    for (u64 j = 0; j < width; ++j) {
      if (c(j) != binOp(a(j), b(j))) {
        GCOUT << "pidx: " << rt.mPartyIdx << " debug:" << int(debug)
              << " failed at " << j << " " << std::hex << c(j)
              << " != " << std::hex << binOp(a(j), b(j)) << " = " << opName
              << "(" << std::hex << a(j) << ", " << std::hex << b(j) << ")"
              << std::endl
              << std::dec;
        failed = true;
      }
    }

    if (a.i64Cast()(0) == 0x66 && failed) {
      while (ac[1] < ac[0])
        ;
      while (ac[2] < ac[0])
        ;
      GCOUT << "pidx: " << rt.mPartyIdx << " check\n";
      GCOUT << "      a " << A.mShares[0](0) << " " << A.mShares[1](0)
            << std::endl;
      GCOUT << "      b " << B.mShares[0](0) << " " << B.mShares[1](0)
            << std::endl;
      GCOUT << "      c " << C.mShares[0](0) << " " << C.mShares[1](0)
            << std::endl;
      // GCOUT << "      C " << CC[0][ci](0) << " " << CC[1][ci](0) << " "
      //       << CC[2][ci](0) << std::endl;
      // GCOUT << "      D " << CC2[0][ci](0) << " " << CC2[1][ci](0) << " "
      //       << CC2[2][ci](0) << std::endl;
      // GCOUT << "        " << ccc0 << " " << ccc1 << " " << ccc2 << std::endl;
      GCOUT << "   recv " << binEval.mRecvFutr.size() << std::endl;

      GCOUT << "   " << binEval.mLog.str() << std::endl;

      // GCOUT << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
      // GCOUT << binEval.mLog.str() << std::endl;
      // GCOUT << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
      // GCOUT << evals[2].mLog.str() << std::endl;
    }
  }
}

void computeMSB(sbMatrix &A, sbMatrix &B, sbMatrix &C, u64 width,
                Sh3ShareGen &gen, Sh3Runtime &runtime,
                Sh3BinaryEvaluator &binEval) {
  KoggeStoneLibrary lib;
  u64 size = 64;
  u64 mask = ~0ull;
  BetaCircuit *cir = lib.int_int_add_msb(size);
  auto aSize = cir->mInputs[0].size();
  auto bSize = cir->mInputs[1].size();
  auto cSize = cir->mOutputs[0].size(); // one wine

  cir->levelByAndDepth();
  bool failed = false;
  A.resize(width, aSize), B.resize(width, bSize), C.resize(width, cSize);
  C.mShares[0](0) = 0;
  C.mShares[1](0) = 0;
  auto task = runtime.noDependencies();
  task.get();
  binEval.setCir(cir, width, gen);
  binEval.setInput(0, A); // set first input
  binEval.setInput(1, B); // set second input
  binEval.asyncEvaluate(task).get();
  binEval.getOutput(0, C);
  task.get();
}
TEST(aby3_msb_test, aby3_3pc_test) {
  pid_t pid = fork();
  if (pid != 0) {
    // Child process as party 0.
    u64 pIdx = 0;
    MPCOperator mpc(pIdx, "01", "02");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1313, (u32)1414);

    u64 width = 2;
    f64Matrix<D8> a(width, 1); // 2*1
    LOG(INFO) << a.size();
    i64Matrix c(width, 1);
    for (u64 i = 0; i < a.size(); ++i) {
      a(i) = 2.1 + 10 * i;
    }
    // sbMatrix A(4, 64), B(4, 64), C(4, 1);
    sbMatrix A(width, 64), B(width, 64), C(width, 1);
    mpc.enc.localBinMatrix(mpc.runtime.noDependencies(), a.i64Cast(), A).get();
    mpc.enc.remoteBinMatrix(mpc.runtime.noDependencies(), B).get();
    computeMSB(A, B, C, 2, mpc.gen, mpc.runtime, mpc.binEval);
    mpc.enc.reveal(mpc.runtime.noDependencies(), C, c).get();
    LOG(INFO) << "ID: " << pIdx << "==============> c:" << c;
    LOG(INFO) << "mission complete";
    mpc.fini();
    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    u64 pIdx = 1;
    MPCOperator mpc(pIdx, "12", "01");
    mpc.setup("127.0.0.1", "127.0.0.1", (u32)1515, (u32)1313);
    u64 width = 2;
    f64Matrix<D8> b(width, 1); // 2*1
    for (u64 i = 0; i < b.size(); ++i) {
      b(i) = -10.5;
    }
    sbMatrix A(width, 64), B(width, 64), C(width, 1); // the row of A is 2
    // sbMatrix A(4, 64), B(4, 64), C(4, 1);
    mpc.enc.remoteBinMatrix(mpc.runtime.noDependencies(), A).get();
    mpc.enc.localBinMatrix(mpc.runtime.noDependencies(), b.i64Cast(), B).get();
    computeMSB(A, B, C, 2, mpc.gen, mpc.runtime, mpc.binEval);
    for (u64 i = 0; i < C.mShares[0].size(); i++)
      LOG(INFO) << "pIdx: " << pIdx << "==============> C[0][" << i
                << "]: " << C.mShares[0](i);
    for (u64 i = 0; i < C.mShares[1].size(); i++)
      LOG(INFO) << "pIdx: " << pIdx << "==============> C[1][" << i
                << "]: " << C.mShares[1](i);
    mpc.enc.reveal(mpc.runtime.noDependencies(), 0, C).get();
    LOG(INFO) << "mission complete";
    mpc.fini();
    return;
  }
  // Parent process as party 2.
  sleep(3);
  u64 pIdx = 2;
  MPCOperator mpc(pIdx, "02", "12");
  mpc.setup("127.0.0.1", "127.0.0.1", (u32)1414, (u32)1515);
  u64 width = 2;

  sbMatrix A(width, 64), B(width, 64), C(width, 1);
  // sbMatrix A(4, 64), B(4, 64), C(4, 1);

  mpc.enc.remoteBinMatrix(mpc.runtime.noDependencies(), A).get();
  mpc.enc.remoteBinMatrix(mpc.runtime.noDependencies(), B).get();

  // C.mShares[0](0) = 0;
  // C.mShares[1](0) = 0;
  // auto task = mpc.runtime.noDependencies();
  // task.get();
  // mpc.binEval.setCir(cir, width, gen);
  // mpc.binEval.setInput(0, A); //设置第一个输入
  // mpc.binEval.setInput(1, B); //设置第二个输入
  // mpc.binEval.asyncEvaluate(task).get();
  // mpc.binEval.getOutput(0, C);
  // task.get();

  computeMSB(A, B, C, 2, mpc.gen, mpc.runtime, mpc.binEval);

  for (u64 i = 0; i < C.mShares[0].size(); i++)
    LOG(INFO) << "pIdx: " << pIdx << "==============> C[0][" << i
              << "]: " << C.mShares[0](i);
  for (u64 i = 0; i < C.mShares[1].size(); i++)
    LOG(INFO) << "pIdx: " << pIdx << "==============> C[1][" << i
              << "]: " << C.mShares[1](i);

  mpc.enc.reveal(mpc.runtime.noDependencies(), 0, C).get();
  // Sh3_BinaryEngine_test(cir, func, true, "msb", 2, mpc.enc, mpc.runtime,
  //                       mpc.binEval, mask);
  LOG(INFO) << "mission complete";
  mpc.fini();
  return;
}