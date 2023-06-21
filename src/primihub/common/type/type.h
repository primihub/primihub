// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_COMMON_TYPE_TYPE_H_
#define SRC_PRIMIHUB_COMMON_TYPE_TYPE_H_

#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <utility>
#include <string>

#include "Eigen/Dense"
#include "src/primihub/common/type/matrix.h"
#include "src/primihub/common/type/matrix_view.h"
#include "src/primihub/common/defines.h"

namespace primihub {

template<typename T>
using eMatrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using i64Matrix = eMatrix<i64>;

struct si64;

template<typename ShareType>
struct Ref {
  friend ShareType;
  using ref_value_type = typename ShareType::value_type;

  std::array<ref_value_type*, 2> mData;

  Ref(ref_value_type& a0, ref_value_type& a1) {
    mData[0] = &a0;
    mData[1] = &a1;
  }

  const ShareType& operator=(const ShareType& copy);
  ref_value_type& operator[](u64 i) { return *mData[i]; }
  const ref_value_type& operator[](u64 i) const { return *mData[i]; }
};

// a shared 64 bit integer
struct si64 {
  using value_type = i64;
  std::array<value_type, 2> mData;

  si64() = default;
  si64(const si64&) = default;
  si64(si64&&) = default;
  si64(const std::array<value_type, 2>& d) :mData(d) {}
  si64(const Ref<si64>& s) {
    mData[0] = *s.mData[0];
    mData[1] = *s.mData[1];
  }

  si64& operator=(const si64& copy);
  si64 operator+(const si64& rhs) const;
  si64 operator-(const si64& rhs) const;

  value_type& operator[](u64 i) { return mData[i]; }
  const value_type& operator[](u64 i) const { return mData[i]; }
};

// a shared 64 binary value
struct sb64 {
  using value_type = i64;
  std::array<i64, 2> mData;

  sb64() = default;
  sb64(const sb64&) = default;
  sb64(sb64&&) = default;
  sb64(const std::array<value_type, 2>& d) :mData(d) {}

  i64& operator[](u64 i) { return mData[i]; }
  const i64& operator[](u64 i) const { return mData[i]; }

  sb64& operator=(const sb64& copy) = default;
  sb64 operator^(const sb64& x) {
    return { { mData[0] ^ x.mData[0], mData[1] ^ x.mData[1] } };
  }
};

bool areEqualImpl(const std::array<MatrixView<u8>, 2>& a,
                  const std::array<MatrixView<u8>, 2>& b,
                  u64 bitCount);
void trimImpl(MatrixView<u8> a, u64 bits);

template<typename T>
void MatrixViewTrim(MatrixView<T> a, i64 bits) {
  static_assert(std::is_pod<T>::value, "");
  MatrixView<u8> aa(reinterpret_cast<u8*>(a.data()),
                        a.rows(), a.cols() * sizeof(T));
  trimImpl(aa, bits);
}

template<typename T>
bool MatrixViewAreEqual(const std::array<MatrixView<T>, 2>& a,
              const std::array<MatrixView<T>, 2>& b,
              u64 bitCount) {
  std::array<MatrixView<u8>, 2> aa;
  std::array<MatrixView<u8>, 2> bb;

  static_assert(std::is_pod<T>::value, "");
  aa[0] = MatrixView<u8>(
    reinterpret_cast<u8*>(a[0].data()), a[0].rows(), a[0].cols() * sizeof(T));
  aa[1] = MatrixView<u8>(
    reinterpret_cast<u8*>(a[1].data()), a[1].rows(), a[1].cols() * sizeof(T));
  bb[0] = MatrixView<u8>(
    reinterpret_cast<u8*>(b[0].data()), b[0].rows(), b[0].cols() * sizeof(T));
  bb[1] = MatrixView<u8>(
    reinterpret_cast<u8*>(b[1].data()), b[1].rows(), b[1].cols() * sizeof(T));

  return areEqualImpl(aa, bb, bitCount);
}

template<typename T>
bool MatrixAreEqual(const std::array<Matrix<T>, 2>& a,
              const std::array<Matrix<T>, 2>& b, u64 bitCount) {
  std::array<MatrixView<T>, 2> aa{ a[0], a[1] };
  std::array<MatrixView<T>, 2> bb{ b[0], b[1] };

  return MatrixViewAreEqual(aa, bb, bitCount);
}

struct si64Matrix {
  std::array<eMatrix<i64>, 2> mShares;

