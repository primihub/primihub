// Copyright [2021] <primihub.com>

#include <random>

#include "gtest/gtest.h"

#include "src/primihub/protocol/aby3/evaluator/piecewise.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/util/network/socket/ioservice.h"

// using namespace primihub;
namespace primihub {
TEST(PiecewiseTest, Sh3_Piecewise_plain_test) {
  u64 dec = 16;
  u64 n = 64;

  eMatrix<double> input(n, 1), output(n, 1);

  PRNG prng(ZeroBlock);
  std::normal_distribution<double> dist(0.0, 2.0);

  for (u64 i = 0; i < n; ++i) {
    input(i) = dist(prng);
  }

  {
    Sh3Piecewise maxZero;
    maxZero.mThresholds.resize(1, 0);
    maxZero.mCoefficients.resize(2);
    maxZero.mCoefficients[1].resize(2, 0);
    maxZero.mCoefficients[1][1] = 1;

    maxZero.eval(input, output, dec, false);

    for (u64 i = 0; i < n; ++i) {
      auto& out = output(i);
      auto& in = input(i);
      auto diff = std::abs(out - std::max(0.0, in));
      auto tol = 1.0 / (1 << dec);
      if (diff > tol)
        throw std::runtime_error(LOCATION);
    }
  }

  {
    Sh3Piecewise ll;
    ll.mThresholds.resize(2, 0);

    ll.mThresholds[0] = -0.5;
    ll.mThresholds[1] = 0.5;

    ll.mCoefficients.resize(3);
    ll.mCoefficients[1].resize(2);
    ll.mCoefficients[1][0] = 0.5;
    ll.mCoefficients[1][1] = 1;

    ll.mCoefficients[2].resize(1);
    ll.mCoefficients[2][0] = 1;

    ll.eval(input, output, dec);

    for (u64 i = 0; i < n; ++i) {
      auto& out = output(i);
      auto& in = input(i);
      auto diff = std::abs(out - std::min(std::max(0.0, in + 0.5), 1.0));
      auto tol = 1.0 / (1 << dec);
      if (diff > tol)
        throw std::runtime_error(LOCATION);
    }
  }
}

void createSharing(
  PRNG& prng,
  i64Matrix& value,
  si64Matrix& s0,
  si64Matrix& s1,
  si64Matrix& s2) {
  s0.mShares[0] = value;

  s1.mShares[0].resizeLike(value);
  s2.mShares[0].resizeLike(value);

  {
    prng.get(s1.mShares[0].data(), s1.mShares[0].size());
    prng.get(s2.mShares[0].data(), s2.mShares[0].size());

    for (u64 i = 0; i < s0.mShares[0].size(); ++i)
      s0.mShares[0](i) -= s1.mShares[0](i) + s2.mShares[0](i);
  }

  s0.mShares[1] = s2.mShares[0];
  s1.mShares[1] = s0.mShares[0];
  s2.mShares[1] = s1.mShares[0];
}

TEST(PiecewiseTest, Sh3_Piecewise_test) {
  IOService ios;
  auto e01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01");
  auto e10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01");
  auto e02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02");
  auto e20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02");
  auto e12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12");
  auto e21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12");

  auto chl01 = e01.addChannel();
  auto dchl01 = e01.addChannel();
  auto chl10 = e10.addChannel();
  auto dchl10 = e10.addChannel();
  auto chl02 = e02.addChannel();
  auto dchl02 = e02.addChannel();
  auto chl20 = e20.addChannel();
  auto dchl20 = e20.addChannel();
  auto chl12 = e12.addChannel();
  auto dchl12 = e12.addChannel();
  auto chl21 = e21.addChannel();
  auto dchl21 = e21.addChannel();

  CommPkg comms[3], dcomms[3];
  comms[0] = { chl02, chl01 };
  dcomms[0] = { dchl02, dchl01 };
  comms[1] = { chl10, chl12 };
  dcomms[1] = { dchl10, dchl12 };
  comms[2] = { chl21, chl20 };
  dcomms[2] = { dchl21, dchl20 };

  u64 size = 2;
  u64 trials = 1;
  u64 trials2 = 4;
  u64 numThresholds = 4;
  // u64 dec = 16;
  const Decimal dec = Decimal::D16;
  // sf64Matrix<dec> p0, p1, p2;
  Sh3Runtime p0, p1, p2;
  Sh3Evaluator ev0, ev1, ev2;

  p0.init(0, dynamic_cast<CommPkgBase*>(&comms[0]));
  p1.init(1, dynamic_cast<CommPkgBase*>(&comms[1]));
  p2.init(2, dynamic_cast<CommPkgBase*>(&comms[2]));

  ev0.init(0, toBlock(3), toBlock(1));
  ev1.init(1, toBlock(1), toBlock(2));
  ev2.init(2, toBlock(2), toBlock(3));

  PRNG prng(ZeroBlock);
  std::normal_distribution<double> dist(3.0);

  for (u64 t = 0; t < trials; ++t) {
    Sh3Piecewise pw0, pw1, pw2;

    pw0.DebugRt.init(0, dynamic_cast<CommPkgBase*>(&dcomms[0]));
    pw1.DebugRt.init(1, dynamic_cast<CommPkgBase*>(&dcomms[1]));
    pw2.DebugRt.init(2, dynamic_cast<CommPkgBase*>(&dcomms[2]));

    pw0.DebugEnc.init(0, ZeroBlock, ZeroBlock);
    pw1.DebugEnc.init(1, ZeroBlock, ZeroBlock);
    pw2.DebugEnc.init(2, ZeroBlock, ZeroBlock);

    pw0.mCoefficients.resize(numThresholds + 1);
    pw1.mCoefficients.resize(numThresholds + 1);
    pw2.mCoefficients.resize(numThresholds + 1);

    std::vector<double> tt(numThresholds);

    for (u64 h = 0; h < numThresholds + 1; ++h) {
      auto degree = i64(prng.get<u64>() % 3) - 1;

      pw0.mCoefficients[h].resize(degree + 1);
      pw1.mCoefficients[h].resize(degree + 1);
      pw2.mCoefficients[h].resize(degree + 1);

      if (degree >= 0) {
        auto coef = dist(prng);
        pw0.mCoefficients[h][0] = coef;
        pw1.mCoefficients[h][0] = coef;
        pw2.mCoefficients[h][0] = coef;
      }

      if (degree >= 1) {
        auto coef = prng.get<i64>() % 100;
        pw0.mCoefficients[h][1] = coef;
        pw1.mCoefficients[h][1] = coef;
        pw2.mCoefficients[h][1] = coef;
      }

      if (h != numThresholds) {
        tt[h] = dist(prng);
      }
    }
    std::sort(tt.begin(), tt.end());

    pw0.mThresholds.resize(numThresholds);
    pw1.mThresholds.resize(numThresholds);
    pw2.mThresholds.resize(numThresholds);
    for (u64 h = 0; h < numThresholds; ++h) {
      pw0.mThresholds[h] = tt[h];
      pw1.mThresholds[h] = tt[h];
      pw2.mThresholds[h] = tt[h];
    }

    for (u64 t2 = 0; t2 < trials2; ++t2) {
      eMatrix<double>
        plain_input(size, 1),
        plain_output(size, 1);

      i64Matrix
        word_input(size, 1),
        word_output(size, 1);

      si64Matrix
        input0(size, 1),
        input1(size, 1),
        input2(size, 1),
        output0(size, 1),
        output1(size, 1),
        output2(size, 1);

      for (u64 i = 0; i < size; ++i) {
        plain_input(i) = dist(prng);
        word_input(i) = (i64)(plain_input(i) * (1ull << dec));
      }
      createSharing(prng, word_input, input0, input1, input2);

      pw0.eval(plain_input, plain_output, dec);

      for (u64 i = 0; i < size; ++i)
        word_output(i) = (i64)(plain_output(i) * (1ull << dec));

      p0.mPrint = false;

      auto thrd0 = std::thread([&]() {
        pw0.eval(p0, input0, output0, dec, ev0, true).get();
        });
      auto thrd1 = std::thread([&]() {
        pw1.eval(p1, input1, output1, dec, ev1, true).get();
      });
      auto thrd2 = std::thread([&]() {
        pw2.eval(p2, input2, output2, dec, ev2, true).get();
      });

      thrd0.join();
      thrd1.join();
      thrd2.join();

      if (output0.mShares[0] != output1.mShares[1])
        throw std::runtime_error(LOCATION);
      if (output1.mShares[0] != output2.mShares[1])
        throw std::runtime_error(LOCATION);
      if (output2.mShares[0] != output0.mShares[1])
        throw std::runtime_error(LOCATION);

      auto out = (output0.mShares[0] + output0.mShares[1]
        + output1.mShares[0]).eval();

      for (u64 i = 0; i < size; ++i) {
        auto actual = out(i);
        auto exp = word_output(i);
        auto diff = std::abs(actual - exp);

        if (diff > 100) {
          std::cout << t << " " << t2 << " " << i << std::endl;
          std::cout << exp << " " << actual << " " << diff << std::endl;
          throw std::runtime_error(LOCATION);
        }
      }
    }
  }
}

TEST(PiecewiseTest, Sh3_Piecewise_compare_test) {
  IOService ios;
  auto e01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01");
  auto e10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01");
  auto e02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02");
  auto e20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02");
  auto e12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12");
  auto e21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12");

  auto chl01 = e01.addChannel();
  auto dchl01 = e01.addChannel();
  auto chl10 = e10.addChannel();
  auto dchl10 = e10.addChannel();
  auto chl02 = e02.addChannel();
  auto dchl02 = e02.addChannel();
  auto chl20 = e20.addChannel();
  auto dchl20 = e20.addChannel();
  auto chl12 = e12.addChannel();
  auto dchl12 = e12.addChannel();
  auto chl21 = e21.addChannel();
  auto dchl21 = e21.addChannel();

  CommPkg comms[3], dcomms[3];
  comms[0] = { chl02, chl01 };
  dcomms[0] = { dchl02, dchl01 };
  comms[1] = { chl10, chl12 };
  dcomms[1] = { dchl10, dchl12 };
  comms[2] = { chl21, chl20 };
  dcomms[2] = { dchl21, dchl20 };

  u64 size = 1;
  u64 trials = 1;
  u64 trials2 = 4;
  u64 numThresholds = 1;
  // u64 dec = 16;
  const Decimal dec = Decimal::D16;
  // sf64Matrix<dec> p0, p1, p2;
  Sh3Runtime p0, p1, p2;
  Sh3Evaluator ev0, ev1, ev2;

  p0.init(0, dynamic_cast<CommPkgBase*>(&comms[0]));
  p1.init(1, dynamic_cast<CommPkgBase*>(&comms[1]));
  p2.init(2, dynamic_cast<CommPkgBase*>(&comms[2]));

  ev0.init(0, toBlock(3), toBlock(1));
  ev1.init(1, toBlock(1), toBlock(2));
  ev2.init(2, toBlock(2), toBlock(3));

  PRNG prng(ZeroBlock);
  std::normal_distribution<double> dist(3.0);

  for (u64 t = 0; t < trials; ++t) {
    Sh3Piecewise pw0, pw1, pw2;

    pw0.DebugRt.init(0, dynamic_cast<CommPkgBase*>(&dcomms[0]));
    pw1.DebugRt.init(1, dynamic_cast<CommPkgBase*>(&dcomms[1]));
    pw2.DebugRt.init(2, dynamic_cast<CommPkgBase*>(&dcomms[2]));

    pw0.DebugEnc.init(0, ZeroBlock, ZeroBlock);
    pw1.DebugEnc.init(1, ZeroBlock, ZeroBlock);
    pw2.DebugEnc.init(2, ZeroBlock, ZeroBlock);

    pw0.mCoefficients.resize(numThresholds + 1);
    pw1.mCoefficients.resize(numThresholds + 1);
    pw2.mCoefficients.resize(numThresholds + 1);

    std::vector<double> tt(numThresholds);

    for (u64 h = 0; h < numThresholds + 1; ++h) {
      auto degree = i64(0);

      pw0.mCoefficients[h].resize(degree + 1);
      pw1.mCoefficients[h].resize(degree + 1);
      pw2.mCoefficients[h].resize(degree + 1);

      if (h == 0) {
        pw0.mCoefficients[h][0] = 1;
        pw1.mCoefficients[h][0] = 1;
        pw2.mCoefficients[h][0] = 1;
      } else {
        pw0.mCoefficients[h][0] = 0;
        pw1.mCoefficients[h][0] = 0;
        pw2.mCoefficients[h][0] = 0;
      }
    }
    tt[0] = 0;
    std::sort(tt.begin(), tt.end());

    pw0.mThresholds.resize(numThresholds);
    pw1.mThresholds.resize(numThresholds);
    pw2.mThresholds.resize(numThresholds);
    for (u64 h = 0; h < numThresholds; ++h) {
      pw0.mThresholds[h] = tt[h];
      pw1.mThresholds[h] = tt[h];
      pw2.mThresholds[h] = tt[h];   // 比较目标是常量；
    }
    for (u64 t2 = 0; t2 < trials2; ++t2) {
      eMatrix<double>
              plain_input(size, 1),
              plain_output(size, 1);

      i64Matrix
              word_input(size, 1),
              word_output(size, 1);

      si64Matrix
              input0(size, 1),
              input1(size, 1),
              input2(size, 1),
              output0(size, 1),
              output1(size, 1),
              output2(size, 1);
      sbMatrix
              output01(size, 1),
              output11(size, 1),
              output21(size, 1);

      for (u64 i = 0; i < size; ++i) {
        plain_input(i) = 2 - (prng.get<u64>() % 5);
        // this value should input by caller;
        word_input(i) = (i64)(plain_input(i) * (1ull << dec));
      }
      createSharing(prng, word_input, input0, input1, input2);

      pw0.eval(plain_input, plain_output, dec);

      for (u64 i = 0; i < size; ++i)
        word_output(i) = (i64)(plain_output(i) * (1ull << dec));

      p0.mPrint = false;

      auto thrd0 = std::thread([&]() {
        pw0.eval(p0, input0, output0, dec, ev0, true).get();
      });
      auto thrd1 = std::thread([&]() {
        pw1.eval(p1, input1, output1, dec, ev1, true).get();
      });
      auto thrd2 = std::thread([&]() {
        pw2.eval(p2, input2, output2, dec, ev2, true).get();
      });

      thrd0.join();
      thrd1.join();
      thrd2.join();

      if (output0.mShares[0] != output1.mShares[1])
        throw std::runtime_error(LOCATION);
      if (output1.mShares[0] != output2.mShares[1])
        throw std::runtime_error(LOCATION);
      if (output2.mShares[0] != output0.mShares[1])
        throw std::runtime_error(LOCATION);

      auto out = (output0.mShares[0] + output0.mShares[1]
        + output1.mShares[0]).eval();

      for (u64 i = 0; i < size; ++i) {
        auto actual = out(i);
        auto exp = word_output(i);
        auto diff = std::abs(actual - exp);

        if (diff != 0) {
          std::cout << t << " " << t2 << " " << i << std::endl;
          std::cout << exp << " " << actual << " " << diff
            << std::endl;
          throw std::runtime_error(LOCATION);
        }
        EXPECT_EQ(exp, actual);
      }
    }
  }
}

} //namespace primihub
