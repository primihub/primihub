// Copyright [2021] <primihub.com>
#include <string>
#include <vector>
#include <iomanip>

#include "gtest/gtest.h"

#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/crypto/bit_vector.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/channel.h"


namespace primihub {

#define _DEBUG 1

void rand(i64Matrix& A, PRNG& prng) {
  prng.get(A.data(), A.size());
}

TEST(EvaluatorTest, Sh3_Evaluator_asyncMul_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
                       "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
                       "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
                       "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
                       "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
                       "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
                       "12").addChannel();

  int trials = 10;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };
  
  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  bool failed = false;
  auto t0 = std::thread([&]() {
    auto& enc = encs[0];
    auto& eval = evals[0];
    auto comm = comms[0];
    Sh3Runtime rt;
    rt.init(0,comm);

    PRNG prng(ZeroBlock);

    for (u64 i = 0; i < trials; ++i) {
      i64Matrix a(trials, trials), b(trials, trials), c(trials, trials),
                cc(trials, trials);
      rand(a, prng);
      rand(b, prng);
      si64Matrix A(trials, trials), B(trials, trials), C(trials, trials);

      enc.localIntMatrix(*comm.get(), a, A);
      enc.localIntMatrix(*comm.get(), b, B);

      auto task = rt.noDependencies();

      for (u64 j = 0; j < trials; ++j) {
        task = eval.asyncMul(task, A, B, C);
        task = task.then([&](CommPkgBase* comm, Sh3Task& self) { A = C + A; });
      }

      task.get();

      enc.reveal(*comm.get(), C, cc);

      for (u64 j = 0; j < trials; ++j) {
        c = a * b;
        a = c + a;
      }

      if (c != cc) {
          failed = true;
          std::cout << c << std::endl;
          std::cout << cc << std::endl;
      }
    }
  });

  auto rr = [&](int i) {
    auto& enc = encs[i];
    auto& eval = evals[i];
    auto comm = comms[i];
    Sh3Runtime rt;
    rt.init(i, comm);

    for (u64 i = 0; i < trials; ++i) {
      si64Matrix A(trials, trials), B(trials, trials), C(trials, trials);
      enc.remoteIntMatrix(*comm.get(), A);
      enc.remoteIntMatrix(*comm.get(), B);

      auto task = rt.noDependencies();
      for (u64 j = 0; j < trials; ++j) {
        task = eval.asyncMul(task, A, B, C);
        task = task.then([&](CommPkgBase* comm, Sh3Task& self) { A = C + A; });
      }
      task.get();

      enc.reveal(*comm.get(), 0, C);
    }
  };

  auto t1 = std::thread(rr, 1);
  auto t2 = std::thread(rr, 2);

  t0.join();
  t1.join();
  t2.join();

  if (failed)
    throw std::runtime_error(LOCATION);
}


std::string prettyShare(u64 partyIdx, const si64& v) {
  std::array<u64, 3> shares;
  shares[partyIdx] = v[0];
  shares[(partyIdx + 2) % 3] = v[1];
  shares[(partyIdx + 1) % 3] = -1;

  std::stringstream ss;
  ss << "(";
  if (shares[0] == -1)
    ss << "               _ ";
  else
    ss << std::hex << std::setw(16) << std::setfill('0') << shares[0] << " ";

  if (shares[1] == -1)
    ss << "               _ ";
  else
    ss << std::hex << std::setw(16) << std::setfill('0') << shares[1] << " ";

  if (shares[2] == -1)
    ss << "               _)";
  else
    ss << std::hex << std::setw(16) << std::setfill('0') << shares[2] << ")";

  return ss.str();
}

