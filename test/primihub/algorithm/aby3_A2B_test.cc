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
void full_adder_circuit(BetaCircuit *cd, BetaBundle a, BetaBundle b,
                        BetaBundle cin, BetaBundle s, BetaBundle cout) {
  u64 bitSize = a.mWires.size();
  BetaBundle tempWire(3);
  cd->addTempWireBundle(tempWire);
  for (u64 i = 0; i < bitSize; i++) {
    auto P = tempWire[0];
    auto G = tempWire[1];
    cd->addGate(a[i], b[i], GateType::Xor, P);
    cd->addGate(a[i], b[i], GateType::And, G);
    cd->addGate(P, cin[i], GateType::Xor, s[i]);
    cd->addGate(P, cin[i], GateType::And, tempWire[2]);
    cd->addGate(G, tempWire[2], GateType::Xor, cout[i]);
  }
}
void shift_circuit(BetaCircuit *cd, BetaBundle a, BetaBundle b) {
  u64 bitSize = a.mWires.size();
  for (u64 i = 1; i < bitSize; i++) {
    cd->addCopy(a[i - 1], b[i]);
  }
  cd->addConst(b[0], 0);
}
void int_int_add_kogge_stone(BetaCircuit *cd, BetaBundle a, BetaBundle b,
                             BetaBundle res) {
  u64 bitSize = a.mWires.size();
  BetaBundle t(bitSize * 2);
  cd->addTempWireBundle(t);
  BetaBundle PP, GG;
  auto &P = PP.mWires;
  auto &G = GG.mWires;
  P.insert(P.end(), t.mWires.begin(), t.mWires.begin() + bitSize);
  G.insert(G.end(), t.mWires.begin() + bitSize,
           t.mWires.begin() + bitSize * 2 - 1);
  auto tempWire = t.mWires.back();
  P[0] = res.mWires[0];
  for (u64 i = 0; i < bitSize; ++i) {
    cd->addGate(a.mWires[i], b.mWires[i], GateType::Xor, P[i]);
    if (i < bitSize - 1)
      cd->addGate(a.mWires[i], b.mWires[i], GateType::And, G[i]);
  }
  auto d = log2ceil(bitSize);
  for (u64 level = 0; level < d; ++level) {
    auto startPos = 1ull << level;
    auto low_step = 1ull << level;
    for (u64 curWire = bitSize - 1; curWire >= startPos; --curWire) {
      auto lowWire = curWire - low_step;
      auto P0 = P[lowWire];
      auto G0 = G[lowWire];
      auto P1 = P[curWire];
      if (curWire < bitSize - 1) {
        auto G1 = G[curWire];
        cd->addGate(P1, G0, GateType::And, tempWire);
        cd->addGate(tempWire, G1, GateType::Xor, G1);
      }
      if (curWire != startPos) {
        cd->addGate(P0, P1, GateType::And, P1);
      }
    }
  }
  for (u64 i = 1; i < bitSize; ++i) {
    cd->addGate(a.mWires[i], b.mWires[i], GateType::Xor, P[i]);
    cd->addGate(P[i], G[i - 1], GateType::Xor, res.mWires[i]);
  }
}
BetaCircuit *arith_to_bin_circuit(u64 bitSize) {
  auto *cd = new BetaCircuit;
  BetaBundle x(bitSize);
  BetaBundle y(bitSize);
  BetaBundle z(bitSize);
  BetaBundle res(bitSize);
  cd->addInputBundle(x);
  cd->addInputBundle(y);
  cd->addInputBundle(z);
  cd->addOutputBundle(res);
  BetaBundle c(bitSize), s(bitSize);
  cd->addTempWireBundle(c);
  cd->addTempWireBundle(s);
  full_adder_circuit(cd, x, y, z, s, c);
  BetaBundle _2c(bitSize);
  cd->addTempWireBundle(_2c);
  shift_circuit(cd, c, _2c);
  int_int_add_kogge_stone(cd, _2c, s, res);
  return cd;
}
void Sh3_convert_arith_to_bin_test() {
  IOService ios;
  auto chl01 =
      Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
  auto chl10 =
      Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
  auto chl02 =
      Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
  auto chl20 =
      Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
  auto chl12 =
      Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
  auto chl21 =
      Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();
  vector<std::shared_ptr<CommPkg>> comms = {
      std::make_shared<CommPkg>(chl02, chl01),
      std::make_shared<CommPkg>(chl10, chl12),
      std::make_shared<CommPkg>(chl21, chl20)};
  // CommPkg comms[3];
  // comms[0] = {chl02, chl01};
  // comms[1] = {chl10, chl12};
  // comms[2] = {chl21, chl20};
  Sh3Encryptor encs[3];
  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));
  Sh3BinaryEvaluator evals[3];
  u64 bitSize = 64;
  if (bitSize < 1 || 64 < bitSize) {
    throw std::runtime_error(LOCATION);
  }
  u64 mask = bitSize == 64 ? -1ull : (1ull << bitSize) - 1;

  auto routine = [&](int pIdx) {
    PRNG prng(
        toBlock(std::chrono::system_clock::now().time_since_epoch().count()));
    Sh3Runtime rt(pIdx, comms[pIdx]);
    auto task = rt.noDependencies();
    Sh3ShareGen gen;
    gen.init(toBlock(pIdx), toBlock((pIdx + 1) % 3));
    auto &eval = evals[pIdx];
    eval.mPrng.SetSeed(toBlock(pIdx));
    auto &enc = encs[pIdx];
    auto &comm = comms[pIdx];
    i64 rand_addition_ss;
    prng.get(&rand_addition_ss, 1);
    rand_addition_ss &= mask;

    f64<D16> fixedInt = 34.62;

    // We can encrypt this value in the similar way as an integer
    sf64<D16> sharedFixedInt;
    if (pIdx == 0)
      enc.localFixed(rt, fixedInt, sharedFixedInt).get();
    else
      enc.remoteFixed(rt, sharedFixedInt).get();

    LOG(INFO) << "pIdx" << pIdx << "===========>rand_addition_ss"
              << rand_addition_ss;
    si64 rand_replicated_ss = enc.localInt(*comm, rand_addition_ss);
    LOG(INFO) << "pIdx:" << pIdx
              << "===========>rand_replicated_ss[0]:" << rand_replicated_ss[0];
    LOG(INFO) << "pIdx:" << pIdx
              << "===========>rand_replicated_ss[1]:" << rand_replicated_ss[1];
    sbMatrix brss_x[3];
    for (int i = 0; i < 3; i++) {
      brss_x[i].resize(1, bitSize);
      brss_x[i].mShares[0].setZero();
      brss_x[i].mShares[1].setZero();
    }
    // brss_x[pIdx].mShares[0](0) = rand_replicated_ss[0];
    // brss_x[(pIdx + 2) % 3].mShares[1](0) = rand_replicated_ss[1];
    brss_x[pIdx].mShares[0](0) = sharedFixedInt[0];
    brss_x[(pIdx + 2) % 3].mShares[1](0) = sharedFixedInt[1];
    BetaCircuit *arith2bin = arith_to_bin_circuit(bitSize);
    arith2bin->levelByAndDepth();
    sbMatrix bool_rss(1, bitSize);
    LOG(INFO) << "Starting convert";
    eval.asyncEvaluate(task, arith2bin, gen,
                       {&brss_x[0], &brss_x[1], &brss_x[2]}, {&bool_rss})
        .get();
    LOG(INFO) << "finished convert";
    if (pIdx) {
      enc.reveal(task, 0, bool_rss).get();
      // enc.reveal(task, 0, rand_replicated_ss).get();
      enc.reveal(task, 0, sharedFixedInt).get();

    } else {
      i64Matrix plain_bool_rss(1, 1);
      f64Matrix<D16> plain_bool_rss_f64(1, 1);
      i64 plain_rss;
      f64<D16> plain_rss_f64;
      // enc.reveal(task, bool_rss, plain_bool_rss).get();
      enc.reveal(task, bool_rss, plain_bool_rss_f64.i64Cast()).get();
      // enc.reveal(task, rand_replicated_ss, plain_rss).get();
      enc.reveal(task, sharedFixedInt, plain_rss_f64).get();
      // if ((plain_rss & mask) != plain_bool_rss(0)) {
      //   throw std::runtime_error(LOCATION);
      // }
      // if ((plain_rss_f64.mValue & mask) != plain_bool_rss_f64(0)) {
      //   throw std::runtime_error(LOCATION);
      // }
      std::cout << "a_rss: " << plain_rss_f64 << ": ";
      // std::cout << "a_rss: " << ((u64)plain_rss & mask) << ": ";
      std::cout << "b_rss: " << plain_bool_rss_f64(0) << ": ";
      // std::cout << "b_rss: " << ((u64)plain_bool_rss(0)) << ": ";
      for (i64 i = bitSize - 1; i >= 0; i--) {
        std::cout << ((plain_bool_rss(0) >> i) & 1);
      }
      std::cout << std::endl;
    }
  };
  auto t0 = std::thread(routine, 0);
  auto t1 = std::thread(routine, 1);
  auto t2 = std::thread(routine, 2);
  t0.join();
  t1.join();
  t2.join();
}
TEST(arith_to_bin_test, Sh3_aby3_test) { Sh3_convert_arith_to_bin_test(); }
} // namespace primihub