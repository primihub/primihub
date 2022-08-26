
#ifndef SRC_primihub_PROTOCOL_ABY3_ENCRYPTOR_H_
#define SRC_primihub_PROTOCOL_ABY3_ENCRYPTOR_H_

#include <utility>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/common/type/matrix_view.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/protocol/aby3/transpose.h"
#include "src/primihub/util/network/socket/commpkg.h"

#include "src/primihub/util/log.h"

namespace primihub {
class Sh3Encryptor {
public:
  void init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize = 256) {
    mShareGen.init(prevSeed, nextSeed, buffSize);
    mPartyIdx = partyIdx;
  }

  void init(u64 partyIdx, CommPkg &comm, block seed, u64 buffSize = 256) {
    mShareGen.init(comm, seed, buffSize);
    mPartyIdx = partyIdx;
  }

  si64 localInt(CommPkg &comm, i64 val);
  si64 remoteInt(CommPkg &comm);

  Sh3Task localInt(Sh3Task dep, i64 val, si64 &dest);
  Sh3Task remoteInt(Sh3Task dep, si64 &dest);

  template <Decimal D>
  Sh3Task localFixed(Sh3Task dep, f64<D> val, sf64<D> &dest);

  template <Decimal D> Sh3Task remoteFixed(Sh3Task dep, sf64<D> &dest);

  sb64 localBinary(CommPkg &comm, i64 val);
  sb64 remoteBinary(CommPkg &comm);

  Sh3Task localBinary(Sh3Task dep, i64 val, sb64 &dest);
  Sh3Task remoteBinary(Sh3Task dep, sb64 &dest);

  // generates a integer sharing of the matrix m and places the result in dest
  void localIntMatrix(CommPkg &comm, const i64Matrix &m, si64Matrix &dest);
  Sh3Task localIntMatrix(Sh3Task dep, const i64Matrix &m, si64Matrix &dest);

  // generates a integer sharing of the matrix input by the remote party and
  // places the result in dest
  void remoteIntMatrix(CommPkg &comm, si64Matrix &dest);
  Sh3Task remoteIntMatrix(Sh3Task dep, si64Matrix &dest);

  template <Decimal D>
  Sh3Task localFixedMatrix(Sh3Task dep, const f64Matrix<D> &m,
                           sf64Matrix<D> &dest);

  template <Decimal D>
  Sh3Task remoteFixedMatrix(Sh3Task dep, sf64Matrix<D> &dest);

  // generates a binary sharing of the matrix m and places the result in dest
  void localBinMatrix(CommPkg &comm, const i64Matrix &m, sbMatrix &dest);
  Sh3Task localBinMatrix(Sh3Task dep, const i64Matrix &m, sbMatrix &dest);

  // generates a binary sharing of the matrix ibput by the remote party and
  // places the result in dest
  void remoteBinMatrix(CommPkg &comm, sbMatrix &dest);
  Sh3Task remoteBinMatrix(Sh3Task dep, sbMatrix &dest);

  // generates a sPackedBin from the given matrix.
  void localPackedBinary(CommPkg &comm, const i64Matrix &m, sPackedBin &dest);
  Sh3Task localPackedBinary(Sh3Task dep, const i64Matrix &m, sPackedBin &dest);

  Sh3Task localPackedBinary(Sh3Task dep, MatrixView<u8> m, sPackedBin &dest,
                            bool transpose);

  // generates a sPackedBin from the given matrix.
  void remotePackedBinary(CommPkg &comm, sPackedBin &dest);
  Sh3Task remotePackedBinary(Sh3Task dep, sPackedBin &dest);

  i64 reveal(CommPkg &comm, const si64 &x);
  i64 revealAll(CommPkg &comm, const si64 &x);
  void reveal(CommPkg &comm, u64 partyIdx, const si64 &x);