TEST(EvaluatorTest, Sh3_Evaluator_asyncMul_fixed_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  int trials = 1;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };
  
  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  bool failed = false;
  auto t0 = std::thread([&]() {
    auto& enc = encs[0];
    auto& eval = evals[0];
    auto comm = comms[0];
    Sh3Runtime rt;
    rt.init(0, comm);

    PRNG prng(ZeroBlock);

    for (u64 i = 0; i < trials; ++i) {
      f64<D8> a = (prng.get<i32>() >> 8) / 100.0;
      f64<D8> b = (prng.get<i32>() >> 8) / 100.0;
      f64<D8> c;

      sf64<D8> A;
      enc.localFixed(rt, a, A);
      sf64<D8> B;
      enc.localFixed(rt, b, B).get();
      sf64<D8> C;
      auto task = rt.noDependencies(); {
        task = eval.asyncMul(task, A, B, C);
      }

      f64<D8> cc, aa;
      enc.reveal(task, A, aa).get();
      enc.reveal(task, C, cc).get();

      {
        c = a * b;
      }

      auto diff0 = (c - cc);
      double diff1 = diff0.mValue / double(1ull << c.mDecimal);
      double diff2 = static_cast<double>(diff0);

      if (std::abs(diff1) > 0.01) {
        failed = true;
      }
    }
  });

  auto rr = [&](int i) {
    auto& enc = encs[i];
    auto& eval = evals[i];
    auto comm = comms[i];
    Sh3Runtime rt;
    rt.init(i, comm);

    for (u64 i = 0; i < trials; ++i) {
      sf64<D8> A;
      enc.remoteFixed(rt, A);
      sf64<D8> B;
      enc.remoteFixed(rt, B).get();
      sf64<D8> C;

      auto task = rt.noDependencies();
      {
        task = eval.asyncMul(task, A, B, C);
      }

      enc.reveal(task, 0, A).get();
      enc.reveal(task, 0, C).get();
    }
  };

  auto t1 = std::thread(rr, 1);
  auto t2 = std::thread(rr, 2);

  t0.join();
  t1.join();
  t2.join();

  if (failed)
    throw std::runtime_error(LOCATION);
}

void sync(CommPkg& comm) {
  char c;
  comm.mNext().send(c);
  comm.mPrev().send(c);
  comm.mNext().recv(c);
  comm.mPrev().recv(c);
}

f64Matrix<D8> reveal(sf64Matrix<D8>& A, CommPkg& comm) {
  comm.mPrev().asyncSend(A[0].data(), A.size());
  comm.mNext().asyncSend(A[0].data(), A.size());
  f64Matrix<D8> ret(A.rows(), A.cols());

  f64Matrix<D8> A1(A.rows(), A.cols());

  comm.mNext().recv((i64*)ret.mData.data(), ret.size());
  comm.mPrev().recv((i64*)A1.mData.data(), A1.size());

  if (A1.i64Cast() != A[1]) {
    lout << "A1  " << A1.i64Cast() << "\nA1' " << A[1]
      << std::endl;
    throw std::runtime_error("inconsistent shares. " LOCATION);
  }

  for (u64 i = 0; i < ret.size(); ++i) {
    ret(i).mValue += A[0](i) + A[1](i);
  }
  return ret;
}


i64Matrix reveal(si64Matrix& A0, si64Matrix& A1, si64Matrix& A2) {
  if (A0[0] != A1[1])
    throw RTE_LOC;
  if (A1[0] != A2[1])
    throw RTE_LOC;
  if (A2[0] != A0[1])
    throw RTE_LOC;

  i64Matrix ret(A0.rows(), A0.cols());
  ret = A0[0] + A1[0] + A2[0];
  return ret;
}

TEST(EvaluatorTest, Sh3_Evaluator_truncationPai_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  int size = 4;
  int trials = 1000;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };

  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  auto dec = Decimal::D8;

  for (u64 t = 0; t < trials; ++t) {
    auto t0 = evals[0].getTruncationTuple(size, size, dec);
    auto t1 = evals[1].getTruncationTuple(size, size, dec);
    auto t2 = evals[2].getTruncationTuple(size, size, dec);

    auto tr = reveal(t0.mRTrunc, t1.mRTrunc, t2.mRTrunc);
    auto r = (t0.mR + t1.mR + t2.mR);

    for (u64 i = 0; i < r.size(); ++i) {
      auto exp = r(i) >> dec;
      auto exp1 = exp - 4;
      auto exp2 = exp + 4;
      if (tr(i) <= exp1 || tr(i) >= exp2) {
        std::cout << "tr " << std::hex << tr(i) << " r1 "
          << std::hex << exp << std::endl;
        std::cout << "tr " << std::hex <<tr(i) << " r2 "
          << std::hex << exp2 << std::endl;
        throw RTE_LOC;
      }
    }
  }
}

