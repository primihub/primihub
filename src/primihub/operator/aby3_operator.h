#ifndef SRC_primihub_operator_ABY3_operator_H
#define SRC_primihub_operator_ABY3_operator_H

#include <Eigen/Dense>
#include <algorithm>
#include <random>
#include <unistd.h>
#include <vector>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/primitive/ppa/kogge_stone.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/binary_evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/piecewise.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/eigen_util.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {

const uint8_t VAL_BITCOUNT = 64;
const Decimal D = D16;

class MPCOperator {
public:
  Channel mNext, mPrev;
  IOService ios;
  Sh3Encryptor enc;
  Sh3BinaryEvaluator binEval;
  Sh3ShareGen gen;
  Sh3Piecewise mdivision;
  Sh3Piecewise mAbs;
  Sh3Piecewise mQuoDertermine;

  Sh3Evaluator eval;
  Sh3Runtime runtime;
  u64 partyIdx;
  string next_name;
  string prev_name;

  MPCOperator(u64 partyIdx_, string NextName, string PrevName)
      : partyIdx(partyIdx_), next_name(NextName), prev_name(PrevName) {}

  int setup(std::string next_ip, std::string prev_ip, u32 next_port,
            u32 prev_port);
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

  // Reveal binary share to one or more party.
  i64Matrix reveal(const sbMatrix &sh_res);
  void reveal(const sbMatrix &sh_res, uint64_t party_id);

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
        for (i64 j = 0; j < sharedFixed.cols(); j++)
          temp[0](i, j) = sharedFixed[0](i, j) + constfixed.mValue;
    else if (partyIdx == 1)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++) {
          temp[1](i, j) = sharedFixed[1](i, j) + constfixed.mValue;
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
  sf64<D> MPC_Sub_Const(f64<D> constfixed, sf64<D> &sharedFixed, bool mode) {
    sf64<D> temp = sharedFixed;
    if (partyIdx == 0)
      temp[0] = sharedFixed[0] - constfixed.mValue;
    else if (partyIdx == 1)
      temp[1] = sharedFixed[1] - constfixed.mValue;
    if (mode != true) {
      temp[0] = -temp[0];
      temp[1] = -temp[1];
    }
    return temp;
  }

  template <Decimal D>
  sf64Matrix<D> MPC_Sub_Const(f64<D> constfixed, sf64Matrix<D> &sharedFixed,
                              bool mode) {
    sf64Matrix<D> temp = sharedFixed;

    if (partyIdx == 0)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++)
          temp[0](i, j) = sharedFixed[0](i, j) - constfixed.mValue;
    else if (partyIdx == 1)
      for (i64 i = 0; i < sharedFixed.rows(); i++)
        for (i64 j = 0; j < sharedFixed.cols(); j++)
          temp[1](i, j) = sharedFixed[1](i, j) - constfixed.mValue;
    if (mode != true) {
      temp[0] = -temp[0];
      temp[1] = -temp[1];
    }
    return temp;
  }

  si64 MPC_Sub_Const(i64 constInt, si64 &sharedInt, bool mode);

  si64Matrix MPC_Sub_Const(i64 constInt, si64Matrix &sharedIntMatrix,
                           bool mode);

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
  sf64Matrix<D> MPC_Mul_Const(const f64<D> &constFixed,
                              const sf64Matrix<D> &sharedFixed) {
    sf64Matrix<D> ret(sharedFixed.rows(), sharedFixed.cols());
    eval.asyncConstFixedMul(runtime, constFixed, sharedFixed, ret).get();
    return ret;
  }

  si64 MPC_Mul_Const(const i64 &constInt, const si64 &sharedInt);

  si64Matrix MPC_Mul_Const(const i64 &constInt,
                           const si64Matrix &sharedIntMatrix);

  template <Decimal D> void MPC_Compare(f64Matrix<D> &m, sbMatrix &sh_res) {
    // Get matrix shape of all party.
    std::vector<std::array<uint64_t, 2>> all_party_shape;
    for (uint64_t i = 0; i < 3; i++) {
      std::array<uint64_t, 2> shape;
      if (partyIdx == i) {
        shape[0] = m.rows();
        shape[1] = m.cols();
        mNext.asyncSendCopy(shape);
        mPrev.asyncSendCopy(shape);
      } else {
        if (partyIdx == (i + 1) % 3)
          mPrev.recv(shape);
        else if (partyIdx == (i + 2) % 3)
          mNext.recv(shape);
        else
          throw std::runtime_error("Message recv logic error.");
      }

      all_party_shape.emplace_back(shape);
    }

    {
      LOG(INFO) << "Dump shape of all party's matrix:";
      for (uint64_t i = 0; i < 3; i++)
        LOG(INFO) << "Party " << i << ": (" << all_party_shape[i][0] << ", "
                << all_party_shape[i][1] << ")";
      LOG(INFO) << "Dump finish.";
    }

    // The compare is a binary operator, so only two party will provide value
    // for compare, find out which party don't provide value.
    int skip_index = -1;
    for (uint64_t i = 0; i < 3; i++) {
      if (all_party_shape[i][0] == 0 && all_party_shape[i][1] == 0) {
        if (skip_index != -1) {
          throw std::runtime_error(
              "There are two party that don't provide any value at last, but "
              "compare operator require value from two party.");
        } else {
          skip_index = i;
        }
      }
    }

    if (skip_index == -1)
      throw std::runtime_error(
          "This operator is binary, can only handle value from two party.");

    // Shape of matrix in two party shoubld be the same.
    if (skip_index == 0) {
      all_party_shape[0][0] = all_party_shape[1][0];
      all_party_shape[0][1] = all_party_shape[1][1];
      all_party_shape[1][0] = all_party_shape[2][0];
      all_party_shape[1][1] = all_party_shape[2][1];
    } else if (skip_index != 2) {
      all_party_shape[1][0] = all_party_shape[2][0];
      all_party_shape[1][1] = all_party_shape[2][1];
    }

    if ((all_party_shape[0][0] != all_party_shape[1][0]) ||
        (all_party_shape[0][1] != all_party_shape[1][1]))
      throw std::runtime_error(
          "Shape of matrix in two party must be the same.");
    
    // Set value to it's negative for some party. 
    if (skip_index == 0 || skip_index == 1) {
      if (partyIdx == 2)
        m.mData = m.mData.array() * -1;
    } else {
      if (partyIdx == 1)
        m.mData = m.mData.array() * -1;
    }

    LOG(INFO) << "Party " << (skip_index + 1) % 3 << " and party "
              << (skip_index + 2) % 3 << " provide value for MPC compare.";

    // Create binary shares.
    uint64_t num_elem = all_party_shape[0][0] * all_party_shape[0][1];
    m.resize(num_elem, 1);

    std::vector<sbMatrix> sh_m_vec;
    for (uint64_t i = 0; i < 3; i++) {
      if (static_cast<int>(i) == skip_index)
        continue;
      if (i == partyIdx) {
        sbMatrix sh_m(num_elem, VAL_BITCOUNT);
        auto task = runtime.noDependencies();
        task.get();
        enc.localBinMatrix(task, m.i64Cast(), sh_m).get();
        sh_m_vec.emplace_back(sh_m);
      } else {
        sbMatrix sh_m(num_elem, VAL_BITCOUNT);
        auto task = runtime.noDependencies();
        task.get();
        enc.remoteBinMatrix(task, sh_m).get();
        sh_m_vec.emplace_back(sh_m);
      }
    }

    LOG(INFO) << "Create binary share for value from two party finish.";

    // Build then run MSB circuit.
    KoggeStoneLibrary lib;
    uint64_t size = 64;
    BetaCircuit *cir = lib.int_int_add_msb(size);
    cir->levelByAndDepth();

    sh_res.resize(m.size(), 1);
    std::vector<const sbMatrix *> input = {&sh_m_vec[0], &sh_m_vec[1]};
    std::vector<sbMatrix *> output = {&sh_res};
    auto task = runtime.noDependencies();
    task = binEval.asyncEvaluate(task, cir, gen, input, output);
    task.get();
  
    // Recover original value.
    if (skip_index == 0 || skip_index == 1) {
      if (partyIdx == 2)
        m.mData = m.mData.array() * -1;
    } else {
      if (partyIdx == 1)
        m.mData = m.mData.array() * -1;
    }

    LOG(INFO) << "Finish evalute MSB circuit.";
  }

  void MPC_Compare(sbMatrix &sh_res);
};

#endif
} // namespace primihub