  Sh3Task reveal(Sh3Task dep, const si64 &x, i64 &dest);
  Sh3Task revealAll(Sh3Task dep, const si64 &x, i64 &dest);
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const si64 &x);

  template <Decimal D>
  Sh3Task reveal(Sh3Task dep, const sf64<D> &x, f64<D> &dest);
  template <Decimal D>
  Sh3Task revealAll(Sh3Task dep, const sf64<D> &x, f64<D> &dest);
  template <Decimal D>
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const sf64<D> &x);

  template <Decimal D>
  Sh3Task reveal(Sh3Task dep, const sf64Matrix<D> &x, f64Matrix<D> &dest);
  template <Decimal D>
  Sh3Task revealAll(Sh3Task dep, const sf64Matrix<D> &x, f64Matrix<D> &dest);
  template <Decimal D>
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const sf64Matrix<D> &x);

  i64 reveal(CommPkg &comm, const sb64 &x);
  i64 revealAll(CommPkg &comm, const sb64 &x);
  void reveal(CommPkg &comm, u64 partyIdx, const sb64 &x);

  void reveal(CommPkg &comm, const si64Matrix &x, i64Matrix &dest);
  void revealAll(CommPkg &comm, const si64Matrix &x, i64Matrix &dest);
  void reveal(CommPkg &comm, u64 partyIdx, const si64Matrix &x);

  void reveal(CommPkg &comm, const sbMatrix &x, i64Matrix &dest);
  void revealAll(CommPkg &comm, const sbMatrix &x, i64Matrix &dest);
  void reveal(CommPkg &comm, u64 partyIdx, const sbMatrix &x);

  void reveal(CommPkg &comm, const sPackedBin &x, i64Matrix &dest);
  void revealAll(CommPkg &comm, const sPackedBin &x, i64Matrix &dest);
  void reveal(CommPkg &comm, u64 partyIdx, const sPackedBin &x);

  Sh3Task reveal(Sh3Task dep, const sb64 &x, i64 &dest);
  Sh3Task revealAll(Sh3Task dep, const sb64 &x, i64 &dest);
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const sb64 &x);

  Sh3Task reveal(Sh3Task dep, const si64Matrix &x, i64Matrix &dest);
  Sh3Task revealAll(Sh3Task dep, const si64Matrix &x, i64Matrix &dest);
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const si64Matrix &x);

  Sh3Task reveal(Sh3Task dep, const sbMatrix &x, i64Matrix &dest);
  Sh3Task revealAll(Sh3Task dep, const sbMatrix &x, i64Matrix &dest);
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const sbMatrix &x);

  Sh3Task reveal(Sh3Task dep, const sPackedBin &x, i64Matrix &dest);
  Sh3Task revealAll(Sh3Task dep, const sPackedBin &x, i64Matrix &dest);
  Sh3Task reveal(Sh3Task dep, u64 partyIdx, const sPackedBin &x);

  Sh3Task reveal(Sh3Task dep, const sPackedBin &x, PackedBin &dest);
  Sh3Task revealAll(Sh3Task dep, const sPackedBin &x, PackedBin &dest);

  void rand(si64Matrix &dest);
  void rand(sbMatrix &dest);
  void rand(sPackedBin &dest);

  u64 mPartyIdx = -1;
  Sh3ShareGen mShareGen;

  void complateSharing(CommPkg &comm, span<i64> send, span<i64> recv);
};

template <Decimal D>
Sh3Task Sh3Encryptor::localFixed(Sh3Task dep, f64<D> val, sf64<D> &dest) {
  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return localInt(dep, val.mValue, dest.mShare);
}

template <Decimal D>
Sh3Task Sh3Encryptor::remoteFixed(Sh3Task dep, sf64<D> &dest) {
  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return remoteInt(dep, dest.mShare);
}

template <Decimal D>
Sh3Task Sh3Encryptor::localFixedMatrix(Sh3Task dep, const f64Matrix<D> &m,
                                       sf64Matrix<D> &dest) {
  static_assert(sizeof(f64<D>) == sizeof(i64), "assumpition for this cast.");
  auto &mCast = (const i64Matrix &)m;

  static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix),
                "assumpition for this cast.");
  auto &destCast = (si64Matrix &)dest;

  return localIntMatrix(dep, mCast, destCast);
}

template <Decimal D>
Sh3Task Sh3Encryptor::remoteFixedMatrix(Sh3Task dep, sf64Matrix<D> &dest) {
  static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix),
                "assumpition for this cast.");
  auto &destCast = (si64Matrix &)dest;

  return remoteIntMatrix(dep, destCast);
}

template <Decimal D>
Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sf64<D> &x, f64<D> &dest) {
  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return reveal(dep, x.mShare, dest.mValue);
}

template <Decimal D>
Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sf64<D> &x, f64<D> &dest) {
  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return revealAll(dep, x.mShare, dest.mValue);
}

template <Decimal D>
Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const sf64<D> &x) {
  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return reveal(dep, partyIdx, x.mShare);
}

template <Decimal D>
Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sf64Matrix<D> &x,
                             f64Matrix<D> &dest) {
  static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix),
                "assumpition for this cast.");
  auto &xCast = (si64Matrix &)x;

  static_assert(sizeof(f64<D>) == sizeof(i64), "assumpition for this cast.");
  auto &destCast = (i64Matrix &)dest;

  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return reveal(dep, xCast, destCast);
}

template <Decimal D>
Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sf64Matrix<D> &x,
                                f64Matrix<D> &dest) {
  static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix),
                "assumpition for this cast.");
  auto &xCast = (si64Matrix &)x;

  static_assert(sizeof(f64<D>) == sizeof(i64), "assumpition for this cast.");
  auto &destCast = (i64Matrix &)dest;

  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return revealAll(dep, xCast, destCast);
}

template <Decimal D>
Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx,
                             const sf64Matrix<D> &x) {
  static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix),
                "assumpition for this cast.");
  auto &xCast = (si64Matrix &)x;

  // since under the hood we represent a fixed point val as an int,
  // just call that function.
  return reveal(dep, partyIdx, xCast);
}

} // namespace primihub

#endif // SRC_primihub_PROTOCOL_ABY3_ENCRYPTOR_H_
