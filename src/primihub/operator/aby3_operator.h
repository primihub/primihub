
#ifndef SRC_primihub_operator_ABY3_operator_H
#define SRC_primihub_operator_ABY3_operator_H

#include <algorithm>
#include <random>
#include <vector>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/binary_evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/piecewise.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/session.h"

#include "src/primihub/common/type/type.h"

#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {

const Decimal D = D16;

class MPCOperator {
public:
  Channel mNext, mPrev;
  IOService ios;
  Sh3Encryptor enc;
  Sh3BinaryEvaluator binEval;
  Sh3ShareGen gen;
  Sh3Evaluator eval;
  Sh3Runtime runtime;
  u64 partyIdx;
  string next_name;
  string prev_name;

  MPCOperator(u64 partyIdx_, string NextName, string PrevName)
      : partyIdx(partyIdx_), next_name(NextName), prev_name(PrevName) {}

  int setup(std::string ip, u32 next_port, u32 prev_port);
  void fini();
  template <Decimal D>
  void createShares(const eMatrix<double> &vals, sf64Matrix<D> &sharedMatrix) {
    f64Matrix<D> fixedMatrix(vals.rows(), vals.cols());
    for (u64 i = 0; i < vals.size(); ++i)
      fixedMatrix(i) = vals(i);
    enc.localFixedMatrix(runtime, fixedMatrix, sharedMatrix).get();
  }

  template <Decimal D> void createShares(sf64Matrix<D> &sharedMatrix) {
    enc.remoteFixedMatrix(runtime, sharedMatrix).get();
  }

  template <Decimal D>
  sf64Matrix<D> createSharesByShape(const eMatrix<double> &val) {
    f64Matrix<D> v2(val.rows(), val.cols());
    for (u64 i = 0; i < val.size(); ++i)
      v2(i) = val(i);
    return createSharesByShape(v2);
  }

  template <Decimal D>
  sf64Matrix<D> createSharesByShape(const f64Matrix<D> &val) {
    std::array<u64, 2> size{val.rows(), val.cols()};
    mNext.asyncSendCopy(size);
    mPrev.asyncSendCopy(size);
    sf64Matrix<D> dest(size[0], size[1]);
    enc.localFixedMatrix(runtime, val, dest).get();
    return dest;
  }

  template <Decimal D> sf64Matrix<D> createSharesByShape(u64 pIdx) {
    std::array<u64, 2> size;
    if (pIdx == (partyIdx + 1) % 3)
      mNext.recv(size);
    else if (pIdx == (partyIdx + 2) % 3)
      mPrev.recv(size);
    else
      throw RTE_LOC;
    sf64Matrix<D> dest(size[0], size[1]);
    enc.remoteFixedMatrix(runtime, dest).get();
    return dest;
  }

  void createShares(const i64Matrix &vals, si64Matrix &sharedMatrix);

  void createShares(si64Matrix &sharedMatrix);

  si64Matrix createSharesByShape(const i64Matrix &val);

  si64Matrix createSharesByShape(u64 partyIdx);

  template <Decimal D> void createShares(double vals, sf64<D> &sharedFixedInt) {
    enc.localFixed<D>(runtime, vals, sharedFixedInt).get();
  }

  template <Decimal D> void createShares(sf64<D> &sharedFixedInt) {
    enc.remoteFixed<D>(runtime, sharedFixedInt).get();
  }

  sbMatrix createBinSharesByShape(i64Matrix &val, u64 bitCount);

  sbMatrix createBinSharesByShape(u64 partyIdx);

  template <Decimal D> eMatrix<double> revealAll(const sf64Matrix<D> &vals) {
    f64Matrix<D> temp(vals.rows(), vals.cols());
    enc.revealAll(runtime, vals, temp).get();

    eMatrix<double> ret(vals.rows(), vals.cols());
    for (u64 i = 0; i < ret.size(); ++i)
      ret(i) = static_cast<double>(temp(i));
    return ret;
  }

  i64Matrix revealAll(const si64Matrix &vals);

  template <Decimal D> double revealAll(const sf64<D> &vals) {
    f64<D> ret;
    enc.revealAll(runtime, vals, ret).get();
    return static_cast<double>(ret);
  }

  template <Decimal D> eMatrix<double> reveal(const sf64Matrix<D> &vals) {
    f64Matrix<D> temp(vals.rows(), vals.cols());
    enc.reveal(runtime, vals, temp).get();
    eMatrix<double> ret(vals.rows(), vals.cols());
    for (u64 i = 0; i < ret.size(); ++i)
      ret(i) = static_cast<double>(temp(i));
    return ret;
  }

  template <Decimal D> void reveal(const sf64Matrix<D> &vals, u64 Idx) {
    enc.reveal(runtime, Idx, vals).get();
  }

  i64Matrix reveal(const si64Matrix &vals);

  void reveal(const si64Matrix &vals, u64 Idx);

  template <Decimal D> double reveal(const sf64<D> &vals) {
    f64<D> ret;
    enc.reveal(runtime, vals, ret).get();
    return static_cast<double>(ret);
  }

  template <Decimal D> void reveal(const sf64<D> &vals, u64 Idx) {
    enc.reveal(runtime, Idx, vals).get();
  }

