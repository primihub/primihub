// "Copyright [2021] <PrimiHub>"
#ifndef SRC_PRIMIHUB_ALGORITHM_ABY3ML_H_
#define SRC_PRIMIHUB_ALGORITHM_ABY3ML_H_

#include <algorithm>
#include <random>
#include <vector>
#include <memory>

#include "cryptoTools/Common/Defines.h"
#include "aby3/sh3/Sh3Types.h"
#include "aby3/sh3/Sh3Encryptor.h"
#include "aby3/sh3/Sh3Evaluator.h"
#include "aby3/sh3/Sh3Piecewise.h"
#include "aby3/sh3/Sh3ShareGen.h"
#include "aby3/sh3/Sh3FixedPoint.h"

#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/Session.h"
#include "network/channel_interface.h"

using Channel = primihub::link::Channel;
using Session = osuCrypto::Session;

namespace primihub {
using namespace aby3;   // NOLINT
class aby3ML {
 public:
  // std::unique_ptr<aby3::CommPkg> comm_pkg_{nullptr};
  aby3::CommPkg* comm_pkg_ref_{nullptr};
  Sh3Encryptor mEnc;
  Sh3Evaluator mEval;
  Sh3Runtime mRt;
  bool mPrint = true;

  u64 partyIdx() { return mRt.mPartyIdx;}
#ifdef MPC_SOCKET_CHANNEL
  void init(u64 partyIdx, Session& prev, Session& next, block seed);
#endif  // MPC_SOCKET_CHANNEL
  void init(u64 partyIdx, std::unique_ptr<aby3::CommPkg> comm_pkg, block seed);
  void init(u64 partyIdx, aby3::CommPkg* comm_pkg, block seed);

  void fini(void);
  Channel& mNext() {return comm_pkg_ref_->mNext;}
  Channel& mPrev() {return comm_pkg_ref_->mPrev;}

  template<Decimal D>
  sf64Matrix<D> localInput(const f64Matrix<D>& val) {
    std::array<u64, 2> size{ val.rows(), val.cols() };
    this->mNext().asyncSendCopy(size);
    this->mPrev().asyncSendCopy(size);
    sf64Matrix<D> dest(size[0], size[1]);
    mEnc.localFixedMatrix(mRt.noDependencies(), val, dest).get();
    return dest;
  }

  //
  template<Decimal D>
  sf64<D> localInput(const f64<D>& val) {
    sf64<D> dest;
    mEnc.localFixed(mRt.noDependencies(), val, dest).get();
    return dest;
  }

  void localInputSize(const eMatrix<double>& val) {
    std::array<u64, 2> size{
      static_cast<u64>(val.rows()),
      static_cast<u64>(val.cols())
    };
    this->mNext().send(size);
    this->mPrev().send(size);
    return;
  }

  template<Decimal D>
  sf64Matrix<D> localInput(const eMatrix<double>& vals) {
    f64Matrix<D> v2(vals.rows(), vals.cols());
    for (i64 i = 0; i < vals.size(); ++i) {
      v2(i) = vals(i);
    }
    return localInput(v2);
  }

  //  share double (local)
  template<Decimal D>
  sf64<D> localInput(const double & vals) {
    f64<D> v2 = vals;
    return localInput(v2);
  }

  template<Decimal D>
  sf64Matrix<D> remoteInput(u64 partyIdx) {
    std::array<u64, 2> size;
    if (partyIdx == (mRt.mPartyIdx + 1) % 3) {
      this->mNext().recv(size);
    } else if (partyIdx == (mRt.mPartyIdx + 2) % 3) {
      this->mPrev().recv(size);
    } else {
      throw RTE_LOC;
    }

    sf64Matrix<D> dest(size[0], size[1]);
    mEnc.remoteFixedMatrix(mRt.noDependencies(), dest).get();
    return dest;
  }

  //  share double (remote)
  template<Decimal D>
  sf64<D> remoteInput(void) {
    sf64<D> dest;
    mEnc.remoteFixed(mRt.noDependencies(), dest).get();
    return dest;
  }

