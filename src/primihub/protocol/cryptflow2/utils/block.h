/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Enquiries about further applications and development opportunities are welcome.

Modified by Deevashwer Rathee
*/

#ifndef UTIL_BLOCK_H__
#define UTIL_BLOCK_H__
#include <algorithm>
#include <assert.h>
#include <bitset>
#include <emmintrin.h>
#include <immintrin.h>
#include <iostream>
#include <smmintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wmmintrin.h>
#include <xmmintrin.h>

namespace sci {
typedef __m128i block128;
typedef __m256i block256;

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC)
static inline void __attribute__((__always_inline__))
_mm256_storeu2_m128i(__m128i *const hiaddr, __m128i *const loaddr,
                     const __m256i a) {
  _mm_storeu_si128(loaddr, _mm256_castsi256_si128(a));
  _mm_storeu_si128(hiaddr, _mm256_extracti128_si256(a, 1));
}
#endif /* defined(__GNUC__) */

inline void print(const uint64_t &value, const char *end = "\n", int len = 64,
                  bool reverse = false) {
  std::string tmp = std::bitset<64>(value).to_string();
  if (reverse)
    std::reverse(tmp.begin(), tmp.end());
  if (reverse)
    std::cout << tmp.substr(0, len); // std::cout << std::hex << buffer[i];
  else
    std::cout << tmp.substr(64 - len,
                            len); // std::cout << std::hex << buffer[i];
  std::cout << end;
}

inline void print(const uint8_t &value, const char *end = "\n", int len = 8,
                  bool reverse = false) {
  std::string tmp = std::bitset<8>(value).to_string();
  if (reverse)
    std::reverse(tmp.begin(), tmp.end());
  if (reverse)
    std::cout << tmp.substr(0, len); // std::cout << std::hex << buffer[i];
  else
    std::cout << tmp.substr(8 - len, len); // std::cout << std::hex <<
                                           // buffer[i];
  std::cout << end;
}

inline void print(const block128 &value, const char *end = "\n") {
  const size_t n = sizeof(__m128i) / sizeof(uint64_t);
  uint64_t buffer[n];
  _mm_storeu_si128((__m128i *)buffer, value);
  // std::cout << "0x";
  for (size_t i = 0; i < n; i++) {
    std::string tmp = std::bitset<64>(buffer[i]).to_string();
    std::reverse(tmp.begin(), tmp.end());
    std::cout << tmp; // std::cout << std::hex << buffer[i];
  }
  std::cout << end;
}

inline void print(const block256 &value, const char *end = "\n") {
  const size_t n = sizeof(__m256i) / sizeof(uint64_t);
  uint64_t buffer[n];
  _mm256_storeu_si256((__m256i *)buffer, value);
  // std::cout << "0x";
  for (size_t i = 0; i < n; i++) {
    std::string tmp = std::bitset<64>(buffer[i]).to_string();
    std::reverse(tmp.begin(), tmp.end());
    std::cout << tmp; // std::cout << std::hex << buffer[i];
  }
  std::cout << end;
}

inline bool getLSB(const block128 &x) { return (*((char *)&x) & 1) == 1; }
__attribute__((target("sse2"))) inline block128 makeBlock128(int64_t x,
                                                             int64_t y) {
  return _mm_set_epi64x(x, y);
}

