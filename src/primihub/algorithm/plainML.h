// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_ALGORITHM_PLAINML_H_
#define SRC_PRIMIHUB_ALGORITHM_PLAINML_H_

#include <iostream>

#include "aby3/Common/Defines.h"
#include "aby3/sh3/Sh3Types.h"
#include "src/primihub/common/common.h"

namespace primihub {
template<typename T>
using eMatrix = aby3::eMatrix<T>;

class PlainML {
 public:
  eMatrix<double> mul(const eMatrix<double>& left,
    const eMatrix<double>& right) {
    return left * right;
  }

  eMatrix<double> mulTruncate(const eMatrix<double>& left,
    const eMatrix<double>& right, u64 shift) {
    auto div = 1ull << shift;
    eMatrix<double> ret = left * right;
    for (i64 i = 0; i < ret.size(); ++i) {
      ret(i) /= div;
    }

    return ret;
  }

  eMatrix<double> logisticFunc(eMatrix<double> x) {
    for (i64 i = 0; i < x.size(); ++i)
      x(i) = 1.0 / (1 + std::exp(-x(i)));
    return x;
  }

  double reveal(const double& v) {
    return v;
  }

  eMatrix<double> reveal(const eMatrix<double>& v) {
    return v;
  }

  u64 partyIdx() {
    return 0;
  }

  bool mPrint = true;

  template<typename T>
  PlainML& operator<<(const T& v) {
    if (mPrint) std::cout << v;
    return *this;
  }

  template<typename T>
  PlainML& operator<<(T& v) {
    if (mPrint) std::cout << v;
    return *this;
  }

  PlainML& operator<< (std::ostream& (*v)(std::ostream&)) {
    if (mPrint) std::cout << v;
    return *this;
  }

  PlainML& operator<< (std::ios& (*v)(std::ios&)) {
    if (mPrint) std::cout << v;
    return *this;
  }

  PlainML& operator<< (std::ios_base& (*v)(std::ios_base&)) {
    if (mPrint) std::cout << v;
    return *this;
  }
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_ALGORITHM_PLAINML_H_