  std::array<u64, 2> remoteInputSize(u64 partyIdx) {
    std::array<u64, 2> size;
    if (partyIdx == (mRt.mPartyIdx + 1) % 3) {
      this->mNext().recv(size);
    } else if (partyIdx == (mRt.mPartyIdx + 2) % 3) {
      this->mPrev().recv(size);
    } else {
      throw RTE_LOC;
    }
    return size;
  }

  void preprocess(u64 n, Decimal d) {
    TODO("implement this");
  }

  template<Decimal D>
  eMatrix<double> reveal(const sf64Matrix<D>& vals) {
    f64Matrix<D> temp(vals.rows(), vals.cols());
    mEnc.revealAll(mRt.noDependencies(), vals, temp).get();

    eMatrix<double> ret(vals.rows(), vals.cols());
    for (i64 i = 0; i < ret.size(); ++i) {
      ret(i) = static_cast<double>(temp(i));
    }
    return ret;
  }

  eMatrix<double> reveal(const si64Matrix& vals) {
    i64Matrix temp(vals.rows(), vals.cols());
    mEnc.revealAll(mRt.noDependencies(), vals, temp).get();

    eMatrix<double> ret(vals.rows(), vals.cols());
    for (i64 i = 0; i < ret.size(); ++i) {
      ret(i) = static_cast<double>(temp(i));
    }
    return ret;
  }

  //
  template<Decimal D>
  sf64<D> mul(const sf64<D>& left, const sf64<D>& right) {
    sf64<D> dest;
    mEval.asyncMul(mRt.noDependencies(), left, right, dest).get();
    return dest;
  }

  template<Decimal D>
  double reveal(const sf64<D>& val) {
    f64<D> dest;
    mEnc.revealAll(mRt.noDependencies(), val, dest).get();
    return static_cast<double>(dest);
  }

  //
  template<Decimal D>
  sf64<D> constMul(const i64& a, const sf64<D>& vals) {
    sf64<D> dest;
    mEval.asyncConstMul(a, vals, dest);
    return dest;
  }

  //
  template<Decimal D>
  sf64Matrix<D> constMul(const i64& a, const sf64Matrix<D>& vals) {
    sf64Matrix<D> dest;
    mEval.asyncConstMul(a, vals, dest);
    return dest;
  }

  //
  template<Decimal D>
  sf64<D> constFixedMul(const double& a, const sf64<D>& vals) {
    sf64<D> dest;
    f64<D> t = a;
    mEval.asyncConstFixedMul(mRt.noDependencies(), t, vals, dest).get();
    return dest;
  }

  //
  template<Decimal D>
  sf64Matrix<D> constFixedMul(const double& a, const sf64Matrix<D>& vals) {
    sf64Matrix<D> dest;
    f64<D> t = a;
    mEval.asyncConstFixedMul(mRt.noDependencies(), t, vals, dest).get();
    return dest;
  }

  //
  template<Decimal D>
  sf64<D> FixedPow(const u64& n, const sf64<D>& B) {
    i64 i, j;
    si64 dest;
    if ((n >= 16) || (n == 0)) {
      throw std::runtime_error(LOCATION);
    }

    i = 3;
    while (((n >> i) & 0x1) == 0) {
      i--;
    }

    sf64<D> C = B;
    for (j = i - 1; j >= 0; j--) {
      C = mul(C, C);
      if ((n >> j) & 0x1) {
        C = mul(C, B);
      }
    }

    return C;
  }

  //
  template<Decimal D>
  sf64Matrix<D> FixedPow(const u64& n, const sf64Matrix<D>& B) {
    i64 i, j;
    sf64Matrix<D> dest;
    if ((n >= 16) || (n == 0)) {
      throw std::runtime_error(LOCATION);
    }

    i = 3;
    while (((n >> i) & 0x1) == 0) {
      i--;
    }

    sf64Matrix<D> C = B;
    for (j = i - 1; j >= 0; j--) {
      C = mul(C, C);
      if ((n >> j) & 0x1) {
        C = mul(C, B);
      }
    }

    return C;
  }

  //
  template<Decimal D>
  sf64<D> average(sf64<D> *A, u64 n) {
    double inv = 1.0 / n;
    sf64<D> dest = A[0];

    for (u64 i = 1; i < n; i++) {
      dest = dest + A[i];
    }

    dest = constFixedMul(inv, dest);

    return dest;
  }