__attribute__((target("avx"))) inline block256
makeBlock256(int64_t w, int64_t x, int64_t y,
             int64_t z) { // return w||x||y||z (MSB->LSB)
  return _mm256_set_epi64x(w, x, y, z);
}
__attribute__((target("avx2,avx"))) inline block256
makeBlock256(block128 x, block128 y) { // return x (MSB) || y (LSB)
  return _mm256_inserti128_si256(_mm256_castsi128_si256(y), x, 1);
  // return _mm256_loadu2_m128i(&x, &y);
}
__attribute__((target("sse2"))) inline block128 zero_block() {
  return _mm_setzero_si128();
}
inline block128 one_block() {
  return makeBlock128(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
}

const block128 select_mask[2] = {zero_block(), one_block()};

__attribute__((target("sse2"))) inline block128 make_delta(const block128 &a) {
  return _mm_or_si128(makeBlock128(0L, 1L), a);
}

typedef __m128i block_tpl[2];

__attribute__((target("sse2"))) inline block128 xorBlocks(block128 x,
                                                          block128 y) {
  return _mm_xor_si128(x, y);
}
__attribute__((target("avx2"))) inline block256 xorBlocks(block256 x,
                                                          block256 y) {
  return _mm256_xor_si256(x, y);
}
__attribute__((target("sse2"))) inline block128 andBlocks(block128 x,
                                                          block128 y) {
  return _mm_and_si128(x, y);
}
__attribute__((target("avx2"))) inline block256 andBlocks(block256 x,
                                                          block256 y) {
  return _mm256_and_si256(x, y);
}

inline void xorBlocks_arr(block128 *res, const block128 *x, const block128 *y,
                          int nblocks) {
  const block128 *dest = nblocks + x;
  for (; x != dest;) {
    *(res++) = xorBlocks(*(x++), *(y++));
  }
}
inline void xorBlocks_arr(block128 *res, const block128 *x, block128 y,
                          int nblocks) {
  const block128 *dest = nblocks + x;
  for (; x != dest;) {
    *(res++) = xorBlocks(*(x++), y);
  }
}

inline void xorBlocks_arr(block256 *res, const block256 *x, const block256 *y,
                          int nblocks) {
  const block256 *dest = nblocks + x;
  for (; x != dest;) {
    *(res++) = xorBlocks(*(x++), *(y++));
  }
}
inline void xorBlocks_arr(block256 *res, const block256 *x, block256 y,
                          int nblocks) {
  const block256 *dest = nblocks + x;
  for (; x != dest;) {
    *(res++) = xorBlocks(*(x++), y);
  }
}

__attribute__((target("sse4.1,sse2"))) inline bool
cmpBlock(const block128 *x, const block128 *y, int nblocks) {
  const block128 *dest = nblocks + x;
  for (; x != dest;) {
    __m128i vcmp = _mm_xor_si128(*(x++), *(y++));
    if (!_mm_testz_si128(vcmp, vcmp))
      return false;
  }
  return true;
}

__attribute__((target("avx2,avx"))) inline bool
cmpBlock(const block256 *x, const block256 *y, int nblocks) {
  const block256 *dest = nblocks + x;
  for (; x != dest;) {
    __m256i vcmp = _mm256_xor_si256(*(x++), *(y++));
    if (!_mm256_testz_si256(vcmp, vcmp))
      return false;
  }
  return true;
}

// deprecate soon
inline bool block_cmp(const block128 *x, const block128 *y, int nblocks) {
  return cmpBlock(x, y, nblocks);
}

inline bool block_cmp(const block256 *x, const block256 *y, int nblocks) {
  return cmpBlock(x, y, nblocks);
}

__attribute__((target("sse4.1"))) inline bool isZero(const block128 *b) {
  return _mm_testz_si128(*b, *b) > 0;
}

__attribute__((target("avx"))) inline bool isZero(const block256 *b) {
  return _mm256_testz_si256(*b, *b) > 0;
}

__attribute__((target("sse4.1,sse2"))) inline bool isOne(const block128 *b) {
  __m128i neq = _mm_xor_si128(*b, one_block());
  return _mm_testz_si128(neq, neq) > 0;
}

/* Linear orthomorphism function
 * [REF] Implementation of "Efficient and Secure Multiparty Computation from
 * Fixed-Key Block Ciphers" https://eprint.iacr.org/2019/074.pdf
 */
#ifdef __x86_64__
__attribute__((target("sse2")))
#endif
inline block128
sigma(block128 a) {
  return _mm_shuffle_epi32(a, 78) ^
         (a & makeBlock128(0xFFFFFFFFFFFFFFFF, 0x00));
}

inline block128 set_bit(const block128 &a, int i) {
  if (i < 64)
    return makeBlock128(0L, 1ULL << i) | a;
  else
    return makeBlock128(1ULL << (i - 64), 0) | a;
}

// Modified from
// https://mischasan.wordpress.com/2011/10/03/the-full-sse2-bit-matrix-transpose-routine/
// with inner most loops changed to _mm_set_epi8 and _mm_set_epi16
#define INP(x, y) inp[(x)*ncols / 8 + (y) / 8]
#define OUT(x, y) out[(y)*nrows / 8 + (x) / 8]

__attribute__((target("sse2"))) inline void
sse_trans(uint8_t *out, uint8_t const *inp, uint64_t nrows, uint64_t ncols) {
  uint64_t rr, cc;
  int i, h;
  union {
    __m128i x;
    uint8_t b[16];
  } tmp;
  __m128i vec;
  assert(nrows % 8 == 0 && ncols % 8 == 0);

  // Do the main body in 16x8 blocks:
  for (rr = 0; rr <= nrows - 16; rr += 16) {
    for (cc = 0; cc < ncols; cc += 8) {
      vec = _mm_set_epi8(INP(rr + 15, cc), INP(rr + 14, cc), INP(rr + 13, cc),
                         INP(rr + 12, cc), INP(rr + 11, cc), INP(rr + 10, cc),
                         INP(rr + 9, cc), INP(rr + 8, cc), INP(rr + 7, cc),
                         INP(rr + 6, cc), INP(rr + 5, cc), INP(rr + 4, cc),
                         INP(rr + 3, cc), INP(rr + 2, cc), INP(rr + 1, cc),
                         INP(rr + 0, cc));
      for (i = 8; --i >= 0; vec = _mm_slli_epi64(vec, 1))
        *(uint16_t *)&OUT(rr, cc + i) = _mm_movemask_epi8(vec);
    }
  }
  if (rr == nrows)
    return;

  // The remainder is a block128 of 8x(16n+8) bits (n may be 0).
  //  Do a PAIR of 8x8 blocks in each step:
  if ((ncols % 8 == 0 && ncols % 16 != 0) ||
      (nrows % 8 == 0 && nrows % 16 != 0)) {
    // The fancy optimizations in the else-branch don't work if the above
    // if-condition holds, so we use the simpler non-simd variant for that case.
    for (cc = 0; cc <= ncols - 16; cc += 16) {
      for (i = 0; i < 8; ++i) {
        tmp.b[i] = h = *(uint16_t const *)&INP(rr + i, cc);
        tmp.b[i + 8] = h >> 8;
      }
      for (i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1)) {
        OUT(rr, cc + i) = h = _mm_movemask_epi8(tmp.x);
        OUT(rr, cc + i + 8) = h >> 8;
      }
    }
  } else {
    for (cc = 0; cc <= ncols - 16; cc += 16) {
      vec = _mm_set_epi16(*(uint16_t const *)&INP(rr + 7, cc),
                          *(uint16_t const *)&INP(rr + 6, cc),
                          *(uint16_t const *)&INP(rr + 5, cc),
                          *(uint16_t const *)&INP(rr + 4, cc),
                          *(uint16_t const *)&INP(rr + 3, cc),
                          *(uint16_t const *)&INP(rr + 2, cc),
                          *(uint16_t const *)&INP(rr + 1, cc),
                          *(uint16_t const *)&INP(rr + 0, cc));
      for (i = 8; --i >= 0; vec = _mm_slli_epi64(vec, 1)) {
        OUT(rr, cc + i) = h = _mm_movemask_epi8(vec);
        OUT(rr, cc + i + 8) = h >> 8;
      }
    }
  }
  if (cc == ncols)
    return;

  //  Do the remaining 8x8 block128:
  for (i = 0; i < 8; ++i)
    tmp.b[i] = INP(rr + i, cc);
  for (i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1))
    OUT(rr, cc + i) = _mm_movemask_epi8(tmp.x);
}