TEST(EvaluatorTest, Sh3_Evaluator_asyncMul_matrixFixed_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  int size = 4;
  int trials = 4;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };

  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  sf64Matrix<D8> A[3];
  sf64Matrix<D8> B[3];
  sf64Matrix<D8> C[3];

  bool failed = false;

  auto routine = [&](int idx) {
    auto& enc = encs[idx];
    auto& eval = evals[idx];
    auto comm = comms[idx];
    Sh3Runtime rt;
    rt.init(idx, comm);

    eval.DEBUG_disable_randomization = true;

    for (u64 tt = 0; tt < trials; ++tt) {
      PRNG prng(toBlock(tt));

      f64Matrix<D8> a(size, size);
      f64Matrix<D8> b(size, size);
      f64Matrix<D8> c;

      for (u64 i = 0; i < a.size(); ++i)
        a(i) = (prng.get<u32>() >> 8) / 100.0;
      for (u64 i = 0; i < b.size(); ++i)
        b(i) = (prng.get<u32>() >> 8) / 100.0;
      c = a * b;
      auto c64 = a.i64Cast() * b.i64Cast();

      A[idx][0].resize(size, size);
      A[idx][1].resize(size, size);
      B[idx][0].resize(size, size);
      B[idx][1].resize(size, size);
      A[idx][0].setZero();
      A[idx][1].setZero();
      B[idx][0].setZero();
      B[idx][1].setZero();

      if (idx == 0) {
        enc.localFixedMatrix(rt, a, A[idx]).get();
        f64Matrix<D8> aa;
        sync(dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));
        enc.revealAll(rt, A[idx], aa).get();
        auto aa2 = reveal(A[idx], dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));
        if (aa.i64Cast() != a.i64Cast()) {
          std::cout << " failure A1 " << std::endl;
          std::cout << "a   " << a << std::endl;
          std::cout << "aa  " << aa << std::endl;
        }
        if (aa2.i64Cast() != a.i64Cast()) {
          std::cout << " failure A2 " << std::endl;
          std::cout << "aa2 " << aa2 << std::endl;
        }
        sync(dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));

        enc.localFixedMatrix(rt, b, B[idx]).get();
      } else {
        enc.remoteFixedMatrix(rt, A[idx]).get();
        f64Matrix<D8> aa;
        sync(dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));
        enc.revealAll(rt, A[idx], aa).get();
        reveal(A[idx], dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));

        sync(dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));

        enc.remoteFixedMatrix(rt, B[idx]).get();
      }

      auto task = rt.noDependencies();
      task = eval.asyncMul(task, A[idx], B[idx], C[idx]);

      f64Matrix<D8> cc, aa;
      enc.revealAll(task, A[idx], aa).get();

      sync(dynamic_cast<CommPkg&>(*rt.mCommPtr.get()));

      // enc.revealAll(task, C[idx], cc).get();
      cc = reveal(C[idx], dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));
      auto aa2 = reveal(A[idx], dynamic_cast<CommPkg&> (*rt.mCommPtr.get()));

      if (idx == 0) {
        if (aa2.i64Cast() != a.i64Cast()) {
          std::cout << " failure AA* " << std::endl;
        }

        if (aa.i64Cast() != a.i64Cast()) {
          std::cout << " failure AA " << std::endl;
        }

        f64Matrix<D8> diffMat(c - cc);

        for (u64 i = 0; i < diffMat.size(); ++i) {
          // fp<i64,D8>::ref r = diffMat(i);
          f64<D8> diff = diffMat(i);
          // double diff2 = diff / double(1ull << D8);

          if (diff.mValue > 1 || diff.mValue < -1) {
            failed = true;
            lout << Color::Red << "======= FAILED ======="
              << std::endl << Color::Default;
            sf64<D8> aa = A[idx](i);
            sf64<D8> bb = B[idx](i);
            lout << "\n" << i << "\n"
              << "p" << rt.mPartyIdx << ": " << "a   " <<
                prettyShare(rt.mPartyIdx, aa.mShare) << " ~ " << std::endl
              << "p" << rt.mPartyIdx << ": " << "b   " <<
                prettyShare(rt.mPartyIdx, bb.mShare) << " ~ " << std::endl
              << "act " << c(i) << " " << c(i).mValue << std::endl
              << "exp " << cc(i) << " " << cc(i).mValue << std::endl
              << "diff\n" << diff << " " << diff.mValue << std::endl;
          }
        }

        if (failed) {
          lout << "a " << a << std::endl;
          lout << "b " << b << std::endl;
          lout << "c " << c << std::endl;
          lout << "C " << cc << std::endl;
        }
      }
    }
  };

  auto t0 = std::thread(routine, 0);
  auto t1 = std::thread(routine, 1);
  auto t2 = std::thread(routine, 2);

  try {
    t0.join();
  } catch (...) {
    failed = true;
  }

  try {
    t1.join();
  } catch (...) {
    failed = true;
  }

  try {
    t2.join();
  } catch (...) {
    failed = true;
  }

  if (failed)
    throw std::runtime_error(LOCATION);
  EXPECT_EQ(failed, false);
}