  //
  template<Decimal D>
  sf64Matrix<D> average(sf64Matrix<D> *A, u64 n) {
    double inv = 1.0 / n;
    sf64Matrix<D> dest = A[0];

    for (u64 i = 1; i < n; i++) {
      dest = dest + A[i];
    }

    dest = constFixedMul(inv, dest);

    return dest;
  }

  template<Decimal D>
  double reveal(const Ref<sf64<D>>& val) {
    sf64<D> v2(val);
    return reveal(v2);
  }

  template<Decimal D>
  sf64Matrix<D> mul(const sf64Matrix<D>& left, const sf64Matrix<D>& right) {
    sf64Matrix<D> dest;
    mEval.asyncMul(mRt.noDependencies(), left, right, dest).get();
    return dest;
  }

  template<Decimal D>
  sf64Matrix<D> mulTruncate(const sf64Matrix<D>& left,
    const sf64Matrix<D>& right, u64 shift) {
    sf64Matrix<D> dest;
    mEval.asyncMul(mRt.noDependencies(), left, right, dest, shift).get();
    return dest;
  }

  Sh3Piecewise mLogistic;

  template<Decimal D>
  sf64Matrix<D> logisticFunc(const sf64Matrix<D>& Y) {
    if (mLogistic.mThresholds.size() == 0) {
      mLogistic.mThresholds.resize(2);
      mLogistic.mThresholds[0] = -0.5;
      mLogistic.mThresholds[1] = 0.5;
      mLogistic.mCoefficients.resize(3);
      mLogistic.mCoefficients[1].resize(2);
      mLogistic.mCoefficients[1][0] = 0.5;
      mLogistic.mCoefficients[1][1] = 1;
      mLogistic.mCoefficients[2].resize(1);
      mLogistic.mCoefficients[2][0] = 1;
    }

    sf64Matrix<D> out(Y.rows(), Y.cols());
    mLogistic.eval<D>(mRt.noDependencies(), Y, out, mEval);
    return out;
  }

  // added on 20210524 by 007
  template<Decimal D>
  sf64Matrix<D> lt(const sf64Matrix<D>& Y, double threshold) {
    mLogistic.mThresholds.resize(1);
    mLogistic.mThresholds[0] = threshold;
    mLogistic.mCoefficients.resize(2);
    mLogistic.mCoefficients[0].resize(1);
    mLogistic.mCoefficients[0][0] = 1;

    sf64Matrix<D> out(Y.rows(), 1);
    mLogistic.eval<D>(mRt.noDependencies(), Y, out, mEval);
    return out;
  }

  si64Matrix lt(const si64Matrix& Y, double threshold) {
    mLogistic.mThresholds.resize(1);
    mLogistic.mThresholds[0] = threshold;
    mLogistic.mCoefficients.resize(2);
    mLogistic.mCoefficients[0].resize(1);
    mLogistic.mCoefficients[0][0] = 1;

    si64Matrix out(Y.rows(), 1);
    mLogistic.eval(mRt.noDependencies(), Y, out, mEval);
    return out;
  }

  template<Decimal D>
  bool lt(const sf64<D>& X, const sf64<D>& Y) {  // X < Y ?
    auto Z = X - Y;
    si64Matrix ZZ;
    ZZ.resize(1, 1);
    ZZ.mShares[0](0, 0) = Z.mShare.mData[0];
    ZZ.mShares[1](0, 0) = Z.mShare.mData[1];
    double threshold = 0.0;
    auto out = lt(ZZ, threshold);  // out: si64Matrix;
    i64Matrix dest;
    dest.resize(1, 1);
    mEnc.revealAll(mRt.noDependencies(), out, dest).get();

    return dest(0, 0) != 0;
  }

  bool lt(const si64& X, double threshold) {  // X < threshold ?
    si64Matrix ZZ;
    ZZ.resize(1, 1);
    ZZ.mShares[0](0, 0) = X.mData[0];
    ZZ.mShares[1](0, 0) = X.mData[1];
    auto out = lt(ZZ, threshold);  // out: si64Matrix;
    i64Matrix dest;
    dest.resize(1, 1);
    mEnc.revealAll(mRt.noDependencies(), out, dest).get();

    return dest(0, 0) != 0;
  }