const char fix_key[] = "\x61\x7e\x8d\xa2\xa0\x51\x1e\x96"
                       "\x5e\x41\xc2\x9b\x15\x3f\xc7\x7a";

/*
        This file is part of JustGarble.

        JustGarble is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        JustGarble is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with JustGarble.  If not, see <http://www.gnu.org/licenses/>.

 */

/*------------------------------------------------------------------------
  / OCB Version 3 Reference Code (Optimized C)     Last modified 08-SEP-2012
  /-------------------------------------------------------------------------
  / Copyright (c) 2012 Ted Krovetz.
  /
  / Permission to use, copy, modify, and/or distribute this software for any
  / purpose with or without fee is hereby granted, provided that the above
  / copyright notice and this permission notice appear in all copies.
  /
  / THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  / WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  / MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  / ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  / WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  / ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  / OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
  /
  / Phillip Rogaway holds patents relevant to OCB. See the following for
  / his patent grant: http://www.cs.ucdavis.edu/~rogaway/ocb/grant.htm
  /
  / Special thanks to Keegan McAllister for suggesting several good improvements
  /
  / Comments are welcome: Ted Krovetz <ted@krovetz.net> - Dedicated to Laurel K
  /------------------------------------------------------------------------- */
__attribute__((target("sse2"))) inline block128 double_block(block128 bl) {
  const __m128i mask = _mm_set_epi32(135, 1, 1, 1);
  __m128i tmp = _mm_srai_epi32(bl, 31);
  tmp = _mm_and_si128(tmp, mask);
  tmp = _mm_shuffle_epi32(tmp, _MM_SHUFFLE(2, 1, 0, 3));
  bl = _mm_slli_epi32(bl, 1);
  return _mm_xor_si128(bl, tmp);
}

__attribute__((target("sse2"))) inline block128 LEFTSHIFT1(block128 bl) {
  const __m128i mask = _mm_set_epi32(0, 0, (1 << 31), 0);
  __m128i tmp = _mm_and_si128(bl, mask);
  bl = _mm_slli_epi64(bl, 1);
  return _mm_xor_si128(bl, tmp);
}
__attribute__((target("sse2"))) inline block128 RIGHTSHIFT(block128 bl) {
  const __m128i mask = _mm_set_epi32(0, 1, 0, 0);
  __m128i tmp = _mm_and_si128(bl, mask);
  bl = _mm_slli_epi64(bl, 1);
  return _mm_xor_si128(bl, tmp);
}
} // namespace sci
#endif // UTIL_BLOCK_H__
