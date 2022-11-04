// Copyright [2021] <primihub.com>
#include <iomanip>
#include <random>
#include <string>
#include <vector>
#pragma once
#include "gtest/gtest.h"

#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/primitive/circuit/circuit_library.h"
#include "src/primihub/primitive/ppa/kogge_stone.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/binary_evaluator.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/bit_vector.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/ioservice.h"

namespace primihub {

#define GCOUT std::cerr << "[          ] [ INFO ]"

std::array<Matrix<i64>, 3> getShares(sbMatrix &S, Sh3Runtime &rt,
                                     CommPkg &comm) {
  std::array<Matrix<i64>, 3> r;

  auto &r0 = r[rt.mPartyIdx];
  auto &r1 = r[(rt.mPartyIdx + 1) % 3];
  auto &r2 = r[(rt.mPartyIdx + 2) % 3];

  r0 = S.mShares[0];
  r1 = S.mShares[1];

  comm.mNext().asyncSend(r0.data(), r0.size());
  comm.mPrev().asyncSend(r1.data(), r1.size());

  r2.resize(S.rows(), S.i64Cols());

  comm.mNext().recv(r2.data(), r2.size());
  Matrix<i64> check(r1.rows(), r1.cols());
  comm.mPrev().recv(check.data(), check.size());

  if (memcmp(check.data(), r1.data(), check.size() * sizeof(i64)) != 0)
    throw RTE_LOC;

  return r;
}
void Sh3_BinaryEngine_test(
    BetaCircuit *cir,
    std::function<fp<i64, D8>(fp<i64, D8>, fp<i64, D8>)> binOp, bool debug,
    std::string opName, u64 valMask = ~0ull) {
  IOService ios;
  Session s01(ios, "127.0.0.1", SessionMode::Server, "01");
  Session s10(ios, "127.0.0.1", SessionMode::Client, "01");
  Session s02(ios, "127.0.0.1", SessionMode::Server, "02");
  Session s20(ios, "127.0.0.1", SessionMode::Client, "02");
  Session s12(ios, "127.0.0.1", SessionMode::Server, "12");
  Session s21(ios, "127.0.0.1", SessionMode::Client, "12");

  Channel chl01 = s01.addChannel("c");
  Channel chl10 = s10.addChannel("c");
  Channel chl02 = s02.addChannel("c");
  Channel chl20 = s20.addChannel("c");
  Channel chl12 = s12.addChannel("c");
  Channel chl21 = s21.addChannel("c");

  vector<std::shared_ptr<CommPkg>> comms = {
      std::make_shared<CommPkg>(chl02, chl01),
      std::make_shared<CommPkg>(chl10, chl12),
      std::make_shared<CommPkg>(chl21, chl20)};

  vector<std::shared_ptr<CommPkg>> debugComm = {
      std::make_shared<CommPkg>(comms[0]->mPrev().getSession().addChannel(),
                                comms[0]->mNext().getSession().addChannel()),
      std::make_shared<CommPkg>(comms[1]->mPrev().getSession().addChannel(),
                                comms[1]->mNext().getSession().addChannel()),
      std::make_shared<CommPkg>(comms[2]->mPrev().getSession().addChannel(),
                                comms[2]->mNext().getSession().addChannel())};

  cir->levelByAndDepth();
  // u64 width = 1 << 8;
  u64 width = 2;
  bool failed = false;
  // bool manual = false;

  enum Mode { Manual, Auto, Replicated };

  std::array<std::vector<Matrix<i64>>, 3> CC;
  std::array<std::vector<Matrix<i64>>, 3> CC2;
  std::array<std::atomic<int>, 3> ac;
  Sh3BinaryEvaluator evals[3];

  // block tag = sysRandomSeed();

  ac[0] = 0;
  ac[1] = 0;
  ac[2] = 0;
  auto aSize = cir->mInputs[0].size(); //为8，有8根线
  auto bSize = cir->mInputs[1].size();
  auto cSize = cir->mOutputs[0].size(); //为1，有1根线
  LOG(INFO) << "============> aSize: " << aSize;
  LOG(INFO) << "============> bSize: " << bSize;
  LOG(INFO) << "============> cSize: " << cSize;

  auto routine = [&](int pIdx) {
    // auto i = 0;

    // i64Matrix a(width, 1), b(width, 1), c(width, 1), ar(1, 1); // 2行1列
    // i64Matrix aa(width, 1), bb(width, 1);

    PRNG prng(ZeroBlock);
    // for (u64 i = 0; i < a.size(); ++i) {
    //   a(i) = prng.get<i64>() & valMask;
    //   b(i) = prng.get<i64>() & valMask;
    // }
    f64Matrix<D8> a(width, 1), b(width, 1), ar(1, 1); // 2行1列
    // f64Matrix<D8> c(width, 1);
    i64Matrix c(width, 1);
    f64Matrix<D8> aa(width, 1), bb(width, 1);
    for (u64 i = 0; i < a.size(); ++i) {
      a(i) = 2.1 + 10 * i;
      b(i) = -10.5;
    }
    ar(0) = prng.get<i64>();
    LOG(INFO) << "partyID: " << pIdx << "============> a: " << a << a.i64Cast();
    LOG(INFO) << "partyID: " << pIdx << "============> b: " << b << b.i64Cast();
    LOG(INFO) << "partyID: " << pIdx << "============> ar: " << ar;
    Sh3Runtime rt(pIdx, comms[pIdx]);

    sbMatrix A(width, aSize), B(width, bSize), // A是2行
        C(width, cSize);                       //分享值矩阵，2行,1列
    sbMatrix Ar(1, aSize);
    LOG(INFO) << "pIdx: " << pIdx
              << "==============>C.i64Size: " << C.i64Size();
    LOG(INFO) << "pIdx: " << pIdx << "==============>C.rows: " << C.rows();
    LOG(INFO) << "pIdx: " << pIdx
              << "==============>C.i64Cols: " << C.i64Cols();
    LOG(INFO) << "pIdx: " << pIdx
              << "==============>C.bitCount: " << C.bitCount();
    Sh3Encryptor enc;
    enc.init(pIdx, toBlock(pIdx), toBlock((pIdx + 1) % 3));

    auto task = rt.noDependencies();
    //通过此处构造分享
    enc.localBinMatrix(rt.noDependencies(), a.i64Cast(), A).get();
    enc.localBinMatrix(rt.noDependencies(), ar.i64Cast(), Ar).get();
    enc.localBinMatrix(rt.noDependencies(), b.i64Cast(), B).get();

    auto &eval = evals[pIdx];

    eval.mPrng.SetSeed(toBlock(pIdx));

#ifdef BINARY_ENGINE_DEBUG
    if (debug)
      eval.enableDebug(pIdx, debugComm[pIdx]->mPrev(),
                       debugComm[pIdx]->mNext());
#endif

    for (auto mode : {Manual, Auto}) {
      Sh3ShareGen gen;
      gen.init(toBlock(pIdx), toBlock((pIdx + 1) % 3));

      C.mShares[0](0) = 0;
      C.mShares[1](0) = 0;

      switch (mode) {
      case Manual:
        task.get();
        eval.setCir(cir, width, gen);
        eval.setInput(0, A); //设置第一个输入
        eval.setInput(1, B); //设置第二个输入
        eval.asyncEvaluate(rt.noDependencies()).get();
        eval.getOutput(0, C);
        break;
      case Auto:
        task = eval.asyncEvaluate(task, cir, gen, {&A, &B}, {&C});
        break;
      case Replicated:
        for (u64 i = 0; i < width; ++i)
          a(i) = ar(0);

        task.get();
        eval.setCir(cir, width, gen);
        eval.setReplicatedInput(0, Ar);
        eval.setInput(1, B);
        eval.asyncEvaluate(rt.noDependencies()).get();
        eval.getOutput(0, C);
        break;
      }
      task.get();
      //   i64Matrix tmp0 = C.mShares[0];
      //   eMatrix<i64> tmp1 = C.mShares[1];

      for (u64 i = 0; i < C.mShares[0].size(); i++)
        LOG(INFO) << "pIdx: " << pIdx << "==============> C[0][" << i
                  << "]: " << C.mShares[0](i);
      for (u64 i = 0; i < C.mShares[1].size(); i++)
        LOG(INFO) << "pIdx: " << pIdx << "==============> C[1][" << i
                  << "]: " << C.mShares[1](i);
      //   LOG(INFO) << "C[0]: " << C.mShares[0];
      //   LOG(INFO) << "C[1]: " <<  C.mShares[1];
      auto ccc0 = C.mShares[0](0);
      CC[pIdx].push_back(C.mShares[0]);
      CC2[pIdx].push_back(C.mShares[1]);
      ++ac[pIdx];
      auto ccc1 = C.mShares[0](0);
      auto ccc2 = CC[pIdx].back()(0);

      if (pIdx) {
        enc.reveal(task, 0, C).get();
        // auto CC = getShares(C, rt, debugComm[pIdx]);
      } else {
        enc.reveal(task, C, c).get();
        auto ci = CC[pIdx].size() - 1;
        LOG(INFO) << "ID: " << pIdx << "------------>mode:" << mode
                  << "==============> c:" << c;

        for (u64 j = 0; j < width; ++j) {
          if (c(j) != binOp(a(j), b(j))) {
            GCOUT << "pidx: " << rt.mPartyIdx << " mode: " << (int)mode
                  << " debug:" << int(debug) << " failed at " << j << " "
                  << std::hex << c(j) << " != " << std::hex << binOp(a(j), b(j))
                  << " = " << opName << "(" << std::hex << a(j) << ", "
                  << std::hex << b(j) << ")" << std::endl
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
          GCOUT << "      C " << CC[0][ci](0) << " " << CC[1][ci](0) << " "
                << CC[2][ci](0) << std::endl;
          GCOUT << "      D " << CC2[0][ci](0) << " " << CC2[1][ci](0) << " "
                << CC2[2][ci](0) << std::endl;
          GCOUT << "        " << ccc0 << " " << ccc1 << " " << ccc2
                << std::endl;
          GCOUT << "   recv " << eval.mRecvFutr.size() << std::endl;

          GCOUT << "   " << eval.mLog.str() << std::endl;

          GCOUT << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
          GCOUT << evals[1].mLog.str() << std::endl;
          GCOUT << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
          GCOUT << evals[2].mLog.str() << std::endl;
        }
      }

      // lout << Color::Green << eval.mLog.str() << Color::Default <<
      // std::endl;
    }
  };

  auto t0 = std::thread(routine, 0);
  auto t1 = std::thread(routine, 1);
  auto t2 = std::thread(routine, 2);

  t0.join();
  t1.join();
  t2.join();

  if (failed)
    throw std::runtime_error(LOCATION);
}

// TEST(BinaryEvaluatorTest, Sh3_BinaryEngine_add_test) {
//   KoggeStoneLibrary lib;
//   u64 size = 64;
//   u64 mask = ~((~0ull) << size);
//   u64 mask1 = ~0ull;

//   std::function<i64(i64, i64)> func = [mask](i64 a, i64 b) {
//     return (a + b) & mask;
//   };

//   BetaCircuit *cir = lib.int_int_add(size, size, size);

//   Sh3_BinaryEngine_test(cir, func, true, "Plus", mask);
//   // Sh3_BinaryEngine_test(cir, func, false, "Plus", mask);
// } // namespace primihub

TEST(BinaryEvaluatorTest, Sh3_BinaryEngine_add_msb_test) {
  KoggeStoneLibrary lib;
  // u64 size = 8;
  u64 size = 64;
  // u64 mask = ~((~0ull) << size);
  u64 mask = ~0ull;

  // std::function<i64(i64, i64)> func = [size, mask](i64 a, i64 b) {
  //   return ((a + b) >> (size - 1)) & 1;
  // };
  std::function<i64(fp<i64, D8>, fp<i64, D8>)> func =
      [size, mask](fp<i64, D8> a, fp<i64, D8> b) {
        return ((a + b) >> (size - 1)).mValue & 1;
      };
  BetaCircuit *cir = lib.int_int_add_msb(size);

  Sh3_BinaryEngine_test(cir, func, true, "msb", mask);
  //   Sh3_BinaryEngine_test(cir, func, false, "msb", mask);
}
} // namespace primihub