  template <Decimal D> sf64<D> MPC_Add(std::vector<sf64<D>> sharedFixedInt) {
    sf64<D> sum;
    sum = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); i++) {
      sum = sum + sharedFixedInt[i];
    }
    return sum;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Add(std::vector<sf64Matrix<D>> sharedFixedInt) {
    sf64Matrix<D> sum;
    sum = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); i++) {
      sum = sum + sharedFixedInt[i];
    }
    return sum;
  }

  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt);

  template <Decimal D>
  sf64<D> MPC_Add_Const(f64<D> constfixed, sf64<D> &sharedFixed) {
    sf64<D> temp = sharedFixed;
    if (partyIdx == 0)
      temp[0] = sharedFixed[0] + constfixed.mValue;
    else if (partyIdx == 1)
      temp[1] = sharedFixed[1] + constfixed.mValue;
    return temp;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Add_Const(f64<D> constfixed, sf64Matrix<D> &sharedFixed) {
    sf64Matrix<D> temp = sharedFixed;
    if (partyIdx == 0)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++) {
          LOG(INFO) << temp[0](i, j);
          temp[0](i, j) = sharedFixed[0](i, j) + constfixed.mValue;
          LOG(INFO) << temp[0](i, j);
        }
    else if (partyIdx == 1)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++) {
          LOG(INFO) << temp[1](i, j);
          temp[1](i, j) = sharedFixed[1](i, j) + constfixed.mValue;
          LOG(INFO) << temp[1](i, j);
        }
    return temp;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Add_Const(f64Matrix<D> constFixedMatrix,
                              sf64Matrix<D> &sharedFixed) {
    auto b0 = sharedFixed[0].cols() != constFixedMatrix.cols();
    auto b1 = sharedFixed[0].rows() != constFixedMatrix.rows();
    if (b0 || b1)
      throw std::runtime_error(LOCATION);
    sf64Matrix<D> temp = sharedFixed;
    if (partyIdx == 0)
      temp[0] = sharedFixed[0] + constFixedMatrix.i64Cast();
    else if (partyIdx == 1)
      temp[1] = sharedFixed[1] + constFixedMatrix.i64Cast();
    return temp;
  }

  si64 MPC_Add_Const(i64 constInt, si64 &sharedInt);

  si64Matrix MPC_Add_Const(i64 constInt, si64Matrix &sharedIntMatrix);

  template <Decimal D>
  sf64<D> MPC_Sub(sf64<D> minuend, std::vector<sf64<D>> subtrahends) {
    sf64<D> difference;
    difference = minuend;
    for (u64 i = 0; i < subtrahends.size(); i++) {
      difference = difference - subtrahends[i];
    }
    return difference;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Sub(sf64Matrix<D> minuend,
                        std::vector<sf64Matrix<D>> subtrahends) {
    sf64Matrix<D> difference;
    difference = minuend;
    for (u64 i = 0; i < subtrahends.size(); i++) {
      difference = difference - subtrahends[i];
    }
    return difference;
  }

  si64Matrix MPC_Sub(si64Matrix minuend, std::vector<si64Matrix> subtrahends);

  template <Decimal D>
  sf64<D> MPC_Sub_Const(f64<D> constfixed, sf64<D> &sharedFixed) {
    sf64<D> temp = sharedFixed;
    if (partyIdx == 0)
      temp[0] = sharedFixed[0] - constfixed.mValue;
    else if (partyIdx == 1)
      temp[1] = sharedFixed[1] - constfixed.mValue;
    return temp;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Sub_Const(f64<D> constfixed, sf64Matrix<D> &sharedFixed) {
    sf64Matrix<D> temp = sharedFixed;
    if (partyIdx == 0)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++)
          temp[0](i, j) = sharedFixed[0](i, j) - constfixed.mValue;
    else if (partyIdx == 1)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++)
          temp[1](i, j) = sharedFixed[1](i, j) - constfixed.mValue;
    return temp;
  }

  si64 MPC_Sub_Const(i64 constInt, si64 &sharedInt);

  si64Matrix MPC_Sub_Const(i64 constInt, si64Matrix &sharedIntMatrix);

  template <Decimal D>
  sf64<D> MPC_Mul(std::vector<sf64<D>> sharedFixedInt, sf64<D> &prod) {
    prod = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); ++i)
      eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
    return prod;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Mul(std::vector<sf64Matrix<D>> sharedFixedInt) {
    sf64Matrix<D> prod;
    prod = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); ++i)
      eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
    return prod;
  }

  si64Matrix MPC_Mul(std::vector<si64Matrix> sharedInt);

  template <Decimal D>
  sf64<D> MPC_Mul_Const(const f64<D> &constFixed, const sf64<D> &sharedFixed) {
    sf64<D> ret;
    eval.asyncConstFixedMul(runtime, constFixed, sharedFixed, ret).get();
    return ret;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Dot_Mul(const sf64Matrix<D> &A, const sf64Matrix<D> &B) {
    return eval.asyncDotMul(runtime, A, B);
  }

  si64Matrix MPC_Dot_Mul(const si64Matrix &A, const si64Matrix &B);

  template <Decimal D>
  sf64Matrix<D> MPC_Mul_Const(const f64<D> &constFixed,
                              const sf64Matrix<D> &sharedFixed) {
    sf64Matrix<D> ret(sharedFixed.rows(), sharedFixed.cols());
    eval.asyncConstFixedMul(runtime, constFixed, sharedFixed, ret).get();
    return ret;
  }

  si64 MPC_Mul_Const(const i64 &constInt, const si64 &sharedInt);

  si64Matrix MPC_Mul_Const(const i64 &constInt,
                           const si64Matrix &sharedIntMatrix);
};

#endif
} // namespace primihub