  struct ConstRow {
    const si64Matrix& mMtx;
    const u64 mIdx;
  };

  struct Row {
    si64Matrix& mMtx;
    const u64 mIdx;
    const Row& operator=(const Row& row);
    const ConstRow& operator=(const ConstRow& row);
  };

  struct ConstCol {
    const si64Matrix& mMtx;
    const u64 mIdx;
  };

  struct Col {
    si64Matrix& mMtx;
    const u64 mIdx;
    const Col& operator=(const Col& col);
    const ConstCol& operator=(const ConstCol& row);
  };

  si64Matrix() = default;
  si64Matrix(u64 xSize, u64 ySize) { resize(xSize, ySize); }

  void resize(u64 xSize, u64 ySize) {
    mShares[0].resize(xSize, ySize);
    mShares[1].resize(xSize, ySize);
  }

  u64 rows() const { return mShares[0].rows(); }
  u64 cols() const { return mShares[0].cols(); }
  u64 size() const { return mShares[0].size(); }

  Ref<si64> operator()(u64 x, u64 y) const;
  Ref<si64> operator()(u64 xy) const;
  si64Matrix operator+(const si64Matrix& B) const;
  si64Matrix operator-(const si64Matrix& B) const;

  si64Matrix transpose() const;
  void transposeInPlace();

  Row row(u64 i);
  Col col(u64 i);
  ConstRow row(u64 i) const;
  ConstCol col(u64 i) const;

  bool operator !=(const si64Matrix& b) const {
    return !(*this == b);
  }

  bool operator ==(const si64Matrix& b) const {
    return (rows() == b.rows() && cols() == b.cols() && mShares == b.mShares);
  }

  eMatrix<i64>& operator[](u64 i) { return mShares[i]; }

  const eMatrix<i64>& operator[](u64 i) const { return mShares[i]; }
};

struct sbMatrix {
  std::array<Matrix<i64>, 2> mShares;
  u64 mBitCount = 0;

  sbMatrix() = default;
  sbMatrix(u64 xSize, u64 ySize) { resize(xSize, ySize); }

  void resize(u64 xSize, u64 bitCount) {
    mBitCount = bitCount;
    auto ySize = (bitCount + 63) / 64;
    mShares[0].resize(xSize, ySize, AllocType::Uninitialized);
    mShares[1].resize(xSize, ySize, AllocType::Uninitialized);
  }

  u64 rows() const { return mShares[0].rows(); }
  u64 i64Size() const { return mShares[0].size(); }
  u64 i64Cols() const { return mShares[0].cols(); }
  u64 bitCount() const { return mBitCount; }

  bool operator !=(const sbMatrix& b) const {
      return !(*this == b);
  }

  bool operator ==(const sbMatrix& b) const {
    return (rows() == b.rows() && bitCount() == b.bitCount() &&
            MatrixAreEqual(mShares, b.mShares, bitCount()));
  }

  void trim() {
    for (auto i = 0ull; i < mShares.size(); ++i) {
      MatrixViewTrim(mShares[i], bitCount());
    }
  }
};

/*
  Represents a packed set of binary secrets. Data is stored in a tranposed
  format. The i'th bit of all the shares are packed together into the i'th row.
  This allows efficient SIMD operations. E.g. applying bit[0] = bit[1] ^ bit[2]
  to all the shares can be performed to 64 shares using one instruction.
  */
template<typename T = i64>
struct sPackedBinBase {
  static_assert(std::is_pod<T>::value, "must be pod");
  u64 mShareCount;