  // added on 20210525 by 007
  static int get_randaom() {
    std::default_random_engine e;
    e.seed(time(nullptr));

    if (e() != 0) {
      return static_cast<int>(e());
    } else {
      return static_cast<int>(e() + 1);
    }
  }

  template<Decimal D>
  si64Matrix eq(const sf64Matrix<D>& X, const sf64Matrix<D>& Y) {
    auto Z = X - Y;
    si64Matrix dest;

    mEval.asyncConstMul_test(mRt.noDependencies(), get_randaom(),
      Z.i64Cast(), dest).get();

    return dest;
  }

  bool eq(const si64& X, const si64& Y) {
    auto Z = X - Y;
    si64 dest;
    i64 result;
    mEval.asyncConstMul_test(mRt.noDependencies(), get_randaom(), Z, dest);

    mEnc.revealAll(mRt.noDependencies(), dest, result).get();

    return result == 0;
  }

  bool is_zero(si64& X) {
    si64 dest;
    i64 result;
    mEval.asyncConstMul_test(mRt.noDependencies(), get_randaom(), X, dest);
    mEnc.revealAll(mRt.noDependencies(), dest, result).get();

    return result == 0;
  }

  template<Decimal D>
  bool gt(sf64<D>& X, double threshold) {
    auto lt_flag = lt(X.i64Cast(), threshold);

    if (lt_flag) {
      return false;
    } else {
        return !is_zero(X.i64Cast());
    }
  }

  int Sign(si64& X) {
    auto lt_flag = lt(X, 0.0);
    if (lt_flag)
      return -1;
    auto zero_flag = is_zero(X);
    if (zero_flag)
      return 0;
    else
      return 1;
  }

  template<Decimal D>
  int sort(const std::vector<sf64<D>> X, u64 size,
    std::vector<sf64<D>>& dest) {
    if (X.size() < 1) return 0;
    std::vector<sf64<D>> Y(size);
    for (int i = 0; i < size; i++) Y[i] = X[i];
    sf64<D> swap;
    for (int i = 0; i < size; i++) {
        for (int j = i+1; j < size; j++) {
            sf64<D> Z = Y[i] - Y[j];
            if (gt(Z, 0.0)) {
                swap = Y[i];
                Y[i] = Y[j];
                Y[j] = swap;
            }
        }
    }
    for (int i = 0; i < size; i++)
      dest[i] = Y[i];
    return 1;
  }

  template<Decimal D>
  int unique_unsorted(const std::vector<sf64<D>> X, const u64 size,
    std::vector<sf64<D>>& dest, u64 &dest_size) {
    if (X.size() < 2 || size < 2) return  0;

    std::vector<sf64<D>> Y(size);
    for (int i = 0; i < size; i++) Y[i] = X[i];

    int count = 0;
    for (int i = 0; i < size; i++) {
      for (int j = i + 1; j < size-count; j++) {
        if (eq(Y[i].i64Cast(), Y[j].i64Cast())) {
          auto iter = Y.erase(std::begin(Y) + j);
          count++;
        }
      }
    }
    // std::cout << "cout = " << count << std::endl;
    for (int i = 0; i < size-count; i++) dest[i] = Y[i];
    dest_size = size - count;
    return  1;
  }

  template<typename T>
  aby3ML& operator<<(const T& v) {
    if (partyIdx() == 0 && mPrint)
      std::cout << v;
    return *this;
  }

  template<typename T>
  aby3ML& operator<<(T& v) {
    if (partyIdx() == 0 && mPrint)
      std::cout << v;
    return *this;
  }

  aby3ML& operator<< (std::ostream& (*v)(std::ostream&)) {
    if (partyIdx() == 0 && mPrint)
      std::cout << v;
    return *this;
  }

  aby3ML& operator<< (std::ios& (*v)(std::ios&)) {
    if (partyIdx() == 0 && mPrint)
      std::cout << v;
    return *this;
  }

  aby3ML& operator<< (std::ios_base& (*v)(std::ios_base&)) {
    if (partyIdx() == 0 && mPrint)
      std::cout << v;
    return *this;
  }
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_ALGORITHM_ABY3ML_H_
