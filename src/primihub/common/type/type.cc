// Copyright [2021] <primihub.com>

#include "src/primihub/common/type/type.h"
#include <string>
namespace primihub {

void columnDtypeFromInteger(int val, ColumnDtype &type) {
  switch (val) {
    case 0:
      type = ColumnDtype::STRING;
      break;
    case 1:
      type = ColumnDtype::INTEGER;
      break;
    case 2:
      type = ColumnDtype::DOUBLE;
      break;
    case 3:
      type = ColumnDtype::LONG;
      break;
    case 4:
      type = ColumnDtype::ENUM;
      break;
    case 5:
      type = ColumnDtype::BOOLEAN;
      break;
    default:
      type = ColumnDtype::UNKNOWN;
      break;
  }
}

std::string columnDtypeToString(const ColumnDtype &type) {
  std::string type_str;
  switch (type) {
    case ColumnDtype::BOOLEAN:
      type_str = "BOOLEAN";
      break;
    case ColumnDtype::DOUBLE:
      type_str = "FP64";
      break;
    case ColumnDtype::INTEGER:
      type_str = "INT64";
      break;
    case ColumnDtype::LONG:
      type_str = "INT64";
      break;
    case ColumnDtype::ENUM:
      type_str = "ENUM";
      break;
    case ColumnDtype::STRING:
      type_str = "STRING";
      break;
    default:
      type_str = "UNKNOWN TYPE";
      break;
  }

  return type_str;
}

template<typename T>
const T& Ref<T>::operator=(const T & copy) {
  mData[0] = reinterpret_cast<i64*>(&copy.mData[0]);
  mData[1] = reinterpret_cast<i64*>(&copy.mData[1]);
  return copy;
}

si64& si64::operator=(const si64& copy) {
  mData[0] = copy.mData[0];
  mData[1] = copy.mData[1];
  return *this;
}

si64 si64::operator+(const si64& rhs) const {
  si64 ret;
  ret.mData[0] = mData[0] + rhs.mData[0];
  ret.mData[1] = mData[1] + rhs.mData[1];
  return ret;
}

si64 si64::operator-(const si64& rhs) const {
  si64 ret;
  ret.mData[0] = mData[0] - rhs.mData[0];
  ret.mData[1] = mData[1] - rhs.mData[1];
  return ret;
}

Ref<si64> si64Matrix::operator()(u64 x, u64 y) const {
  return Ref<si64>(
  (i64&)mShares[0](x, y),
  (i64&)mShares[1](x, y));
}

Ref<si64> si64Matrix::operator()(u64 xy) const {
  return Ref<si64>(
  (i64&)mShares[0](xy),
  (i64&)mShares[1](xy));
}

si64Matrix si64Matrix::operator+(const si64Matrix& B) const {
  si64Matrix ret;
  ret.mShares[0] = mShares[0] + B.mShares[0];
  ret.mShares[1] = mShares[1] + B.mShares[1];
  return ret;
}

si64Matrix si64Matrix::operator-(const si64Matrix& B) const {
  si64Matrix ret;
  ret.mShares[0] = mShares[0] - B.mShares[0];
  ret.mShares[1] = mShares[1] - B.mShares[1];
  return ret;
}

si64Matrix si64Matrix::transpose() const {
  si64Matrix ret;
  ret.mShares[0] = mShares[0].transpose();
  ret.mShares[1] = mShares[1].transpose();
  return ret;
}

void si64Matrix::transposeInPlace() {
  mShares[0].transposeInPlace();
  mShares[1].transposeInPlace();
}

si64Matrix::Row si64Matrix::row(u64 i) {
  return Row{ *this, i };
}

si64Matrix::ConstRow si64Matrix::row(u64 i) const {
  return ConstRow{ *this, i };
}

si64Matrix::Col si64Matrix::col(u64 i) {
  return Col{ *this, i };
}

si64Matrix::ConstCol si64Matrix::col(u64 i) const {
  return ConstCol{ *this, i };
}

const si64Matrix::Row & si64Matrix::Row::operator=(const Row & row) {
  mMtx.mShares[0].row(mIdx) = row.mMtx.mShares[0].row(row.mIdx);
  mMtx.mShares[1].row(mIdx) = row.mMtx.mShares[1].row(row.mIdx);

  return row;
}

const si64Matrix::ConstRow & si64Matrix::Row::operator=(
  const ConstRow & row) {
  mMtx.mShares[0].row(mIdx) = row.mMtx.mShares[0].row(row.mIdx);
  mMtx.mShares[1].row(mIdx) = row.mMtx.mShares[1].row(row.mIdx);
  return row;
}

const si64Matrix::Col & si64Matrix::Col::operator=(const Col & col) {
  mMtx.mShares[0].col(mIdx) = col.mMtx.mShares[0].col(col.mIdx);
  mMtx.mShares[1].col(mIdx) = col.mMtx.mShares[1].col(col.mIdx);
  return col;
}

const si64Matrix::ConstCol & si64Matrix::Col::operator=(
  const ConstCol & col) {
  mMtx.mShares[0].col(mIdx) = col.mMtx.mShares[0].col(col.mIdx);
  mMtx.mShares[1].col(mIdx) = col.mMtx.mShares[1].col(col.mIdx);
  return col;
}

}  // namespace primihub
