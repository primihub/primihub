
#ifndef SRC_primihub_PROTOCOL_ABY3_EVALUATOR_EVALUATOR_H_
#define SRC_primihub_PROTOCOL_ABY3_EVALUATOR_EVALUATOR_H_

#include <iomanip>
#include <memory>
#include <utility>
#include <vector>

#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/primitive/ot/share_ot.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/log.h"
namespace primihub {

struct TruncationPair {
  // the share that should be added before the value being
  // trucnated is revealed.
  i64Matrix mR;

  // the share that thsould be subtracted after the value has been truncated.
  si64Matrix mRTrunc;
};

class Sh3Evaluator {
public:
  void init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize = 256);
  void init(u64 partyIdx, CommPkg &comm, block seed, u64 buffSize = 256);

  bool DEBUG_disable_randomization = false;

  Sh3Task asyncMul(Sh3Task dependency, const si64 &A, const si64 &B, si64 &C);

  Sh3Task asyncMul(Sh3Task dependency, const si64Matrix &A, const si64Matrix &B,
                   si64Matrix &C);

  Sh3Task asyncMul(Sh3Task dependency, const si64Matrix &A, const si64Matrix &B,
                   si64Matrix &C, u64 shift);

  Sh3Task asyncConstMul_test(Sh3Task dependency, const i64 &A, si64 B,
                             si64Matrix &C);

  Sh3Task asyncMul(Sh3Task dependency, const si64 &A, const si64 &B, si64 &C,
                   u64 shift);

  template <Decimal D>
  Sh3Task asyncMul(Sh3Task dependency, const sf64<D> &A, const sf64<D> &B,
                   sf64<D> &C) {
    return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D);
  }

  template <Decimal D>
  Sh3Task asyncMul(Sh3Task dependency, const sf64Matrix<D> &A,
                   const sf64Matrix<D> &B, sf64Matrix<D> &C, u64 shift) {
    return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(),
                    D + shift);
  }

  template <Decimal D>
  Sh3Task asyncMul(Sh3Task dependency, const sf64Matrix<D> &A,
                   const sf64Matrix<D> &B, sf64Matrix<D> &C) {
    return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D);
  }

  Sh3Task asyncMul(Sh3Task dep, const si64Matrix &A, const sbMatrix &B,
                   si64Matrix &C);

  Sh3Task asyncMul(Sh3Task dep, const i64 &a, const sbMatrix &B, si64Matrix &C);

  Sh3Task asyncDotMul(Sh3Task dependency, const si64Matrix &A,
                      const si64Matrix &B, si64Matrix &C, u64 shift);

  template <Decimal D>
  Sh3Task asyncDotMul(Sh3Task dependency, const sf64Matrix<D> &A,
                      const sf64Matrix<D> &B, sf64Matrix<D> &C) {
    return asyncDotMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D);
  }

  template <Decimal D>
  Sh3Task asyncDotMul(Sh3Task dependency, const sf64Matrix<D> &A,
                      const sf64Matrix<D> &B, sf64Matrix<D> &C, u64 shift) {
    return asyncDotMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(),
                       D + shift);
  }

  Sh3Task asyncDotMul(Sh3Task dependency, const si64Matrix &A,
                      const si64Matrix &B, si64Matrix &C);
  // const matrix mul
  void asyncConstMul(const i64 &a, const si64Matrix &b, si64Matrix &c);

  // const mul
  void asyncConstMul(const i64 &a, const si64 &b, si64 &c);

  // secret share -> fixed const matrix mul
  template <Decimal D>
  void asyncConstMul(const i64 &A, const sf64<D> &B, sf64<D> &C) {
    return asyncConstMul(A, B.i64Cast(), C.i64Cast());
  }

  // secret share -> fixed matrix const mul
  template <Decimal D>
  void asyncConstMul(const i64 &A, const sf64Matrix<D> &B, sf64Matrix<D> &C) {
    return asyncConstMul(A, B.i64Cast(), C.i64Cast());
  }

  // const fixed number mul
  Sh3Task asyncConstFixedMul(Sh3Task dependency, const i64 &A, const si64 &B,
                             si64 &C, u64 shift);

  // const fixed matrix mul
  Sh3Task asyncConstFixedMul(Sh3Task dependency, const i64 &A,
                             const si64Matrix &B, si64Matrix &C, u64 shift);

  // secret share -> fixed matrix mul
  template <Decimal D>
  Sh3Task asyncConstFixedMul(Sh3Task dependency, const f64<D> &A,
                             const sf64<D> &B, sf64<D> &C) {
    return asyncConstFixedMul(dependency, A.mValue, B.i64Cast(), C.i64Cast(),
                              D);
  }

  // secret share -> fixed matrix const mul
  template <Decimal D>
  Sh3Task asyncConstFixedMul(Sh3Task dependency, const f64<D> &A,
                             const sf64Matrix<D> &B, sf64Matrix<D> &C) {
    return asyncConstFixedMul(dependency, A.mValue, B.i64Cast(), C.i64Cast(),
                              D);
  }

  TruncationPair getTruncationTuple(u64 xSize, u64 ySize, u64 d);

  u64 mPartyIdx = -1, mTruncationIdx = 0;
  Sh3ShareGen mShareGen;
  SharedOT mOtPrev, mOtNext;

  Sh3Task asyncConstMul_test(Sh3Task dependency, const i64 &A, const si64 &B,
                             si64 &C);

  Sh3Task asyncConstMul_test(Sh3Task dependency, const i64 &A, si64Matrix B,
                             si64Matrix &C);
};

} // namespace primihub

#endif // SRC_primihub_PROTOCOL_ABY3_EVALUATOR_EVALUATOR_H_
