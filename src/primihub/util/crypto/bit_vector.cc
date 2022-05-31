// Copyright [2021] <primihub.com>
#include "src/primihub/util/crypto/bit_vector.h"

namespace primihub {

BitVector::BitVector(std::string data)
  :
  mData(nullptr),
  mNumBits(0),
  mAllocBytes(0) {
  fromString(data);
}

BitVector::BitVector(BitVector&& rref)
  :
  mData(rref.mData),
  mNumBits(rref.mNumBits),
  mAllocBytes(rref.mAllocBytes) {
  rref.mData = nullptr;
  rref.mAllocBytes = 0;
  rref.mNumBits = 0;
}

BitVector::BitVector(u8 * data, u64 length)
  :
  mData(nullptr),
  mNumBits(0),
  mAllocBytes(0) {
  append(data, length, 0);
}

void BitVector::assign(const block& b) {
  reset(128);
  memcpy(mData, (u8*)&(b), sizeBytes());
}

void BitVector::assign(const BitVector& K) {
  reset(K.mNumBits);
  memcpy(mData, K.mData, sizeBytes());
}

void BitVector::append(u8* data, u64 length, u64 offset) {
  auto bitIdx = mNumBits;
  auto destOffset = mNumBits % 8;
  auto destIdx = mNumBits / 8;
  auto srcOffset = offset % 8;
  auto srcIdx = offset / 8;
  auto byteLength = (length + 7) / 8;

  resize(mNumBits + length);

  static const u8 masks[8] = { 1,2,4,8,16,32,64,128 };

  // if we have to do bit shifting, copy bit by bit
  if (srcOffset || destOffset) {
    //TODO("make this more efficient");
    for (u64 i = 0; i < length; ++i, ++bitIdx, ++offset) {
      u8 bit = data[offset / 8] & masks[offset % 8];
      (*this)[bitIdx] = bit;
    }
  } else {
    memcpy(mData + destIdx, data + srcIdx, byteLength);
  }
}

void BitVector::append(const BitVector& k, u64 length, u64 offset) {
  if (k.size() < length + offset)
    throw std::runtime_error("length too long. " LOCATION);

  append(k.data(), length, offset);
}

void BitVector::reserve(u64 bits) {
  u64 curBits = mNumBits;
  resize(bits);

  mNumBits = curBits;
}

void BitVector::resize(u64 newSize) {
  u64 new_nbytes = (newSize + 7) / 8;

  if (mAllocBytes < new_nbytes) {
    u8* tmp = new u8[new_nbytes]();
    mAllocBytes = new_nbytes;

    memcpy(tmp, mData, sizeBytes());

    if (mData)
      delete[] mData;

    mData = tmp;
  }
  mNumBits = newSize;
}

void BitVector::resize(u64 newSize, u8 val) {
  val = bool(val) * ~0;

  auto oldSize = size();
  resize(newSize);

  u64 offset = oldSize & 7;
  u64 idx = oldSize / 8;

  if (offset) {
    u8 mask = (~0) << offset;
    mData[idx] = (mData[idx] & ~mask) | (val & mask);
    ++idx;
  }

  u64 rem = sizeBytes() - idx;
  if(rem)
    memset(mData + idx, val, rem);
}

void BitVector::reset(size_t new_nbits) {
  u64 newSize = (new_nbits + 7) / 8;

  if (newSize > mAllocBytes) {
    // if (mData)
      // delete[] mData;

    mData = new u8[newSize]();
    mAllocBytes = newSize;
  } else {
    memset(mData, 0, newSize);
  }

  mNumBits = new_nbits;
}

void BitVector::copy(const BitVector& src, u64 idx, u64 length) {
  resize(0);
  append(src.mData, length, idx);
}

BitVector& BitVector::operator=(const BitVector& K) {
  if (this != &K) {
    assign(K);
  }
  return *this;
}

BitReference BitVector::operator[](const u64 idx) const {
  if (idx >= mNumBits)
    throw std::runtime_error("rt error at " LOCATION);
  return BitReference(mData + (idx / 8), static_cast<u8>(idx % 8));
}

std::ostream& operator<<(std::ostream& out, const BitReference& bit) {
  out << (u32)bit;
  return out;
}

BitVector BitVector::operator^(const BitVector& B)const {
  BitVector ret(*this);

  ret ^= B;

  return ret;
}

BitVector BitVector::operator&(const BitVector & B) const {
  BitVector ret(*this);

  ret &= B;

  return ret;
}

BitVector BitVector::operator|(const BitVector & B) const {
  BitVector ret(*this);

  ret |= B;

  return ret;
}

BitVector BitVector::operator~() const {
  BitVector ret(*this);

  for (u64 i = 0; i < sizeBytes(); i++)
    ret.mData[i] = ~mData[i];

  return ret;
}

void BitVector::operator&=(const BitVector & A) {
  for (u64 i = 0; i < sizeBytes(); i++) {
    mData[i] &= A.mData[i];
  }
}

void BitVector::operator|=(const BitVector & A) {
  for (u64 i = 0; i < sizeBytes(); i++) {
    mData[i] |= A.mData[i];
  }
}

void BitVector::operator^=(const BitVector& A) {
  if (mNumBits != A.mNumBits)
    throw std::runtime_error("rt error at " LOCATION);
  for (u64 i = 0; i < sizeBytes(); i++) {
    mData[i] ^= A.mData[i];
  }
}

void BitVector::fromString(std::string data) {
  resize(data.size());

  for (u64 i = 0; i < size(); ++i) {
#ifndef NDEBUG
    if (u8(data[i] - '0') > 1)
      throw std::runtime_error("");
#endif
    (*this)[i] = data[i] - '0';
  }
}

bool BitVector::equals(const BitVector& rhs) const {
  if (mNumBits != rhs.mNumBits)
    return false;

  u64 lastByte = sizeBytes() - 1;
  for (u64 i = 0; i < lastByte; i++) {
    if (mData[i] != rhs.mData[i]) {
      return false;
    }
  }

  // numBits = 4
  // 00001010
  // 11111010
  //     ^^^^ compare these

  u64 rem = mNumBits & 7;
  u8 mask = ((u8)-1) >> (8 - rem);
  if ((mData[lastByte] & mask) != (rhs.mData[lastByte] & mask))
    return false;

  return true;
}

BitIterator BitVector::begin() const {
  return BitIterator(mData, 0);
}

BitIterator BitVector::end() const {
  return BitIterator(mData + (mNumBits >> 3), mNumBits & 7);
}

void BitVector::nChoosek(u64 n, u64 k, PRNG & prng) {
  reset(n);
  // wiki: Reservoir sampling

  memset(data(), u8(-1), k / 8);
  for (u64 i = k - 1; i >= (k & (~3)); --i)
    (*this)[i] = 1;

  for (u64 i = k; i < n; ++i) {
    u64 j = prng.get<u64>() % i;

    if (j < k) {
      u8 b = (*this)[j];
      (*this)[j] = 0;
      (*this)[i] = b;
    }
  }
}

u64 BitVector::hammingWeight() const {
  //TODO("make sure top bits are cleared");
  u64 ham(0);
  for (u64 i = 0; i < sizeBytes(); ++i) {
    u8 b = data()[i];
    while (b) {
      ++ham;
      b &= b - 1;
    }
  }
  return ham;
}

u8 BitVector::parity() {
  u8 bit = 0;

  u64 lastByte = mNumBits / 8;
  for (u64 i = 0; i < lastByte; i++) {
    bit ^= (mData[i] & 1); // bit 0
    bit ^= ((mData[i] >> 1) & 1); // bit 1
    bit ^= ((mData[i] >> 2) & 1); // bit 2
    bit ^= ((mData[i] >> 3) & 1); // bit 3
    bit ^= ((mData[i] >> 4) & 1); // bit 4
    bit ^= ((mData[i] >> 5) & 1); // bit 5
    bit ^= ((mData[i] >> 6) & 1); // bit 6
    bit ^= ((mData[i] >> 7) & 1); // bit 7
  }

  u64 lastBits = mNumBits - lastByte * 8;
  for (u64 i = 0; i < lastBits; i++) {
    bit ^= (mData[lastByte] >> i) & 1;
  }

  return bit;
}

void BitVector::pushBack(u8 bit) {
  if (size() == capacity()) {
    reserve(size() * 2);
  }

  resize(size() + 1);

  back() = bit;
}

void BitVector::randomize(PRNG& G) {
  G.get(mData, sizeBytes());
}

std::string BitVector::hex() const {
  std::stringstream s;

  s << std::hex;
  for (unsigned int i = 0; i < sizeBytes(); i++) {
    s << std::setw(2) << std::setfill('0') << int(mData[i]);
  }

  return s.str();
}

std::ostream & operator<<(std::ostream & out, const BitVector & vec) {
  //for (i64 i = static_cast<i64>(val.size()) - 1; i > -1; --i)
  //{
  //    in << (u32)val[i];
  //}

  //return in;
  for (u64 i = 0; i < vec.size(); ++i) {
    out << char('0' + (u8)vec[i]);
  }

  return out;
}

}
