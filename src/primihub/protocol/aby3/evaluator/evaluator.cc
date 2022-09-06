
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"

namespace primihub {

void Sh3Evaluator::init(u64 partyIdx, block prevSeed, block nextSeed,
                        u64 buffSize) {
  mShareGen.init(prevSeed, nextSeed, buffSize);
  mPartyIdx = partyIdx;
  mOtPrev.setSeed(mShareGen.mNextCommon.get<block>());
  mOtNext.setSeed(mShareGen.mPrevCommon.get<block>());
}

void Sh3Evaluator::init(u64 partyIdx, CommPkg &comm, block seed, u64 buffSize) {
  mShareGen.init(comm, seed, buffSize);
  mPartyIdx = partyIdx;
  mOtPrev.setSeed(mShareGen.mNextCommon.get<block>());
  mOtNext.setSeed(mShareGen.mPrevCommon.get<block>());
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64 &A, const si64 &B,
                               si64 &C) {
  return dependency
      .then([&](CommPkgBase *comm, Sh3Task self) {
        C[0] = A[0] * B[0] + A[0] * B[1] + A[1] * B[0] + mShareGen.getShare();

        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        comm_cast.mNext().asyncSendCopy(C[0]);
        auto fu = comm_cast.mPrev().asyncRecv(C[1]).share();

        self.then([fu = std::move(fu)](CommPkgBase *comm, Sh3Task &self) {
          fu.get();
        });
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncConstMul_test(Sh3Task dependency, const i64 &A,
                                         si64Matrix B, si64Matrix &C) {
  return dependency
      .then([&](CommPkgBase *comm, Sh3Task self) {
        C.mShares[0] = A * B.mShares[0];
        C.mShares[1] = A * B.mShares[1];
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncConstMul_test(Sh3Task dependency, const i64 &A,
                                         const si64 &B, si64 &C) {
  return dependency
      .then([&](CommPkgBase *comm, Sh3Task self) {
        C.mData[0] = A * B.mData[0];
        C.mData[1] = A * B.mData[1];
      })
      .getClosure();
}

//  const matrix mul
void Sh3Evaluator::asyncConstMul(const i64 &a, const si64Matrix &b,
                                 si64Matrix &c) {
  c.mShares[0] = a * b.mShares[0];
  c.mShares[1] = a * b.mShares[1];
}

//  const mul
void Sh3Evaluator::asyncConstMul(const i64 &a, const si64 &b, si64 &c) {
  c[0] = a * b[0];
  c[1] = a * b[1];
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64Matrix &A,
                               const si64Matrix &B, si64Matrix &C) {
  return dependency
      .then([&](CommPkgBase *comm, Sh3Task self) {
        C.mShares[0] = A.mShares[0] * B.mShares[0] +
                       A.mShares[0] * B.mShares[1] +
                       A.mShares[1] * B.mShares[0];

        for (u64 i = 0; i < C.size(); ++i) {
          C.mShares[0](i) += mShareGen.getShare();
        }

        C.mShares[1].resizeLike(C.mShares[0]);
        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        comm_cast.mNext().asyncSendCopy(C.mShares[0].data(),
                                        C.mShares[0].size());
        auto fu = comm_cast.mPrev()
                      .asyncRecv(C.mShares[1].data(), C.mShares[1].size())
                      .share();

        self.then([fu = std::move(fu)](CommPkgBase *comm, Sh3Task &self) {
          fu.get();
        });
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dep, const si64Matrix &a,
                               const sbMatrix &b, si64Matrix &c) {
  return dep
      .then([&](CommPkgBase *comm, Sh3Task self) {
        switch (mPartyIdx) {
        case 0: {
          std::vector<std::array<i64, 2>> s0(a.size());
          BitVector c1(a.size());
          for (u64 i = 0; i < s0.size(); ++i) {
            auto bb = b.mShares[0](i) ^ b.mShares[1](i);

            auto zeroShare = mShareGen.getShare();

            s0[i][bb] = zeroShare;
            s0[i][bb ^ 1] = a.mShares[1](i) + zeroShare;

            c1[i] = static_cast<u8>(b.mShares[1](i));
          }
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtNext.send(comm_cast.mNext(), s0);
          mOtPrev.send(comm_cast.mPrev(), s0);

          mOtPrev.help(comm_cast.mPrev(), c1);
          auto fu1 = comm_cast.mPrev()
                         .asyncRecv(c.mShares[0].data(), c.size())
                         .share();
          i64 *dd = c.mShares[1].data();
          auto fu2 = SharedOT::asyncRecv(comm_cast.mNext(), comm_cast.mPrev(),
                                         std::move(c1), {dd, i64(c.size())})
                         .share();

          self.then([fu1 = std::move(fu1), fu2 = std::move(fu2)](
                        CommPkgBase *comm, Sh3Task self) mutable {
            fu1.get();
            fu2.get();
          });
          break;
        }
        case 1: {
          std::vector<std::array<i64, 2>> s1(a.size());
          BitVector c0(a.size());
          for (u64 i = 0; i < s1.size(); ++i) {
            auto bb = b.mShares[0](i) ^ b.mShares[1](i);
            auto ss = mShareGen.getShare();

            s1[i][bb] = ss;
            s1[i][bb ^ 1] = (a.mShares[0](i) + a.mShares[1](i)) + ss;

            c0[i] = static_cast<u8>(b.mShares[0](i));
          }
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtNext.help(comm_cast.mNext(), c0);
          mOtNext.send(comm_cast.mNext(), s1);
          mOtPrev.send(comm_cast.mPrev(), s1);

          i64 *dd = c.mShares[0].data();
          auto fu1 = SharedOT::asyncRecv(comm_cast.mPrev(), comm_cast.mNext(),
                                         std::move(c0), {dd, i64(c.size())})
                         .share();

          // share 1:
          auto fu2 = comm_cast.mNext()
                         .asyncRecv(c.mShares[1].data(), c.size())
                         .share();

          self.then(
              [fu1 = std::move(fu1), fu2 = std::move(fu2), &c,
               _2 = std::move(c0)](CommPkgBase *comm, Sh3Task self) mutable {
                fu1.get();
                fu2.get();
              });

          break;
        }
        case 2: {
          BitVector c0(a.size()), c1(a.size());
          std::vector<i64> s0(a.size()), s1(a.size());
          for (u64 i = 0; i < a.size(); ++i) {
            c0[i] = static_cast<u8>(b.mShares[1](i));
            c1[i] = static_cast<u8>(b.mShares[0](i));

            s0[i] = s1[i] = mShareGen.getShare();
          }
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtPrev.help(comm_cast.mPrev(), c0);
          comm_cast.mNext().asyncSend(std::move(s0));

          mOtNext.help(comm_cast.mNext(), c1);
          comm_cast.mPrev().asyncSend(std::move(s1));

          // share 0: from p0 to p1,p2
          i64 *dd0 = c.mShares[1].data();
          auto fu1 = SharedOT::asyncRecv(comm_cast.mNext(), comm_cast.mPrev(),
                                         std::move(c0), {dd0, i64(c.size())})
                         .share();

          // share 1: from p1 to p0,p2
          i64 *dd1 = c.mShares[0].data();
          auto fu2 = SharedOT::asyncRecv(comm_cast.mPrev(), comm_cast.mNext(),
                                         std::move(c1), {dd1, i64(c.size())})
                         .share();

          self.then([fu1 = std::move(fu1), fu2 = std::move(fu2),
                     &c](CommPkgBase *comm, Sh3Task self) mutable {
            fu1.get();
            fu2.get();
          });
          break;
        }
        default:
          throw std::runtime_error(LOCATION);
        }
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dep, const i64 &a, const sbMatrix &b,
                               si64Matrix &c) {
  return dep
      .then([&](CommPkgBase *comm, Sh3Task self) {
        if (b.bitCount() != 1)
          throw RTE_LOC;

        switch (mPartyIdx) {
        case 0: {
          std::vector<std::array<i64, 2>> s0(b.rows());
          for (u64 i = 0; i < s0.size(); ++i) {
            auto bb = b.mShares[0](i) ^ b.mShares[1](i);
            auto zeroShare = mShareGen.getShare();

            s0[i][bb] = zeroShare;
            s0[i][bb ^ 1] = a + zeroShare;
          }

          // share 0: from p0 to p1,p2
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtNext.send(comm_cast.mNext(), s0);
          mOtPrev.send(comm_cast.mPrev(), s0);

          auto fu1 = comm_cast.mNext()
                         .asyncRecv(c.mShares[0].data(), c.size())
                         .share();
          auto fu2 = comm_cast.mPrev()
                         .asyncRecv(c.mShares[1].data(), c.size())
                         .share();
          self.then([fu1 = std::move(fu1),
                     fu2 = std::move(fu2)](CommPkgBase *_, Sh3Task __) mutable {
            fu1.get();
            fu2.get();
          });
          break;
        }
        case 1: {
          BitVector c0(b.rows());
          for (u64 i = 0; i < b.rows(); ++i) {
            c.mShares[1](i) = mShareGen.getShare();
            c0[i] = static_cast<u8>(b.mShares[0](i));
          }

          // share 0: from p0 to p1,p2
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtNext.help(comm_cast.mNext(), c0);
          comm_cast.mPrev().asyncSendCopy(c.mShares[1].data(), c.size());

          i64 *dd = c.mShares[0].data();
          auto fu1 = SharedOT::asyncRecv(comm_cast.mPrev(), comm_cast.mNext(),
                                         std::move(c0), {dd, i64(c.size())})
                         .share();
          self.then([fu1 = std::move(fu1)](CommPkgBase *_, Sh3Task __) mutable {
            fu1.get();
          });

          break;
        }
        case 2: {
          BitVector c0(b.rows());
          for (u64 i = 0; i < b.rows(); ++i) {
            c.mShares[0](i) = mShareGen.getShare();
            c0[i] = static_cast<u8>(b.mShares[1](i));
          }

          // share 0: from p0 to p1,p2
          auto comm_cast = dynamic_cast<CommPkg &>(*comm);
          mOtPrev.help(comm_cast.mPrev(), c0);
          comm_cast.mNext().asyncSendCopy(c.mShares[0].data(), c.size());

          i64 *dd0 = c.mShares[1].data();
          auto fu1 = SharedOT::asyncRecv(comm_cast.mNext(), comm_cast.mPrev(),
                                         std::move(c0), {dd0, i64(c.size())})
                         .share();

          self.then([fu1 = std::move(fu1)](CommPkgBase *_, Sh3Task __) mutable {
            fu1.get();
          });
          break;
        }
        default:
          throw std::runtime_error(LOCATION);
        }
      })
      .getClosure();
}

// si64Matrix Sh3Evaluator::asyncDotMul(Sh3Task dependency, const si64Matrix &A,
//                                      const si64Matrix &B) {
//   if (A.cols() != B.cols() || A.rows() != B.rows())
//     throw std::runtime_error(LOCATION);

//   si64Matrix ret(A.rows(), A.cols());
//   si64 a, b, c;
//   for (int i = 0; i < A.rows(); i++)
//     for (int j = 0; j < A.cols(); j++) {
//       a[0] = A[0](i, j);
//       a[1] = A[1](i, j);
//       b[0] = B[0](i, j);
//       b[1] = B[1](i, j);
//       asyncMul(dependency, a, b, c).get();
//       ret[0](i, j) = c[0];
//       ret[1](i, j) = c[1];
//     }
//   return ret;
// }

Sh3Task Sh3Evaluator::asyncDotMul(Sh3Task dependency, const si64Matrix &A,
                                  const si64Matrix &B, si64Matrix &C) {
  return dependency
      .then([&](CommPkgBase *comm, Sh3Task self) {
        C.mShares[0] = A.mShares[0].array() * B.mShares[0].array() +
                       A.mShares[0].array() * B.mShares[1].array() +
                       A.mShares[1].array() * B.mShares[0].array();

        for (u64 i = 0; i < C.size(); ++i) {
          C.mShares[0](i) += mShareGen.getShare();
        }

        C.mShares[1].resizeLike(C.mShares[0]);
        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        comm_cast.mNext().asyncSendCopy(C.mShares[0].data(),
                                        C.mShares[0].size());
        auto fu = comm_cast.mPrev()
                      .asyncRecv(C.mShares[1].data(), C.mShares[1].size())
                      .share();

        self.then([fu = std::move(fu)](CommPkgBase *comm, Sh3Task &self) {
          fu.get();
        });
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncDotMul(Sh3Task dependency, const si64Matrix &A,
                                  const si64Matrix &B, si64Matrix &C,
                                  u64 shift) {
  return dependency
      .then([&, shift](CommPkgBase *comm, Sh3Task &self) -> void {
        i64Matrix abMinusR = A.mShares[0].array() * B.mShares[0].array() +
                             A.mShares[0].array() * B.mShares[1].array() +
                             A.mShares[1].array() * B.mShares[0].array();

        auto truncationTuple =
            getTruncationTuple(abMinusR.rows(), abMinusR.cols(), shift);

        abMinusR -= truncationTuple.mR;
        C.mShares = std::move(truncationTuple.mRTrunc.mShares);

        auto &rt = self.getRuntime();

        // reveal dependency.getRuntime().the value to party 0, 1
        auto next = (rt.mPartyIdx + 1) % 3;
        auto prev = (rt.mPartyIdx + 2) % 3;
        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        if (next < 2)
          comm_cast.mNext().asyncSendCopy(abMinusR.data(), abMinusR.size());
        if (prev < 2)
          comm_cast.mPrev().asyncSendCopy(abMinusR.data(), abMinusR.size());
        if (rt.mPartyIdx < 2) {
          auto shares = std::make_unique<std::array<i64Matrix, 3>>();

          (*shares)[0].resize(abMinusR.rows(), abMinusR.cols());
          (*shares)[1].resize(abMinusR.rows(), abMinusR.cols());

          // perform the async receives
          auto fu0 = comm_cast.mNext()
                         .asyncRecv((*shares)[0].data(), (*shares)[0].size())
                         .share();
          auto fu1 = comm_cast.mPrev()
                         .asyncRecv((*shares)[1].data(), (*shares)[1].size())
                         .share();
          (*shares)[2] = std::move(abMinusR);

          // set the completion handle complete the computation
          self.then([fu0, fu1, shares = std::move(shares), &C, shift,
                     this](CommPkgBase *comm, Sh3Task self) mutable {
            fu0.get();
            fu1.get();

            // xy-r
            (*shares)[0] += (*shares)[1] + (*shares)[2];

            // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
            auto &v = C.mShares[mPartyIdx];
            auto &s = (*shares)[0];
            for (u64 i = 0; i < v.size(); ++i)
              v(i) += s(i) >> shift;
          });
        }
      })
      .getClosure();
}

TruncationPair Sh3Evaluator::getTruncationTuple(u64 xSize, u64 ySize, u64 d) {
  TruncationPair pair;
  if (DEBUG_disable_randomization) {
    pair.mR.resize(xSize, ySize);
    pair.mR.setZero();

    pair.mRTrunc.mShares[0].resize(xSize, ySize);
    pair.mRTrunc.mShares[1].resize(xSize, ySize);
    pair.mRTrunc.mShares[0].setZero();
    pair.mRTrunc.mShares[1].setZero();
  } else {
    const auto d2 = d + 2;
    pair.mR.resize(xSize, ySize);
    pair.mRTrunc.resize(xSize, ySize);
    const u64 mask = (~0ull) >> 1;
    // if (mPartyIdx == 0)
    {
      // mShareGen.mPrevCommon.get(pair.mR.data(), pair.mR.size());
      mShareGen.mNextCommon.get(pair.mRTrunc[0].data(), pair.mRTrunc[0].size());
      mShareGen.mPrevCommon.get(pair.mRTrunc[1].data(), pair.mRTrunc[1].size());
      for (u64 i = 0; i < pair.mR.size(); ++i) {
        auto &t0 = pair.mRTrunc[0](i);
        auto &t1 = pair.mRTrunc[1](i);
        auto &r0 = pair.mR(i);

        r0 = t0 >> 2;
        t0 >>= d2;
        t1 >>= d2;
      }
    }
  }
  return pair;
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64 &A, const si64 &B,
                               si64 &C, u64 shift) {
  return dependency
      .then([&, shift](CommPkgBase *comm, Sh3Task &self) -> void {
        auto truncationTuple = getTruncationTuple(1, 1, shift);
        auto abMinusR = A.mData[0] * B.mData[0] + A.mData[0] * B.mData[1] +
                        A.mData[1] * B.mData[0];

        abMinusR -= truncationTuple.mR(0);
        C = truncationTuple.mRTrunc(0);

        auto &rt = self.getRuntime();

        // reveal dependency.getRuntime().the value to party 0, 1
        auto next = (rt.mPartyIdx + 1) % 3;
        auto prev = (rt.mPartyIdx + 2) % 3;
        auto &comm_cast = dynamic_cast<CommPkg &>(*comm);
        if (next < 2)
          comm_cast.mNext().asyncSendCopy(abMinusR);
        if (prev < 2)
          comm_cast.mPrev().asyncSendCopy(abMinusR);

        if (rt.mPartyIdx < 2) {
          // these will hold the three shares of r-xy
          std::unique_ptr<std::array<i64, 3>> shares(new std::array<i64, 3>);

          // perform the async receives
          auto fu0 = comm_cast.mNext().asyncRecv((*shares)[0]).share();
          auto fu1 = comm_cast.mPrev().asyncRecv((*shares)[1]).share();
          (*shares)[2] = abMinusR;

          // set the completion handle complete the computation
          self.then([fu0, fu1, shares = std::move(shares), &C, shift,
                     this](CommPkgBase *comm, Sh3Task self) mutable {
            fu0.get();
            fu1.get();

            // xy-r
            (*shares)[0] += (*shares)[1] + (*shares)[2];

            // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
            C.mData[mPartyIdx] += (*shares)[0] >> shift;
          });
        }
      })
      .getClosure();
}

Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64Matrix &A,
                               const si64Matrix &B, si64Matrix &C, u64 shift) {
  return dependency
      .then([&, shift](CommPkgBase *comm, Sh3Task &self) -> void {
        i64Matrix abMinusR = A.mShares[0] * B.mShares[0] +
                             A.mShares[0] * B.mShares[1] +
                             A.mShares[1] * B.mShares[0];

        auto truncationTuple =
            getTruncationTuple(abMinusR.rows(), abMinusR.cols(), shift);

        abMinusR -= truncationTuple.mR;
        C.mShares = std::move(truncationTuple.mRTrunc.mShares);

        auto &rt = self.getRuntime();

        // reveal dependency.getRuntime().the value to party 0, 1
        auto next = (rt.mPartyIdx + 1) % 3;
        auto prev = (rt.mPartyIdx + 2) % 3;
        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        if (next < 2)
          comm_cast.mNext().asyncSendCopy(abMinusR.data(), abMinusR.size());
        if (prev < 2)
          comm_cast.mPrev().asyncSendCopy(abMinusR.data(), abMinusR.size());
        if (rt.mPartyIdx < 2) {
          auto shares = std::make_unique<std::array<i64Matrix, 3>>();

          (*shares)[0].resize(abMinusR.rows(), abMinusR.cols());
          (*shares)[1].resize(abMinusR.rows(), abMinusR.cols());

          // perform the async receives
          auto fu0 = comm_cast.mNext()
                         .asyncRecv((*shares)[0].data(), (*shares)[0].size())
                         .share();
          auto fu1 = comm_cast.mPrev()
                         .asyncRecv((*shares)[1].data(), (*shares)[1].size())
                         .share();
          (*shares)[2] = std::move(abMinusR);

          // set the completion handle complete the computation
          self.then([fu0, fu1, shares = std::move(shares), &C, shift,
                     this](CommPkgBase *comm, Sh3Task self) mutable {
            fu0.get();
            fu1.get();

            // xy-r
            (*shares)[0] += (*shares)[1] + (*shares)[2];

            // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
            auto &v = C.mShares[mPartyIdx];
            auto &s = (*shares)[0];
            for (u64 i = 0; i < v.size(); ++i)
              v(i) += s(i) >> shift;
          });
        }
      })
      .getClosure();
}

//  const fixed number mul
Sh3Task Sh3Evaluator::asyncConstFixedMul(Sh3Task dependency, const i64 &A,
                                         const si64 &B, si64 &C, u64 shift) {
  return dependency
      .then([&, shift](CommPkgBase *comm, Sh3Task &self) -> void {
        auto truncationTuple = getTruncationTuple(1, 1, shift);
        auto abMinusR = A * B.mData[0];
        abMinusR -= truncationTuple.mR(0);
        C = truncationTuple.mRTrunc(0);
        auto &rt = self.getRuntime();

        // reveal dependency.getRuntime().the value to party 0, 1
        auto next = (rt.mPartyIdx + 1) % 3;
        auto prev = (rt.mPartyIdx + 2) % 3;
        auto &comm_cast = dynamic_cast<CommPkg &>(*comm);
        if (next < 2)
          comm_cast.mNext().asyncSendCopy(abMinusR);
        if (prev < 2)
          comm_cast.mPrev().asyncSendCopy(abMinusR);

        if (rt.mPartyIdx < 2) {
          // these will hold the three shares of r-xy
          std::unique_ptr<std::array<i64, 3>> shares(new std::array<i64, 3>);

          // perform the async receives
          auto fu0 = comm_cast.mNext().asyncRecv((*shares)[0]).share();
          auto fu1 = comm_cast.mPrev().asyncRecv((*shares)[1]).share();
          (*shares)[2] = abMinusR;

          // set the completion handle complete the computation
          self.then([fu0, fu1, shares = std::move(shares), &C, shift,
                     this](CommPkgBase *comm, Sh3Task self) mutable {
            fu0.get();
            fu1.get();

            // xy-r
            (*shares)[0] += (*shares)[1] + (*shares)[2];

            // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
            C.mData[mPartyIdx] += (*shares)[0] >> shift;
          });
        }
      })
      .getClosure();
}

//  const fixed matrix mul
Sh3Task Sh3Evaluator::asyncConstFixedMul(Sh3Task dependency, const i64 &A,
                                         const si64Matrix &B, si64Matrix &C,
                                         u64 shift) {
  return dependency
      .then([&, shift](CommPkgBase *comm, Sh3Task &self) -> void {
        i64Matrix abMinusR = A * B.mShares[0];

        auto truncationTuple =
            getTruncationTuple(abMinusR.rows(), abMinusR.cols(), shift);

        abMinusR -= truncationTuple.mR;
        C.mShares = std::move(truncationTuple.mRTrunc.mShares);

        auto &rt = self.getRuntime();

        // reveal dependency.getRuntime().the value to party 0, 1
        auto next = (rt.mPartyIdx + 1) % 3;
        auto prev = (rt.mPartyIdx + 2) % 3;
        auto comm_cast = dynamic_cast<CommPkg &>(*comm);
        if (next < 2)
          comm_cast.mNext().asyncSendCopy(abMinusR.data(), abMinusR.size());
        if (prev < 2)
          comm_cast.mPrev().asyncSendCopy(abMinusR.data(), abMinusR.size());

        if (rt.mPartyIdx < 2) {
          // these will hold the three shares of r-xy
          // std::unique_ptr<std::array<i64, 3>> shares(new std::array<i64, 3>);
          auto shares = std::make_unique<std::array<i64Matrix, 3>>();

          // i64Matrix& rr = (*shares)[0]);

          (*shares)[0].resize(abMinusR.rows(), abMinusR.cols());
          (*shares)[1].resize(abMinusR.rows(), abMinusR.cols());

          // perform the async receives
          auto fu0 = comm_cast.mNext()
                         .asyncRecv((*shares)[0].data(), (*shares)[0].size())
                         .share();
          auto fu1 = comm_cast.mPrev()
                         .asyncRecv((*shares)[1].data(), (*shares)[1].size())
                         .share();
          (*shares)[2] = std::move(abMinusR);

          // set the completion handle complete the computation
          self.then([fu0, fu1, shares = std::move(shares), &C, shift,
                     this](CommPkgBase *comm, Sh3Task self) mutable {
            fu0.get();
            fu1.get();

            // xy-r
            (*shares)[0] += (*shares)[1] + (*shares)[2];

            // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
            auto &v = C.mShares[mPartyIdx];
            auto &s = (*shares)[0];
            for (u64 i = 0; i < v.size(); ++i)
              v(i) += s(i) >> shift;
          });
        }
      })
      .getClosure();
}
} // namespace primihub
