// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_COMMON_TYPE_MATRIX_VIEW_H_
#define SRC_primihub_COMMON_TYPE_MATRIX_VIEW_H_
#include <array>
#include <tuple>

#include "src/primihub/common/defines.h"

namespace primihub {

template<class T>
class MatrixView {
 public:
#ifdef ENABLE_FULL_GSL
  using iterator = gsl::details::span_iterator<gsl::span<T>, false>;
  using const_iterator = gsl::details::span_iterator<gsl::span<T>, true>;
#else
  typedef T* iterator;
  typedef T const* const_iterator;
#endif
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  typedef T value_type;
  typedef value_type* pointer;
  typedef u64 size_type;

  MatrixView() :mStride(0) {}

  MatrixView(const MatrixView& av) :
    mView(av.mView),
    mStride(av.mStride) {}

  MatrixView(pointer data, size_type numRows, size_type stride) :
    mView(data, numRows * stride),
    mStride(stride) {}

  MatrixView(pointer start, pointer end, size_type stride) :
    mView(start, end - ((end - start) % stride)),
    mStride(stride) {}

  template <class Iter>
  MatrixView(Iter start, Iter end, size_type stride,
    typename Iter::iterator_category * = 0) :
    mView(start, end/* - ((end - start) % stride)*/),
    mStride(stride) {}

  template<template<typename, typename...> class C, typename... Args>
  MatrixView(const C<T, Args...>& cont, size_type stride,
    typename C<T, Args...>::value_type*  = 0) :
    MatrixView(cont.begin(), cont.end(), stride) {}

  const MatrixView<T>& operator=(const MatrixView<T>& copy) {
    mView = copy.mView;
    mStride = copy.mStride;
    return copy;
  }

  void reshape(size_type rows, size_type columns) {
    if (rows * columns != size())
      throw std::runtime_error(LOCATION);

    mView = span<T>(mView.data(), rows * columns);
    mStride = columns;
  }

  const size_type size() const { return mView.size(); }
  const size_type stride() const { return mStride; }

  // returns the number of rows followed by the stride.
  std::array<size_type, 2> bounds() const { return { rows(), stride() }; }

  u64 rows() const {
    return stride() ? size() / stride() : 0;
  }

  u64 cols() const { return stride(); }

  pointer data() const { return mView.data(); }

  pointer data(u64 rowIdx) const {
#ifndef NDEBUG
    if (rowIdx >= rows()) throw std::runtime_error(LOCATION);
#endif
    return mView.data() + rowIdx * stride();
  }

  iterator begin() const { return mView.begin(); }
  iterator end() const { return mView.end(); }

  T& operator()(size_type idx) {
    return mView[idx];
  }

  const T& operator()(size_type idx) const {
    return mView[idx];
  }

  T& operator()(size_type rowIdx, size_type colIdx) {
    return mView[rowIdx * stride() + colIdx];
  }

  const T& operator()(size_type rowIdx, size_type colIdx) const {
    return mView[rowIdx * stride() + colIdx];
  }

  const span<T> operator[](size_type rowIdx) const {
#ifndef NDEBUG
    if (rowIdx >= rows()) throw std::runtime_error(LOCATION);
#endif

    return span<T>(mView.data() + rowIdx * stride(), stride());
  }

  template<typename TT = T>
  typename std::enable_if<std::is_pod<TT>::value>::type setZero() {
    static_assert(std::is_same<TT, T>::value, "");

    if (mView.size())
      memset(mView.data(), 0, mView.size() * sizeof(T));
  }

 protected:
  span<T> mView;
  size_type mStride = 0;
};

}  // namespace primihub

#endif  // SRC_primihub_COMMON_TYPE_MATRIX_VIEW_H_