TEST(EvaluatorTest, Sh3_Evaluator_intmul_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  int trials = 10;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };

  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  bool failed = false;
  auto t0 = std::thread([&]() {
    auto& enc = encs[0];
    auto& eval = evals[0];
    auto comm = comms[0];
    Sh3Runtime rt;
    rt.init(0, comm);

    for (u64 i = 0; i < trials; ++i) {
      i64 a, b, c, cc;
      si64 aa, bb;
      a = (i + 1) * trials;
      b = (i + 2) * trials;
      Sh3Task task = rt.noDependencies();

      auto i0 = enc.localInt(task, a, aa);
      auto i1 = enc.localInt(task, b, bb);
      si64 result;
      si64 &rr = result;
      clock_t start = clock();
      task = eval.asyncMul(i0 && i1,  aa, bb, rr);
      enc.reveal(task, rr, cc).get();

      clock_t end = clock();
      if (_DEBUG == 1)
        std::cout << "intmul speed test:" << double(end-start)/CLOCKS_PER_SEC
          << std::endl;

      c = a * b;

      if (c != cc) {
        failed = true;
        std::cout << c << std::endl;
        std::cout << cc << std::endl;
      }
    }
  });

  auto rr = [&](int i) {
    auto& enc = encs[i];
    auto& eval = evals[i];
    auto comm = comms[i];
    Sh3Runtime rt;
    rt.init(i, comm);

    for (u64 i = 0; i < trials; ++i) {
      si64 aa, bb, result;
      si64 & rr = result;
      auto i0 = enc.remoteInt(rt, aa);
      auto i1 = enc.remoteInt(rt, bb);  //  compile error ?
      auto m = eval.asyncMul(i0 && i1, aa, bb, rr);
      auto r = enc.reveal(m, 0, rr);

      r.get();
    }
  };

  auto t1 = std::thread(rr, 1);
  auto t2 = std::thread(rr, 2);

  t0.join();
  t1.join();
  t2.join();

  if (failed)
    throw std::runtime_error(LOCATION);
}

TEST(EvaluatorTest, Sh3_Evaluator_mul_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  int trials = 10;
  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl02, chl01),
    std::make_shared<CommPkg>(chl10, chl12),
    std::make_shared<CommPkg>(chl21, chl20)
  };
  
  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];

  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  bool failed = false;
  auto t0 = std::thread([&]() {
    auto& enc = encs[0];
    auto& eval = evals[0];
    auto comm = comms[0];
    Sh3Runtime rt;
    rt.init(0, comm);
    PRNG prng(ZeroBlock);

    for (u64 i = 0; i < trials; ++i) {
      i64Matrix a(trials, trials), b(trials, trials), c(trials, trials),
        cc(trials, trials);
      rand(a, prng);
      rand(b, prng);
      si64Matrix A(trials, trials), B(trials, trials), C(trials, trials);

      auto i0 = enc.localIntMatrix(rt, a, A);
      auto i1 = enc.localIntMatrix(rt, b, B);
      clock_t start = clock();
      auto m = eval.asyncMul(i0 && i1, A, B, C);
      enc.reveal(m, C, cc).get();

      clock_t end = clock();
      if (_DEBUG == 1)
        std::cout << "mul speed test:" << double(end-start)/CLOCKS_PER_SEC
          << std::endl;
      c = a * b;

      if (c != cc) {
        failed = true;
        std::cout << c << std::endl;
        std::cout << cc << std::endl;
      }
    }
  });

  auto rr = [&](int i) {
    auto& enc = encs[i];
    auto& eval = evals[i];
    auto comm = comms[i];
    Sh3Runtime rt;
    rt.init(i, comm);

    for (u64 i = 0; i < trials; ++i) {
      si64Matrix A(trials, trials), B(trials, trials), C(trials, trials);
      auto i0 = enc.remoteIntMatrix(rt, A);
      auto i1 = enc.remoteIntMatrix(rt, B);
      clock_t start = clock();
      auto m = eval.asyncMul(i0 && i1, A, B, C);
      clock_t end = clock();
      auto r = enc.reveal(m, 0, C);

      r.get();
    }
  };

  auto t1 = std::thread(rr, 1);
  auto t2 = std::thread(rr, 2);

  t0.join();
  t1.join();
  t2.join();

  if (failed)
    throw std::runtime_error(LOCATION);
}

