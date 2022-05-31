// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_BLOCK_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_BLOCK_H_

#include <cstdint>
#include <array>
#include <iostream>

#include <iomanip>
#include <sstream>
#include <vector>
#include <random>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/crypto/bit_iterator.h"

#ifdef OC_ENABLE_SSE2
#include <emmintrin.h>
#include <smmintrin.h>
#endif
#ifdef OC_ENABLE_PCLMUL
#include <wmmintrin.h>
#endif

namespace primihub {

// 16位对其
struct alignas(16) block {
#ifdef OC_ENABLE_SSE2
  __m128i mData;
#else
  std::uint64_t mData[2];
#endif

  block() = default;
  block(const block&) = default;
  block(uint64_t x1, uint64_t x0) {
#ifdef OC_ENABLE_SSE2
    // 向量里面每个数都是整形
    mData = _mm_set_epi64x(x1, x0);
#else
    as<uint64_t>()[0] = x0;
    as<uint64_t>()[1] = x1;
#endif
  }

  block(char e15, char e14, char e13, char e12, char e11,
    char e10, char e9, char e8, char e7, char e6,
    char e5, char e4, char e3, char e2, char e1, char e0) {
#ifdef OC_ENABLE_SSE2
    mData = _mm_set_epi8(e15, e14, e13, e12, e11, e10, e9, e8,
      e7, e6, e5, e4, e3, e2, e1, e0);
#else
    as<char>()[0] = e0;
    as<char>()[1] = e1;
    as<char>()[2] = e2;
    as<char>()[3] = e3;
    as<char>()[4] = e4;
    as<char>()[5] = e5;
    as<char>()[6] = e6;
    as<char>()[7] = e7;
    as<char>()[8] = e8;
    as<char>()[9] = e9;
    as<char>()[10] = e10;
    as<char>()[11] = e11;
    as<char>()[12] = e12;
    as<char>()[13] = e13;
    as<char>()[14] = e14;
    as<char>()[15] = e15;
#endif
  }

#ifdef OC_ENABLE_SSE2
  block(const __m128i& x) {
    mData = x;
  }

  operator const __m128i& () const {
    return mData;
  }

  operator __m128i& () {
    return mData;
  }

  __m128i& m128i() {
    return mData;
  }

  const __m128i& m128i() const {
    return mData;
  }
#endif

  // 不超过16个字节的POD类型
  template<typename T>
  typename std::enable_if<std::is_pod<T>::value && (sizeof(T) <= 16) && (16 % sizeof(T) == 0), std::array<T, 16 / sizeof(T)>&>::type as() {
    return *(std::array<T, 16 / sizeof(T)>*)this;
  }

  template<typename T>
  typename std::enable_if<std::is_pod<T>::value && (sizeof(T) <= 16) && (16 % sizeof(T) == 0), const std::array<T, 16 / sizeof(T)>&>::type as() const {
      return *(const std::array<T, 16 / sizeof(T)>*)this;
  }

  inline bool operator<(const primihub::block& rhs) {
    auto& x = as<std::uint64_t>();
    auto& y = rhs.as<std::uint64_t>();
    return x[1] < y[1] || (x[1] == y[1] && x[0] < y[0]);
  }

  inline primihub::block operator^(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_xor_si128(rhs);
#else
    return cc_xor_si128(rhs);
#endif
  }
#ifdef OC_ENABLE_SSE2

  inline primihub::block mm_xor_si128(const primihub::block& rhs) const {
    return _mm_xor_si128(*this, rhs);
  }
#endif

  inline primihub::block cc_xor_si128(const primihub::block& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] ^= rhs.as<std::uint64_t>()[0];
    ret.as<std::uint64_t>()[1] ^= rhs.as<std::uint64_t>()[1];
    return ret;
  }

  inline primihub::block operator&(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_and_si128(rhs);
#else
    return cc_and_si128(rhs);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline primihub::block mm_and_si128(const primihub::block& rhs) const {
    return _mm_and_si128(*this, rhs);
  }
#endif
  inline primihub::block cc_and_si128(const primihub::block& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] &= rhs.as<std::uint64_t>()[0];
    ret.as<std::uint64_t>()[1] &= rhs.as<std::uint64_t>()[1];
    return ret;
  }

