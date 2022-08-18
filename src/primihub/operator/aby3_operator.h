
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
  IOService ios;
  Sh3Encryptor enc;
  Sh3BinaryEvaluator binEval;
  Sh3Evaluator eval;
  Sh3Runtime runtime;
  u64 partyIdx;
  string next_name;
  string prev_name;

  MPCOperator(u64 partyIdx_, string NextName, string PrevName)
      : partyIdx(partyIdx_), next_name(NextName), prev_name(PrevName) {}

  int setup(std::string ip, u32 next_port, u32 prev_port);

  template <Decimal D>
  sf64Matrix<D> createShares(const eMatrix<double> &vals,
                             sf64Matrix<D> &sharedMatrix) {
    f64Matrix<D> fixedMatrix(vals.rows(), vals.cols());
    for (u64 i = 0; i < vals.size(); ++i)
      fixedMatrix(i) = vals(i);
    enc.localFixedMatrix(runtime, fixedMatrix, sharedMatrix).get();
    return sharedMatrix;
  }

  template <Decimal D> sf64Matrix<D> createShares(sf64Matrix<D> &sharedMatrix) {
    enc.remoteFixedMatrix(runtime, sharedMatrix).get();
    return sharedMatrix;
  }

  si64Matrix createShares(const i64Matrix &vals, si64Matrix &sharedMatrix);

  si64Matrix createShares(si64Matrix &sharedMatrix);

  template <Decimal D>
  sf64<D> createShares(double vals, sf64<D> &sharedFixedInt) {
    enc.localFixed<D>(runtime, vals, sharedFixedInt).get();
    return sharedFixedInt;
  }

  template <Decimal D> sf64<D> createShares(sf64<D> &sharedFixedInt) {
    enc.remoteFixed<D>(runtime, sharedFixedInt).get();
    return sharedFixedInt;
  }

  template <Decimal D> eMatrix<double> revealAll(const sf64Matrix<D> &vals) {
    f64Matrix<D> temp(vals.rows(), vals.cols());
    enc.revealAll(runtime, vals, temp).get();

    eMatrix<double> ret(vals.rows(), vals.cols());
    for (u64 i = 0; i < ret.size(); ++i)
      ret(i) = static_cast<double>(temp(i));
    return ret;
  }
  void createBinShares(i64Matrix &vals, sbMatrix &ret);
  void createBinShares(sbMatrix &ret);

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

  template <Decimal D> double reveal(const sf64<D> &vals, u64 Idx) {
    if (Idx == partyIdx) {
      f64<D> ret;
      enc.reveal(runtime, vals, ret).get();
      return static_cast<double>(ret);
    } else {
      enc.reveal(runtime, Idx, vals).get();
    }
  }

  template <Decimal D>
  sf64<D> MPC_Add(std::vector<sf64<D>> sharedFixedInt, sf64<D> &sum) {
    sum = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); i++) {
      sum = sum + sharedFixedInt[i];
    }
    return sum;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Add(std::vector<sf64Matrix<D>> sharedFixedInt,
                        sf64Matrix<D> &sum) {
    sum = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); i++) {
      sum = sum + sharedFixedInt[i];
    }
    return sum;
  }

  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum);

  template <Decimal D>
  sf64<D> MPC_Sub(sf64<D> minuend, std::vector<sf64<D>> subtrahends,
                  sf64<D> &difference) {
    difference = minuend;
    for (u64 i = 0; i < subtrahends.size(); i++) {
      difference = difference - subtrahends[i];
    }
    return difference;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Sub(sf64Matrix<D> minuend,
                        std::vector<sf64Matrix<D>> subtrahends,
                        sf64Matrix<D> &difference) {
    difference = minuend;
    for (u64 i = 0; i < subtrahends.size(); i++) {
      difference = difference - subtrahends[i];
    }
    return difference;
  }

  si64Matrix MPC_Sub(si64Matrix minuend, std::vector<si64Matrix> subtrahends,
                     si64Matrix &difference);

  template <Decimal D>
  sf64<D> MPC_Mul(std::vector<sf64<D>> sharedFixedInt, sf64<D> &prod) {
    prod = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); ++i)
      eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
    return prod;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Mul(std::vector<sf64Matrix<D>> sharedFixedInt,
                        sf64Matrix<D> &prod) {
    prod = sharedFixedInt[0];
    for (u64 i = 1; i < sharedFixedInt.size(); ++i)
      eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
    return prod;
  }

  si64Matrix MPC_Mul(std::vector<si64Matrix> sharedInt, si64Matrix &prod);
};

#endif
} // namespace primihub