  std::array<MatrixView<T>, 2> mShares;
  std::unique_ptr<u8[]> mBacking;

  sPackedBinBase() = default;
  sPackedBinBase(u64 shareCount, u64 bitCount, u64 wordMultiple = 1) {
    reset(shareCount, bitCount, wordMultiple);
  }

  // resize without copy
  void reset(u64 shareCount, u64 bitCount, u64 wordMultiple = 1) {
    auto bitsPerWord = 8 * sizeof(T);
    auto wordCount = (shareCount + bitsPerWord - 1) / bitsPerWord;
    wordCount = roundUpTo(wordCount, wordMultiple);

    if (shareCount != mShareCount || wordCount != mShares[0].stride()) {
      mShareCount = shareCount;

      auto sizeT = bitCount * wordCount;
      // auto sizeBytes = (sizeT + 1) * sizeof(T);
      size_t sizeBytes = (sizeT + 1) * sizeof(T);
      // plus one to make sure we have enought space for aligned storage.
      auto totalT = 2 * sizeT + 1;

      mBacking.reset(new u8[totalT * sizeof(T)]);

      void* ptr = reinterpret_cast<void*>(mBacking.get());
      auto alignment = alignof(T);

      if (!std::align(alignment, sizeT * sizeof(T), ptr, sizeBytes))
          throw RTE_LOC;

      T* aligned0 = static_cast<T*>(ptr);
      T* aligned1 = aligned0 + sizeT;

      mShares[0] = MatrixView<T>(aligned0, bitCount, wordCount);
      mShares[1] = MatrixView<T>(aligned1, bitCount, wordCount);
    }
  }

  u64 size() const { return mShares[0].size(); }

  // the number of shares that are stored in this packed (shared) binary matrix.
  u64 shareCount() const { return mShareCount; }

  // the number of bits that each share has.
  u64 bitCount() const { return mShares[0].rows(); }

  // the number of i64s in each row = divCiel(mShareCount, 8 * sizeof(i64))
  u64 simdWidth() const { return mShares[0].cols(); }

  sPackedBinBase<T> operator^(const sPackedBinBase<T>& rhs) {
    if (shareCount() != rhs.shareCount() || bitCount() != rhs.bitCount())
      throw std::runtime_error(LOCATION);

    sPackedBinBase<T> r(shareCount(), bitCount());
    for (u64 i = 0; i < 2; ++i) {
      for (u64 j = 0; j < mShares[0].size(); ++j) {
        r.mShares[i](j) = mShares[i](j) ^ rhs.mShares[i](j);
      }
    }
    return r;
  }

  bool operator!=(const sPackedBinBase<T>& b) const {
    return !(*this == b);
  }

  bool operator==(const sPackedBinBase<T>& b) const {
    return (shareCount() == b.shareCount() && bitCount() == b.bitCount() &&
            MatrixViewAreEqual(mShares, b.mShares, shareCount()));
  }

  void trim() {
    for (auto i = 0; i < mShares.size(); ++i) {
      MatrixView<T>s(mShares[i].data(), mShares[i].rows(),
                        mShares[i].cols());
      MatrixViewTrim(s, shareCount());
    }
  }
};

using sPackedBin = sPackedBinBase<i64>;

template<typename T = i64>
struct PackedBinBase {
  static_assert(std::is_pod<T>::value, "must be pod");
  u64 mShareCount;

  Matrix<T> mData;

  PackedBinBase() = default;
  PackedBinBase(u64 shareCount, u64 bitCount, u64 wordMultiple = 1) {
    resize(shareCount, bitCount, wordMultiple);
  }

  void resize(u64 shareCount, u64 bitCount, u64 wordMultiple = 1) {
    mShareCount = shareCount;
    auto bitsPerWord = 8 * sizeof(T);
    auto wordCount = (shareCount + bitsPerWord - 1) / bitsPerWord;
    wordCount = roundUpTo(wordCount, wordMultiple);
    mData.resize(bitCount, wordCount, AllocType::Uninitialized);
  }

