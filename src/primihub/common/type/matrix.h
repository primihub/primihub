// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_COMMON_TYPE_MATRIX_H_
#define SRC_primihub_COMMON_TYPE_MATRIX_H_

#include "src/primihub/common/defines.h"
#ifndef ENABLE_FULL_GSL
#include "src/primihub/common/gsl/gls-lite.hpp"
#endif
#include "src/primihub/common/type/matrix_view.h"
#include <cstring>
#include <type_traits>

namespace primihub {

enum class AllocType {
  Uninitialized,
  Zeroed
};

template<typename T>
class Matrix : public MatrixView<T> {
  u64 mCapacity = 0;
  using Storage = u8[sizeof(T)];

 public:
  Matrix() = default;

  Matrix(u64 rows, u64 columns, AllocType t = AllocType::Zeroed) {
    mCapacity = rows * columns;
    Storage* ptr;
    if (t == AllocType::Zeroed) {
      if (std::is_trivially_constructible<T>::value)
        ptr = new Storage[mCapacity]();
      else {
            ptr = new Storage[mCapacity];
            auto iter = (T*)ptr;
            for (u64 i = 0; i < mCapacity; ++i) {
              new (iter++)T();
            }
      }
    } else {
      ptr = new Storage[mCapacity];
    }

    *((MatrixView<T>*)this) = MatrixView<T>((T*)ptr, rows, columns);
  }

  Matrix(const Matrix<T>& copy)
      : Matrix(static_cast<const MatrixView<T>&>(copy))
  { }


  Matrix(const MatrixView<T>& copy) : mCapacity(copy.size()) {
    static_assert(std::is_copy_constructible<T>::value, "T must by copy");

    auto ptr = new Storage[copy.size()];
    *((MatrixView<T>*)this) = MatrixView<T>((T*)ptr, copy.rows(), copy.cols());

    if (std::is_trivially_copyable<T>::value) {
      memcpy(MatrixView<T>::data(), copy.data(), copy.size() * sizeof(T));
    } else {
      auto iter = MatrixView<T>::mView.data();
      for (u64 i = 0; i < copy.size(); ++i) {
        new ((char*)iter++)T(copy(i));
      }
    }
  }

  Matrix(Matrix<T>&& copy)
    : MatrixView<T>(copy.data(), copy.bounds()[0], copy.stride())
    , mCapacity(copy.mCapacity) {
    copy.mView = span<T>();
    copy.mStride = 0;
    copy.mCapacity = 0;
  }

  ~Matrix() {
    if (std::is_trivially_destructible<T>::value == false) {
      for (u64 i = 0; i < MatrixView<T>::size(); ++i)
        (MatrixView<T>::data() + i)->~T();
    }

    delete[](Storage*)(MatrixView<T>::data());
  }

  const Matrix<T>& operator=(const Matrix<T>& copy) {
    resize(copy.rows(), copy.stride());
    auto b = copy.begin();
    auto e = copy.end();

    std::copy(b, e, MatrixView<T>::mView.begin());
    return copy;
  }

  void resize(u64 rows, u64 columns, AllocType type = AllocType::Zeroed) {
      if (rows * columns > mCapacity)
      {
          auto old = MatrixView<T>::mView;
          mCapacity = rows * columns;

          if (std::is_trivially_constructible<T>::value)
          {
              if (type == AllocType::Zeroed)
                  MatrixView<T>::mView = span<T>((T*)new Storage[mCapacity](), mCapacity);
              else
                  MatrixView<T>::mView = span<T>((T*)new Storage[mCapacity], mCapacity);

              std::copy(old.begin(), old.end(), MatrixView<T>::mView.begin());
          }
          else
          {
              MatrixView<T>::mView = span<T>((T*)new Storage[mCapacity], mCapacity);

              auto iter = MatrixView<T>::mView.data();
              for (u64 i = 0; i < old.size(); ++i)
              {
                  new (iter++) T(std::move(old[i]));
              }

              if (type == AllocType::Zeroed)
              {
                  for (u64 i = old.size(); i < mCapacity; ++i)
                  {
                      new (iter++) T;
                  }
              }
          }


          if (std::is_trivially_destructible<T>::value == false)
          {
              for (u64 i = 0; i < old.size(); ++i)
                  (old.data() + i)->~T();
          }

          delete[](Storage*)old.data();
      }
      else
      {
          auto newSize = rows * columns;
          if (MatrixView<T>::size() &&
              newSize > MatrixView<T>::size() &&
              type == AllocType::Zeroed)
          {
              auto b = MatrixView<T>::data() + MatrixView<T>::size();
              auto e = b + newSize - MatrixView<T>::size();

              if (std::is_trivially_constructible<T>::value)
              {
                  std::memset(b, 0, (e - b) * sizeof(T));
              }
              else
              {
                  for (auto i = b; i < e; ++i)
                      new(i)T;
              }
          }

          MatrixView<T>::mView = span<T>(MatrixView<T>::data(), newSize);
      }

      MatrixView<T>::mStride = columns;
  }


  // return the internal memory, stop managing its lifetime, and set the current container to null.
  T* release()
  {
      auto ret = MatrixView<T>::mView.data();
      MatrixView<T>::mView = span<T>(nullptr, 0);
      mCapacity = 0;
      return ret;
  }


  bool operator==(const Matrix<T>& m) const
  {
      if (m.rows() != MatrixView<T>::rows() || m.cols() != MatrixView<T>::cols())
          return false;
      auto b0 = m.begin();
      auto e0 = m.end();
      auto b1 = MatrixView<T>::begin();
      return std::equal(b0, e0, b1);
  }
};


}

#endif  // SRC_primihub_COMMON_TYPE_MATRIX_H_
