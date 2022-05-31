// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_COMMON_TYPE_BIT_VECTOR_H_
#define SRC_primihub_COMMON_TYPE_BIT_VECTOR_H_

#include <sstream>
#include <cstring>
#include <iomanip>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/bit_iterator.h"
#include "src/primihub/util/crypto/prng.h"

namespace primihub {

// A class to access a vector of packed bits. Similar to std::vector<bool>.
class BitVector {
 public:
  typedef u8 value_type;
  typedef value_type* pointer;
  typedef u64 size_type;

  // Default constructor.
  BitVector() = default;

  // Inititialize the BitVector with length bits pointed to by data.
  BitVector(u8* data, u64 length);

  // Inititialize the BitVector from a string of '0' and '1' characters.
  BitVector(std::string data);

  // Construct a zero initialized BitVector of size n.
  explicit BitVector(u64 n) { reset(n); }

  // Copy an existing BitVector.
  BitVector(const BitVector& K) { assign(K); }

  // Move an existing BitVector. Moved from is set to size zero.
  BitVector(BitVector&& rref);

  ~BitVector() { delete[] mData; }

  // Reset the BitVector to have value b.
  void assign(const block& b);

  // Copy an existing BitVector
  void assign(const BitVector& K); 

  // Append length bits pointed to by data starting a the bit index by offset.
  void append(u8* data, u64 length, u64 offset = 0);

  // Append an existing BitVector to this BitVector.
  void append(const BitVector& k) { append(k.data(), k.size()); }

  // Append length bits pointed to by data starting a the bit index by offset.
  void append(const BitVector& k, u64 length, u64 offset = 0);

  // erases original contents and set the new size, default 0.
  void reset(size_t new_nbits = 0);

  // Resize the BitVector to have the desired number of bits.
  void resize(u64 newSize);

  // Resize the BitVector to have the desired number of bits.
  // Fill each new bit with val.
  void resize(u64 newSize, u8 val);

  // Reserve enough space for the specified number of bits.
  void reserve(u64 bits);

  // Copy length bits from src starting at offset idx.
  void copy(const BitVector& src, u64 idx, u64 length);

  // Returns the number of bits this BitVector can contain using the current allocation.
  u64 capacity() const { return mAllocBytes * 8; }

  // Returns the number of bits this BitVector current has.
  u64 size() const { return mNumBits; }

  // Return the number of bytes the BitVector currently utilize.
  u64 sizeBytes() const { return (mNumBits + 7) / 8; }

  // Returns a byte pointer to the underlying storage.
  u8* data() const { return mData; }

  // Copy and existing BitVector.
  BitVector& operator=(const BitVector& K);

  // Get a reference to a specific bit.
  BitReference operator[](const u64 idx) const;

  // Xor two BitVectors together and return the result. Must have the same size. 
  BitVector operator^(const BitVector& B)const;

  // AND two BitVectors together and return the result. Must have the same size. 
  BitVector operator&(const BitVector& B)const;

  // OR two BitVectors together and return the result. Must have the same size. 
  BitVector operator|(const BitVector& B)const;

  // Invert the bits of the BitVector and return the result. 
  BitVector operator~()const;

  // Xor the rhs into this BitVector
  void operator^=(const BitVector& A);

  // And the rhs into this BitVector
  void operator&=(const BitVector& A);

  // Or the rhs into this BitVector
  void operator|=(const BitVector& A);

  // Check for equality between two BitVectors
  bool operator==(const BitVector& k) { return equals(k); }

  // Check for inequality between two BitVectors
  bool operator!=(const BitVector& k)const { return !equals(k); }

  // Check for equality between two BitVectors
  bool equals(const BitVector& K) const;
         
  // Initialize this BitVector from a string of '0' and '1' characters.
  void fromString(std::string data);

  // Returns an Iterator for the first bit.
  BitIterator begin() const;

  // Returns an Iterator for the position past the last bit.
  BitIterator end() const;

  // Initialize this bit vector to size n with a random set of k bits set to 1.
  void nChoosek(u64 n, u64 k, PRNG& prng);

  // Return the hamming weight of the BitVector.
  u64 hammingWeight() const;

  // Append the bit to the end of the BitVector.
  void pushBack(u8 bit);

  // Returns a refernce to the last bit.
  inline BitReference back() { return (*this)[size() - 1]; }

  // Set all the bits to random values.
  void randomize(PRNG& G); 

  // Return the parity of the vector.
  u8 parity();  

  // Return the hex representation of the vector.
  std::string hex()const;

  // Reinterpret the vector of bits as a vector of type T.
  template<class T>
  span<T> getArrayView() const;

  // Reinterpret the vector of bits as a vector of type T.
  template<class T>
  span<T> getSpan() const;

 private:
  u8* mData = nullptr;
  u64 mNumBits = 0, mAllocBytes = 0;
};

template<class T>
inline span<T> BitVector::getArrayView() const {
  return span<T>((T*)mData, (T*)mData + (sizeBytes() / sizeof(T)));
}

template<class T>
inline gsl::span<T> BitVector::getSpan() const {
  return gsl::span<T>((T*)mData, (T*)mData + (sizeBytes() / sizeof(T)));
}

std::ostream& operator<<(std::ostream& in, const BitVector& val);

#ifdef ENABLE_BOOST
    // template<>
    // inline u8* channelBuffData<BitVector>(const BitVector& container)
    // {
    //     return (u8*)container.data();
    // }

    // template<>
    // inline BitVector::size_type channelBuffSize<BitVector>(const BitVector& container)
    // {
    //     return container.sizeBytes();
    // }

    // template<>
    // inline bool channelBuffResize<BitVector>(BitVector& container, u64 size)
    // {
    //     return size == container.sizeBytes();
    // }
#endif

}

#endif  // SRC_primihub_COMMON_TYPE_BIT_VECTOR_H_
