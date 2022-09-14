// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_COMMON_TYPE_FIXED_POINT_H_
#define SRC_primihub_COMMON_TYPE_FIXED_POINT_H_

#include <boost/multiprecision/cpp_int.hpp>
#include <utility>

#include "src/primihub/common/type/type.h"

namespace primihub {

enum Decimal { D0 = 0, D8 = 8, D16 = 16, D20 = 20, D32 = 32 };

// template<Decimal D>
// using f64 = fpml::fixed_point<i64, 63 - D, D>;
struct monostate {};

template <typename T, Decimal D> struct fp {
  static const Decimal mDecimal = D;

  T mValue = 0;

  fp() = default;
  fp(const fp &) = default;
  fp(fp &&) = default;

  fp(const double v) { *this = v; }

  fp operator+(const fp &rhs) const {
    return {mValue + rhs.mValue, monostate{}};
  }

  fp operator-(const fp &rhs) const {
    return {mValue - rhs.mValue, monostate{}};
  }

  fp operator*(const fp &rhs) const;
  fp operator>>(i64 shift) const { return {mValue >> shift, monostate{}}; }
  fp operator<<(i64 shift) const { return {mValue << shift, monostate{}}; }

  fp &operator+=(const fp &rhs) {
    mValue += rhs.mValue;
    return *this;
  }

  fp &operator-=(const fp &rhs) {
    mValue -= rhs.mValue;
    return *this;
  }

  fp &operator*=(const fp &rhs) {
    *this = *this * rhs;
    return *this;
  }

  fp &operator=(const fp &v) = default;

  operator double() const {
    return mValue / static_cast<double>(T(1) << mDecimal);
  }

  void operator=(const double &v) { mValue = T(v * (T(1) << D)); }

  bool operator==(const fp &v) const { return mValue == v.mValue; }

  bool operator!=(const fp &v) const { return !(*this == v); }

private:
  fp(T v, monostate) : mValue(v) {}
};

template <Decimal D> using f64 = fp<i64, D>;

template <typename T, Decimal D>
std::ostream &operator<<(std::ostream &o, const fp<T, D> &f) {
  auto mask = ((1ull << f.mDecimal) - 1);
  std::stringstream ss;
  u64 v;
  if (f.mValue >= 0) {
    v = f.mValue;
  } else {
    ss << '-';
    v = -f.mValue;
  }

  ss << (v >> f.mDecimal) << ".";

  v &= mask;
  if (v) {
    while (v & mask) {
      v *= 10;
      ss << (v >> f.mDecimal);
      v &= mask;
    }
  } else {
    ss << '0';
  }

  o << ss.str();
  return o;
}

template <typename T, Decimal D> struct fpMatrix {
  using value_type = fp<T, D>;
  static const Decimal mDecimal = D;
  eMatrix<value_type> mData;

  fpMatrix() = default;
  fpMatrix(const fpMatrix<T, D> &) = default;
  fpMatrix(fpMatrix<T, D> &&) = default;

  fpMatrix(u64 xSize, u64 ySize) : mData(xSize, ySize) {}

  void resize(u64 xSize, u64 ySize) { mData.resize(xSize, ySize); }

  u64 rows() const { return mData.rows(); }
  u64 cols() const { return mData.cols(); }
  u64 size() const { return mData.size(); }

  const fpMatrix<T, D> &operator=(const fpMatrix<T, D> &rhs) {
    mData = rhs.mData;
    return rhs;
  }

  fpMatrix<T, D> operator+(const fpMatrix<T, D> &rhs) const {
    return {mData + rhs.mData};
  }

  fpMatrix<T, D> operator-(const fpMatrix<T, D> &rhs) const {
    return {mData - rhs.mData};
  }

  fpMatrix<T, D> operator*(const fpMatrix<T, D> &rhs) const {
    fpMatrix<T, D> ret;
    eMatrix<i64> &view = ret.i64Cast();
    const eMatrix<i64> &l = i64Cast();
    const eMatrix<i64> &r = rhs.i64Cast();
    view = l * r;
    for (u64 i = 0; i < size(); ++i)
      view(i) >>= mDecimal;
    return ret;
  }

  fpMatrix<T, D> &operator+=(const fpMatrix<T, D> &rhs) {
    mData += rhs.mData;
    return *this;
  }

  fpMatrix<T, D> &operator-=(const fpMatrix<T, D> &rhs) {
    mData -= rhs.mData;
    return *this;
  }

  fpMatrix<T, D> &operator*=(const fpMatrix<T, D> &rhs) {
    auto &view = i64Cast();
    view *= rhs.i64Cast();
    for (u64 i = 0; i < size(); ++i)
      view(i) >>= mDecimal;
    return *this;
  }

  value_type &operator()(u64 x, u64 y) { return mData(x, y); }
  value_type &operator()(u64 xy) { return mData(xy); }
  const value_type &operator()(u64 x, u64 y) const { return mData(x, y); }
  const value_type &operator()(u64 xy) const { return mData(xy); }

  eMatrix<i64> &i64Cast() {
    static_assert(sizeof(value_type) == sizeof(i64),
                  "required for this operation");
    return reinterpret_cast<eMatrix<i64> &>(mData);
  }

  const eMatrix<i64> &i64Cast() const {
    static_assert(sizeof(value_type) == sizeof(i64),
                  "required for this operation");
    return reinterpret_cast<const eMatrix<i64> &>(mData);
  }

private:
  fpMatrix(const eMatrix<value_type> &v) : mData(v) {}
  fpMatrix(eMatrix<value_type> &&v)
      : mData(std::forward<eMatrix<value_type>>(v)) {}
};

template <Decimal D> using f64Matrix = fpMatrix<i64, D>;

template <typename T, Decimal D>
std::ostream &operator<<(std::ostream &o, const fpMatrix<T, D> &f) {
  o << '[';
  for (u64 i = 0; i < f.rows(); ++i) {
    o << '(';
    if (f.cols())
      o << f(i, 0);

    for (u64 j = 1; j < f.cols(); ++j)
      o << ", " << f(i, j);

    o << ")\n";
  }
  o << ']';
  return o;
}

template <Decimal D> struct sf64 {
  static const Decimal mDecimal = D;

  using value_type = si64::value_type;
  si64 mShare;

  sf64() = default;
  sf64(const sf64<D> &) = default;
  sf64(sf64<D> &&) = default;
  sf64(const std::array<value_type, 2> &d) : mShare(d) {}
  sf64(const Ref<sf64<D>> &s) {
    mShare.mData[0] = *s.mData[0];
    mShare.mData[1] = *s.mData[1];
  }

  sf64<D> &operator=(const sf64<D> &copy) {
    mShare = copy.mShare;
    return *this;
  }

  sf64<D> operator+(const sf64<D> &rhs) const {
    sf64<D> r;
    r.mShare = mShare + rhs.mShare;
    return r;
  }

  sf64<D> operator-(const sf64<D> &rhs) const {
    sf64<D> r;
    r.mShare = mShare - rhs.mShare;
    return r;
  }

  value_type &operator[](u64 i) { return mShare[i]; }
  const value_type &operator[](u64 i) const { return mShare[i]; }

  si64 &i64Cast() { return mShare; }
  const si64 &i64Cast() const { return mShare; }
};

template <Decimal D> struct sf64Matrix : private si64Matrix {
  static const Decimal mDecimal = D;

  struct ConstRow {
    const sf64Matrix<D> &mMtx;
    const u64 mIdx;
  };

  struct Row {
    sf64Matrix<D> &mMtx;
    const u64 mIdx;
    const Row &operator=(const Row &row);
    const ConstRow &operator=(const ConstRow &row);
  };

  struct ConstCol {
    const sf64Matrix<D> &mMtx;
    const u64 mIdx;
  };

  struct Col {
    sf64Matrix<D> &mMtx;
    const u64 mIdx;
    const Col &operator=(const Col &col);
    const ConstCol &operator=(const ConstCol &row);
  };

  sf64Matrix() = default;
  sf64Matrix(u64 xSize, u64 ySize) { resize(xSize, ySize); }

  void resize(u64 xSize, u64 ySize) {
    mShares[0].resize(xSize, ySize);
    mShares[1].resize(xSize, ySize);
  }

  u64 rows() const { return mShares[0].rows(); }
  u64 cols() const { return mShares[0].cols(); }
  u64 size() const { return mShares[0].size(); }

  Ref<sf64<D>> operator()(u64 x, u64 y) {
    typename sf64<D>::value_type &s0 = mShares[0](x, y);
    typename sf64<D>::value_type &s1 = mShares[1](x, y);
    return Ref<sf64<D>>(s0, s1);
  }

  Ref<sf64<D>> operator()(u64 xy) {
    auto &s0 = static_cast<typename sf64<D>::value_type &>(mShares[0](xy));
    auto &s1 = static_cast<typename sf64<D>::value_type &>(mShares[1](xy));
    return Ref<sf64<D>>(s0, s1);
  }

  const sf64Matrix<D> &operator=(const sf64Matrix<D> &B) {
    mShares = B.mShares;
    return B;
  }

  sf64Matrix<D> &operator+=(const sf64Matrix<D> &B) {
    mShares[0] += B.mShares[0];
    mShares[1] += B.mShares[1];
    return *this;
  }

  sf64Matrix<D> &operator-=(const sf64Matrix<D> &B) {
    mShares[0] -= B.mShares[0];
    mShares[1] -= B.mShares[1];
    return *this;
  }

  sf64Matrix<D> operator+(const sf64Matrix<D> &B) const {
    sf64Matrix<D> r = *this;
    r += B;
    return r;
  }

  sf64Matrix<D> operator-(const sf64Matrix<D> &B) const {
    sf64Matrix<D> r = *this;
    r -= B;
    return r;
  }

  sf64Matrix<D> transpose() const {
    sf64Matrix<D> r = *this;
    r.transposeInPlace();
    return r;
  }

  void transposeInPlace() {
    mShares[0].transposeInPlace();
    mShares[1].transposeInPlace();
  }

  Row row(u64 i) { return Row{*this, i}; }
  Col col(u64 i) { return Col{*this, i}; }
  ConstRow row(u64 i) const { return ConstRow{*this, i}; }
  ConstCol col(u64 i) const { return ConstCol{*this, i}; }

  bool operator!=(const sf64Matrix<D> &b) const { return !(*this == b); }

  bool operator==(const sf64Matrix<D> &b) const {
    return (rows() == b.rows() && cols() == b.cols() && mShares == b.mShares);
  }

  si64Matrix &i64Cast() { return static_cast<si64Matrix &>(*this); }

  const si64Matrix &i64Cast() const {
    return static_cast<const si64Matrix &>(*this);
  }

  eMatrix<i64> &operator[](u64 i) { return mShares[i]; }
  const eMatrix<i64> &operator[](u64 i) const { return mShares[i]; }
};

template <Decimal D>
inline const typename sf64Matrix<D>::Row &
sf64Matrix<D>::Row::operator=(const Row &row) {
  mMtx.mShares[0].row(mIdx) = row.mMtx.mShares[0].row(row.mIdx);
  mMtx.mShares[1].row(mIdx) = row.mMtx.mShares[1].row(row.mIdx);

  return row;
}

template <Decimal D>
inline const typename sf64Matrix<D>::ConstRow &
sf64Matrix<D>::Row::operator=(const ConstRow &row) {
  mMtx.mShares[0].row(mIdx) = row.mMtx.mShares[0].row(row.mIdx);
  mMtx.mShares[1].row(mIdx) = row.mMtx.mShares[1].row(row.mIdx);
  return row;
}

template <Decimal D>
inline const typename sf64Matrix<D>::Col &
sf64Matrix<D>::Col::operator=(const Col &row) {
  mMtx.mShares[0].col(mIdx) = row.mMtx.mShares[0].col(row.mIdx);
  mMtx.mShares[1].col(mIdx) = row.mMtx.mShares[1].col(row.mIdx);

  return row;
}

template <Decimal D>
inline const typename sf64Matrix<D>::ConstCol &
sf64Matrix<D>::Col::operator=(const ConstCol &row) {
  mMtx.mShares[0].col(mIdx) = row.mMtx.mShares[0].col(row.mIdx);
  mMtx.mShares[1].col(mIdx) = row.mMtx.mShares[1].col(row.mIdx);
  return row;
}

} // namespace primihub

#endif // SRC_primihub_COMMON_TYPE_FIXED_POINT_H_