TEST(EvaluatorTest, Sh3_f64_basics_test) {
  f64<D8> v0(0.0), v1(0.0);
  auto trials = 100;
  PRNG prng(ZeroBlock);

  for (int i = 0; i < trials; ++i) {
    auto vv0 = prng.get<i16>() / double(1 << 7);
    auto vv1 = prng.get<i16>() / double(1 << 7);

    v0 = vv0;
    v1 = vv1;

    if (v0 + v1 != f64<D8>(vv0 + vv1))
      throw RTE_LOC;

    if (v0 - v1 != f64<D8>(vv0 - vv1))
      throw RTE_LOC;

    if (v0 + v1 != f64<D8>(vv0 + vv1))
      throw RTE_LOC;

    if (v0 * v1 != f64<D8>(vv0 * vv1)) {
      std::cout << vv0 << " * " << vv1 << " = " << vv0 * vv1 << std::endl;
      std::cout << v0 << " * " << v1 << " = " << v0 * v1 << std::endl;

      throw RTE_LOC;
    }
  }
}


void createSharing(PRNG& prng, i64Matrix& value, si64Matrix& s0,
                   si64Matrix& s1, si64Matrix& s2) {
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

void createSharing(PRNG& prng, i64Matrix& value, sbMatrix& s0,
                   sbMatrix& s1, sbMatrix& s2) {
  s0.mShares[0].resize(value.rows(), value.cols());
  s1.mShares[0].resize(value.rows(), value.cols());
  s2.mShares[0].resize(value.rows(), value.cols());

  for (auto i = 0ull; i < s0.mShares[0].size(); ++i)
    s0.mShares[0](i) = value(i);

    {
      prng.get(s1.mShares[0].data(), s1.mShares[0].size());
      prng.get(s2.mShares[0].data(), s2.mShares[0].size());

      for (u64 i = 0; i < s0.mShares[0].size(); ++i)
        s0.mShares[0](i) ^= s1.mShares[0](i) ^ s2.mShares[0](i);
    }

    s0.mShares[1] = s2.mShares[0];
    s1.mShares[1] = s0.mShares[0];
    s2.mShares[1] = s1.mShares[0];
}

TEST(EvaluatorTest, sh3_asyncArithBinMul_test) {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  PRNG prng(ZeroBlock);
  u64 size = 100;
  u64 trials = 10;
  u64 dec = 16;

  vector<std::shared_ptr<CommPkg>> comms = {
    std::make_shared<CommPkg>(chl01, chl10),
    std::make_shared<CommPkg>(chl02, chl20),
    std::make_shared<CommPkg>(chl12, chl21)
  };
  
  Sh3Runtime p0, p1, p2;

  p0.init(0, comms[0]);
  p1.init(1, comms[1]);
  p2.init(2, comms[2]);

  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];
  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));
  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  eMatrix<i64> a(size, 1), b(size, 1), c(size, 1), cc(size, 1);

  si64Matrix
    a0(size, 1),
    a1(size, 1),
    a2(size, 1),
    c0(size, 1),
    c1(size, 1),
    c2(size, 1);

  sbMatrix
    b0(size, 1),
    b1(size, 1),
    b2(size, 1);

  for (u64 t = 0; t < trials; ++t) {
    for (u64 i = 0; i < size; ++i)
      b(i) = prng.get<bool>();

    prng.get(a.data(), a.size());

    for (u64 i = 0; i < size; ++i) {
      a(i) = a(i) >> 32;
      c(i) = a(i) * b(i);
    }

    createSharing(prng, b, b0, b1, b2);
    createSharing(prng, a, a0, a1, a2);

    for (u64 i = 0; i < size; ++i) {
      b0.mShares[0](i) &= 1;
      b0.mShares[1](i) &= 1;
      b1.mShares[0](i) &= 1;
      b1.mShares[1](i) &= 1;
      b2.mShares[0](i) &= 1;
      b2.mShares[1](i) &= 1;
    }

    auto as0 = evals[0].asyncMul(p0, a0, b0, c0);
    auto as1 = evals[1].asyncMul(p1, a1, b1, c1);
    auto as2 = evals[2].asyncMul(p2, a2, b2, c2);

    p0.runNext();
    p1.runNext();
    p2.runNext();

    as0.get();
    as1.get();
    as2.get();

    if (c0.mShares[0] != c1.mShares[1])
      throw std::runtime_error(LOCATION);
    if (c1.mShares[0] != c2.mShares[1]) {
      throw std::runtime_error(LOCATION);
    }
    if (c2.mShares[0] != c0.mShares[1])
      throw std::runtime_error(LOCATION);


    for (u64 i = 0; i < size; ++i) {
      auto ci0 = c0.mShares[0](i);
      auto ci1 = c1.mShares[0](i);
      auto ci2 = c2.mShares[0](i);
      auto di0 = c0.mShares[1](i);
      auto di1 = c1.mShares[1](i);
      auto di2 = c2.mShares[1](i);
      cc(i) = c0.mShares[0](i) + c0.mShares[1](i) + c1.mShares[0](i);
      auto ai = a(i);
      auto bi = b(i);
      auto cci = cc(i);
      auto ci = c(i);
      if (cc(i) != c(i)) {
        throw std::runtime_error(LOCATION);
      }
    }
  }
}