  u64 size() const { return mData.size(); }

  // the number of shares that are stored in this packed (shared) binary matrix.
  u64 shareCount() const { return mShareCount; }

  // the number of bits that each share has.
  u64 bitCount() const { return mData.rows(); }

  // the number of i64s in each row = divCiel(mShareCount, 8 * sizeof(i64))
  u64 simdWidth() const { return mData.cols(); }

  PackedBinBase<T> operator^(const PackedBinBase<T>& rhs) {
    if (shareCount() != rhs.shareCount() || bitCount() != rhs.bitCount())
      throw std::runtime_error(LOCATION);

    PackedBinBase<T> r(shareCount(), bitCount());
    for (u64 j = 0; j < mData.size(); ++j) {
      r.mData(j) = mData(j) ^ rhs.mData(j);
    }
    return r;
  }

  void trim() {
    MatrixViewTrim(mData, shareCount());
  }
};

template<typename T>
class SpscQueue {
 public:
  struct BaseQueue {
    BaseQueue() = delete;
    BaseQueue(const BaseQueue&) = delete;
    BaseQueue(BaseQueue&&) = delete;

    explicit BaseQueue(uint64_t cap) : mPopIdx(0), mPushIdx(0),
      mData(new uint8_t[cap * sizeof(T)]),
      mStorage((T*)mData.get(), cap) {}

    ~BaseQueue() {
      while (size())
        pop_front();
    }

    uint64_t mPopIdx, mPushIdx;
    std::unique_ptr<uint8_t[]> mData;
    span<T> mStorage;

    uint64_t capacity() const { return mStorage.size(); }
    uint64_t size() const { return mPushIdx - mPopIdx; }
    bool full() const { return size() == capacity(); }
    bool empty() const { return size() == 0; }

    void push_back(T&& v) {
      assert(size() < capacity());
      auto pushIdx = mPushIdx++ % capacity();
      void* data = &mStorage[pushIdx];
      new (data) T(std::forward<T>(v));
    }

    T& front() {
      assert(mPopIdx != mPushIdx);
      return *(T*)&mStorage[mPopIdx % capacity()];
    }

    void pop_front(T&out) {
      out = std::move(front());
      pop_front();
    }

    void pop_front() {
      front().~T();
      ++mPopIdx;
    }
  };

  // boost::circular_buffer_space_optimized<T> mQueue;
  std::list<BaseQueue> mQueues;
  mutable std::mutex mMtx;

  explicit SpscQueue(uint64_t cap = 64) {
    mQueues.emplace_back(cap);
  }

  bool isEmpty() const {
    std::lock_guard<std::mutex> l(mMtx);
    // return mQueue.empty();
    return mQueues.front().empty();
  }

  void push_back(T&& v) {
    std::lock_guard<std::mutex> l(mMtx);
    if (mQueues.back().full())
      mQueues.emplace_back(mQueues.back().size() * 4);
    mQueues.back().push_back(std::forward<T>(v));
  }

  T& front() {
    std::lock_guard<std::mutex> l(mMtx);
    return mQueues.front().front();
  }

  void pop_front(T& out) {
    std::lock_guard<std::mutex> l(mMtx);
    out = std::move(mQueues.front().front());
    mQueues.front().pop_front();
    if (mQueues.front().empty() && mQueues.size() > 1)
      mQueues.pop_front();
  }

  void pop_front() {
    T out;
    pop_front(out);
  }
};

enum ColumnDtype {
  STRING = 0,
  INTEGER = 1,
  DOUBLE = 2,
  LONG = 3,
  ENUM = 4,
  BOOLEAN = 5,
  UNKNOWN = 6,
};

std::string columnDtypeToString(const ColumnDtype &type);
void columnDtypeFromInteger(int val, ColumnDtype &type);
}  // namespace primihub

#endif  // SRC_PRIMIHUB_COMMON_TYPE_TYPE_H_
