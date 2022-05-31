// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_COMMON_TYPE_BIT_ITERATOR_H_
#define SRC_primihub_COMMON_TYPE_BIT_ITERATOR_H_

#include "src/primihub/common/defines.h"

namespace primihub {

// A class to reference a specific bit.
class BitReference {
 public:
  // Default copy constructor
  BitReference(const BitReference& rhs) = default;

  // Construct a reference to the bit in the provided byte offset by the shift.
  BitReference(u8* byte, u64 shift)
      :mByte(byte + (shift / 8)), mMask(1 << (shift%8)), mShift(shift % 8) {}

  // Construct a reference to the bit in the provided byte offset by the shift and mask.
  // Shift should be less than 8. and the mask should equal 1 << shift.
  BitReference(u8* byte, u8 mask, u8 shift)
      :mByte(byte), mMask(mask), mShift(shift) {}

  // Copy the underlying values of the rhs to the lhs.
  void operator=(const BitReference& rhs) { *this = (u8)rhs; }

  // Copy the value of the rhs to the lhs.
  inline void operator=(u8 n) {
      if (n > 0)  *mByte |= mMask;
      else        *mByte &= ~mMask;
  }

  inline void operator^=(bool b)
  {
      *mByte ^= ((b & 1) << mShift);
  }

  // Convert the reference to the underlying value
  operator u8() const {
      return (*mByte & mMask) >> mShift;
  }

 private:
  u8* mByte;
  u8 mMask, mShift;
  };

	// Function to allow the printing of a BitReference.
  std::ostream& operator<<(std::ostream& out, const BitReference& bit);

	// A class to allow the iteration of bits.
  class BitIterator {
   public:
    BitIterator() = default;
		// Default copy constructor
		BitIterator(const BitIterator& cp) = default;

		// Construct a reference to the bit in the provided byte offset by the shift.
		// Shift should be less than 8.
    BitIterator(u8* byte, u64 shift = 0)
      :mByte(byte + (shift / 8)), mMask(1 << (shift & 7)), mShift(shift & 7) {}

		// Construct a reference to the current bit pointed to by the iterator.
    BitReference operator*() { return BitReference(mByte, mMask, mShift); }

		// Pre increment the iterator by 1.
    BitIterator& operator++() {
      mByte += (mShift == 7) & 1;
      ++mShift &= 7;
      mMask = 1 << mShift;
      return *this;
    }

		// Post increment the iterator by 1. Returns a copy of this class.
    BitIterator operator++(int) {
      BitIterator ret(*this);

      mByte += (mShift == 7) & 1;
      ++mShift &= 7;
      mMask = 1 << mShift;

      return ret;
    }

    // Pre increment the iterator by 1.
    BitIterator& operator--() {
      mByte -= (mShift == 0) & 1;
      --mShift &= 7;
      mMask = 1 << mShift;
      return *this;
    }

    // Pre increment the iterator by 1.
    BitIterator operator--(int) {
      auto ret = *this;
      --(*this);
      return ret;
    }

		// Return the Iterator that has been incremented by v.
		// v must be possitive.
    BitIterator operator+(i64 v) const {
      Expects(v >= 0);

      BitIterator ret(mByte, mShift + v);

      return ret;
    }

		// Check if two iterators point to the same bit.
    bool operator==(const BitIterator& cmp) {
      return mByte == cmp.mByte && mShift == cmp.mShift;
    }

		u8* mByte;
		u8 mMask, mShift;
  };
}

#endif  // SRC_primihub_COMMON_TYPE_BIT_ITERATOR_H_