TEST(EvaluatorTest, sh3_asyncPubArithBinMul_test) {
  IOService ios;

  auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server,
    "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client,
    "12").addChannel();

  PRNG prng(ZeroBlock);
  u64 size = 100;

  vector<std::shared_ptr<CommPkg>>comms = {
    std::make_shared<CommPkg>(chl01, chl10),
    std::make_shared<CommPkg>(chl02, chl20),
    std::make_shared<CommPkg>(chl12, chl21)
  };
  Sh3Runtime p0, p1, p2;

  p0.init(0,  comms[0]);
  p1.init(1,  comms[1]);
  p2.init(2,  comms[2]);
  Sh3Encryptor encs[3];
  Sh3Evaluator evals[3];
  encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
  encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
  encs[2].init(2, toBlock(0, 2), toBlock(0, 0));
  evals[0].init(0, toBlock(1, 0), toBlock(1, 1));
  evals[1].init(1, toBlock(1, 1), toBlock(1, 2));
  evals[2].init(2, toBlock(1, 2), toBlock(1, 0));

  eMatrix<i64> b(size, 1), c(size, 1), cc(size, 1);

  sbMatrix
    b0(size, 1),
    b1(size, 1),
    b2(size, 1);
  si64Matrix
    c0(size, 1),
    c1(size, 1),
    c2(size, 1);

  i64 a = prng.get<i64>();

  for (u64 i = 0; i < size; ++i)
    b(i) = prng.get<bool>();

  for (u64 i = 0; i < size; ++i) {
    c(i) = a * b(i);
  }

  createSharing(prng, b, b0, b1, b2);

  for (u64 i = 0; i < size; ++i) {
    b0.mShares[0](i) &= 1;
    b0.mShares[1](i) &= 1;
    b1.mShares[0](i) &= 1;
    b1.mShares[1](i) &= 1;
    b2.mShares[0](i) &= 1;
    b2.mShares[1](i) &= 1;
  }

  auto as0 = evals[0].asyncMul(p0, a, b0, c0);
  auto as1 = evals[1].asyncMul(p1, a, b1, c1);
  auto as2 = evals[2].asyncMul(p2, a, b2, c2);

  p0.runNext();
  p1.runNext();
  p2.runNext();

  as0.get();
  as1.get();
  as2.get();

  for (u64 i = 0; i < size; ++i) {
    if (c0.mShares[0] != c1.mShares[1])
      throw std::runtime_error(LOCATION);
    if (c1.mShares[0] != c2.mShares[1])
      throw std::runtime_error(LOCATION);
    if (c2.mShares[0] != c0.mShares[1])
      throw std::runtime_error(LOCATION);
    auto ci0 = c0.mShares[0](i);
    auto ci1 = c1.mShares[0](i);
    auto ci2 = c2.mShares[0](i);
    auto di0 = c0.mShares[1](i);
    auto di1 = c1.mShares[1](i);
    auto di2 = c2.mShares[1](i);

    cc(i) = c0.mShares[0](i) + c0.mShares[1](i) + c1.mShares[0](i);
    auto bi = b(i);
    auto cci = cc(i);
    auto ci = c(i);
    if (cc(i) != c(i)) {
      throw std::runtime_error(LOCATION);
    }
  }
}

}  // namespace primihub