  inline primihub::block operator|(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_or_si128(rhs);
#else
    return cc_or_si128(rhs);
#endif
  }
#ifdef OC_ENABLE_SSE2
  inline primihub::block mm_or_si128(const primihub::block& rhs) const {
    return _mm_or_si128(*this, rhs);
  }
#endif
  inline primihub::block cc_or_si128(const primihub::block& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] |= rhs.as<std::uint64_t>()[0];
    ret.as<std::uint64_t>()[1] |= rhs.as<std::uint64_t>()[1];
    return ret;
  }

  // 移位操作，左移
  inline primihub::block operator<<(const std::uint8_t& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_slli_epi64(rhs);
#else
    return cc_slli_epi64(rhs);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline primihub::block mm_slli_epi64(const std::uint8_t& rhs) const {
    return _mm_slli_epi64(*this, rhs);
  }
#endif
  inline primihub::block cc_slli_epi64(const std::uint8_t& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] <<= rhs;
    ret.as<std::uint64_t>()[1] <<= rhs;
    return ret;
  }

  // 移位操作，右移
  inline block operator>>(const std::uint8_t& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_srli_epi64(rhs);
#else
    return cc_srli_epi64(rhs);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline block mm_srli_epi64(const std::uint8_t& rhs) const {
    return _mm_srli_epi64(*this, rhs);
  }
#endif
  inline block cc_srli_epi64(const std::uint8_t& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] >>= rhs;
    ret.as<std::uint64_t>()[1] >>= rhs;
    return ret;
  }

  inline primihub::block operator+(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_add_epi64(rhs);
#else
    return cc_add_epi64(rhs);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline block mm_add_epi64(const primihub::block& rhs) const {
    return _mm_add_epi64(*this, rhs);
  }
#endif
  inline block cc_add_epi64(const primihub::block& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] += rhs.as<std::uint64_t>()[0];
    ret.as<std::uint64_t>()[1] += rhs.as<std::uint64_t>()[1];
    return ret;
  }

  inline primihub::block operator-(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    return mm_sub_epi64(rhs);
#else
    return cc_sub_epi64(rhs);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline block mm_sub_epi64(const primihub::block& rhs) const {
    return _mm_sub_epi64(*this, rhs);
  }
#endif
  inline block cc_sub_epi64(const primihub::block& rhs) const {
    auto ret = *this;
    ret.as<std::uint64_t>()[0] -= rhs.as<std::uint64_t>()[0];
    ret.as<std::uint64_t>()[1] -= rhs.as<std::uint64_t>()[1];
    return ret;
  }


  inline bool operator==(const primihub::block& rhs) const {
#ifdef OC_ENABLE_SSE2
    auto neq = _mm_xor_si128(*this, rhs);
    return _mm_test_all_zeros(neq, neq) != 0;
#else
    return as<std::uint64_t>()[0] == rhs.as<std::uint64_t>()[0] &&
      as<std::uint64_t>()[1] == rhs.as<std::uint64_t>()[1];
#endif
  }

  inline bool operator!=(const primihub::block& rhs) const {
    return !(*this == rhs);
  }

  inline bool operator<(const primihub::block& rhs) const {
    return as<std::uint64_t>()[1] < rhs.as< std::uint64_t>()[1] ||
      (as<std::uint64_t>()[1] == rhs.as< std::uint64_t>()[1] &&
      as<std::uint64_t>()[0] < rhs.as< std::uint64_t>()[0]);
  }

  inline block srai_epi16(int imm8) const {
#ifdef OC_ENABLE_SSE2
    return mm_srai_epi16(imm8);
#else
    return cc_srai_epi16(imm8);
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline block mm_srai_epi16(char imm8) const {
    return _mm_srai_epi16(*this, imm8);
  }
#endif
  inline block cc_srai_epi16(char imm8) const {
    block ret;
    auto& v = as<std::int16_t>();
    auto& r = ret.as<std::int16_t>();
    if (imm8 <= 15) {
      r[0] = v[0] >> imm8;
      r[1] = v[1] >> imm8;
      r[2] = v[2] >> imm8;
      r[3] = v[3] >> imm8;
      r[4] = v[4] >> imm8;
      r[5] = v[5] >> imm8;
      r[6] = v[6] >> imm8;
      r[7] = v[7] >> imm8;
    } else {
      r[0] = v[0] ? 0xFFFF : 0;
      r[1] = v[1] ? 0xFFFF : 0;
      r[2] = v[2] ? 0xFFFF : 0;
      r[3] = v[3] ? 0xFFFF : 0;
      r[4] = v[4] ? 0xFFFF : 0;
      r[5] = v[5] ? 0xFFFF : 0;
      r[6] = v[6] ? 0xFFFF : 0;
      r[7] = v[7] ? 0xFFFF : 0;
    }
    return ret;
  }

  inline int movemask_epi8() const {
#ifdef OC_ENABLE_SSE2
    return mm_movemask_epi8();
#else
    return cc_movemask_epi8();
#endif
  }

#ifdef OC_ENABLE_SSE2
  inline int mm_movemask_epi8() const {
    return _mm_movemask_epi8(*this);
  }
#endif

  inline int cc_movemask_epi8() const {
    int ret{ 0 };
    auto& v = as<unsigned char>();
    int j = 0;
    for (int i = 7; i >= 0; --i)
      ret |= std::uint16_t(v[j++] & 128) >> i;

    for (size_t i = 1; i <= 8; i++)
      ret |= std::uint16_t(v[j++] & 128) << i;

    return ret;
  }

  inline int testc(const block& b) const {
#ifdef OC_ENABLE_SSE2
    return mm_testc_si128(b);
#else
    return cc_testc_si128(b);
#endif
  }

  inline int cc_testc_si128(const block& b) const {
    auto v0 = ~as<uint64_t>()[0] & b.as<uint64_t>()[0];
    auto v1 = ~as<uint64_t>()[1] & b.as<uint64_t>()[1];
    return (v0 || v1) ? 0 : 1;
  }

#ifdef OC_ENABLE_SSE2
  inline int mm_testc_si128(const block& b) const {
    return _mm_testc_si128(*this, b);
  }
#endif

  inline void gf128Mul(const block& y, block& xy1, block& xy2) const {
#ifdef OC_ENABLE_PCLMUL
    mm_gf128Mul(y, xy1, xy2);
#else
    cc_gf128Mul(y, xy1, xy2);
#endif // !OC_ENABLE_PCLMUL
  }

  inline block gf128Mul(const block& y) const {
    block xy1, xy2;
#ifdef OC_ENABLE_PCLMUL
    mm_gf128Mul(y, xy1, xy2);
#else
    cc_gf128Mul(y, xy1, xy2);
#endif // !OC_ENABLE_PCLMUL
    return xy1.gf128Reduce(xy2);
  }

  inline block gf128Pow(std::uint64_t i) const {
    if (*this == block(0,0))
      return block(0, 0);

    block pow2 = *this;
    block s = block(0, 1);
    while (i) {
      if (i & 1) {
        //s = 1 * i_0 * x^{2^{1}} * ... * i_j x^{2^{j+1}}
        s = s.gf128Mul(pow2);
      }

      // pow2 = x^{2^{j+1}}
      pow2 = pow2.gf128Mul(pow2);
      i >>= 1;
    }
    return s;
  }


#ifdef OC_ENABLE_PCLMUL
  inline void mm_gf128Mul(const block& y, block& xy1, block& xy2) const {
    auto& x = *this;

    block t1 = _mm_clmulepi64_si128(x, y, (int)0x00);
    block t2 = _mm_clmulepi64_si128(x, y, 0x10);
    block t3 = _mm_clmulepi64_si128(x, y, 0x01);
    block t4 = _mm_clmulepi64_si128(x, y, 0x11);
    t2 = (t2 ^ t3);
    t3 = _mm_slli_si128(t2, 8);
    t2 = _mm_srli_si128(t2, 8);
    t1 = (t1 ^ t3);
    t4 = (t4 ^ t2);

    xy1 = t1;
    xy2 = t4;
  }
#endif
  inline void cc_gf128Mul(const block& y, block& xy1, block& xy2) const {
    static const constexpr std::uint64_t mod = 0b10000111;
    auto shifted = as<uint64_t>();
    auto& result0 = xy1.as<uint64_t>();
    auto& result1 = xy2.as<uint64_t>();

    result0[0] = 0;
    result0[1] = 0;
    result1[0] = 0;
    result1[1] = 0;

    for (int64_t i = 0; i < 2; ++i) {
      for (int64_t j = 0; j < 64; ++j) {
        if (y.as<int64_t>()[i] & (1ull << j)) {
          result0[0] ^= shifted[0];
          result0[1] ^= shifted[1];
        }

        if (shifted[1] & (1ull << 63)) {
          shifted[1] = (shifted[1] << 1) | (shifted[0] >> 63);
          shifted[0] = (shifted[0] << 1) ^ mod;
        } else {
          shifted[1] = (shifted[1] << 1) | (shifted[0] >> 63);
          shifted[0] = shifted[0] << 1;
        }
      }
    }
  }

  block gf128Reduce(const block& x1) const {
#ifdef OC_ENABLE_PCLMUL
    return mm_gf128Reduce(x1);
#else
    return cc_gf128Reduce(x1);
#endif
  }

  block cc_gf128Reduce(const block& x1) const;

#ifdef OC_ENABLE_PCLMUL
  block mm_gf128Reduce(const block& x1) const {
    auto mul256_low = *this;
    auto mul256_high = x1;
    static const constexpr std::uint64_t mod = 0b10000111;

    /* reduce w.r.t. high half of mul256_high */
    const __m128i modulus = _mm_loadl_epi64((const __m128i*) & (mod));
    __m128i tmp = _mm_clmulepi64_si128(mul256_high, modulus, 0x01);
    mul256_low = _mm_xor_si128(mul256_low, _mm_slli_si128(tmp, 8));
    mul256_high = _mm_xor_si128(mul256_high, _mm_srli_si128(tmp, 8));

    /* reduce w.r.t. low half of mul256_high */
    tmp = _mm_clmulepi64_si128(mul256_high, modulus, 0x00);
    mul256_low = _mm_xor_si128(mul256_low, tmp);

    //std::cout << "redu " << bits(x, 128) << std::endl;
    //std::cout << "     " << bits(mul256_low, 128) << std::endl;

    return mul256_low;
  }
#endif
};

static_assert(sizeof(block) == 16, "expected block size");
static_assert(std::alignment_of<block>::value == 16, "expected block alignment");
static_assert(std::is_trivial<block>::value, "expected block trivial");
static_assert(std::is_standard_layout<block>::value, "expected block pod");
//#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
    //static_assert(std::is_pod<block>::value, "expected block pod");

inline block toBlock(std::uint64_t high_u64, std::uint64_t low_u64) {
  block ret;
  ret.as<std::uint64_t>()[0] = low_u64;
  ret.as<std::uint64_t>()[1] = high_u64;
  return ret;
}

inline block toBlock(std::uint64_t low_u64) {
  return toBlock(0, low_u64);
}

inline block toBlock(const std::uint8_t* data) {
  return toBlock(((std::uint64_t*)data)[1], ((std::uint64_t*)data)[0]);
}

extern const block ZeroBlock;
extern const block OneBlock;
extern const block AllOneBlock;
extern const block CCBlock;
extern const std::array<block, 2> zeroAndAllOne;

block sysRandomSeed();

using sPackedBin128 = sPackedBinBase<block>;

std::ostream& operator<<(std::ostream& out, const primihub::block& block);

inline bool eq(const primihub::block& lhs, const primihub::block& rhs) {
  return lhs == rhs;
}

inline bool neq(const primihub::block& lhs, const primihub::block& rhs) {
  return lhs != rhs;
}

}

namespace std {

template <>
struct hash<primihub::block> {
  std::size_t operator()(const primihub::block& k) const;
};

}

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_BLOCK_H_
