/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2021 Microsoft Research

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

#ifndef EMP_F2K_H__
#define EMP_F2K_H__
#include "src/primihub/protocol/cryptflow2/utils/block.h"

namespace primihub::sci {
/* multiplication in galois field without reduction */
#ifdef __x86_64__
__attribute__((target("sse2,pclmul"))) inline void
mul128(__m128i a, __m128i b, __m128i *res1, __m128i *res2) {
  __m128i tmp3, tmp4, tmp5, tmp6;
  tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
  tmp4 = _mm_clmulepi64_si128(a, b, 0x10);
  tmp5 = _mm_clmulepi64_si128(a, b, 0x01);
  tmp6 = _mm_clmulepi64_si128(a, b, 0x11);

  tmp4 = _mm_xor_si128(tmp4, tmp5);
  tmp5 = _mm_slli_si128(tmp4, 8);
  tmp4 = _mm_srli_si128(tmp4, 8);
  tmp3 = _mm_xor_si128(tmp3, tmp5);
  tmp6 = _mm_xor_si128(tmp6, tmp4);
  // initial mul now in tmp3, tmp6
  *res1 = tmp3;
  *res2 = tmp6;
}
#elif __aarch64__
inline void mul128(__m128i a, __m128i b, __m128i *res1, __m128i *res2) {
  __m128i tmp3, tmp4, tmp5, tmp6;
  poly64_t a_lo = (poly64_t)vget_low_u64(vreinterpretq_u64_m128i(a));
  poly64_t a_hi = (poly64_t)vget_high_u64(vreinterpretq_u64_m128i(a));
  poly64_t b_lo = (poly64_t)vget_low_u64(vreinterpretq_u64_m128i(b));
  poly64_t b_hi = (poly64_t)vget_high_u64(vreinterpretq_u64_m128i(b));
  tmp3 = (__m128i)vmull_p64(a_lo, b_lo);
  tmp4 = (__m128i)vmull_p64(a_hi, b_lo);
  tmp5 = (__m128i)vmull_p64(a_lo, b_hi);
  tmp6 = (__m128i)vmull_p64(a_hi, b_hi);

  tmp4 = _mm_xor_si128(tmp4, tmp5);
  tmp5 = _mm_slli_si128(tmp4, 8);
  tmp4 = _mm_srli_si128(tmp4, 8);
  tmp3 = _mm_xor_si128(tmp3, tmp5);
  tmp6 = _mm_xor_si128(tmp6, tmp4);
  // initial mul now in tmp3, tmp6
  *res1 = tmp3;
  *res2 = tmp6;
}
#endif

/* multiplication in galois field with reduction */
#ifdef __x86_64__
__attribute__((target("sse2,pclmul")))
#endif
inline void
gfmul(__m128i a, __m128i b, __m128i *res) {
  __m128i tmp3, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
  __m128i XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);
  mul128(a, b, &tmp3, &tmp6);
  tmp7 = _mm_srli_epi32(tmp6, 31);
  tmp8 = _mm_srli_epi32(tmp6, 30);
  tmp9 = _mm_srli_epi32(tmp6, 25);
  tmp7 = _mm_xor_si128(tmp7, tmp8);
  tmp7 = _mm_xor_si128(tmp7, tmp9);
  tmp8 = _mm_shuffle_epi32(tmp7, 147);

  tmp7 = _mm_and_si128(XMMMASK, tmp8);
  tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
  tmp3 = _mm_xor_si128(tmp3, tmp8);
  tmp6 = _mm_xor_si128(tmp6, tmp7);
  tmp10 = _mm_slli_epi32(tmp6, 1);
  tmp3 = _mm_xor_si128(tmp3, tmp10);
  tmp11 = _mm_slli_epi32(tmp6, 2);
  tmp3 = _mm_xor_si128(tmp3, tmp11);
  tmp12 = _mm_slli_epi32(tmp6, 7);
  tmp3 = _mm_xor_si128(tmp3, tmp12);

  *res = _mm_xor_si128(tmp3, tmp6);
}

/* inner product of two galois field vectors with reduction */
inline void vector_inn_prdt_sum_red(block128 *res, const block128 *a,
                                    const block128 *b, int sz) {
  block128 r = zero_block();
  block128 r1;
  for (int i = 0; i < sz; i++) {
    gfmul(a[i], b[i], &r1);
    r = r ^ r1;
  }
  *res = r;
}

/* inner product of two galois field vectors with reduction */
template <int N>
inline void vector_inn_prdt_sum_red(block128 *res, block128 const *a,
                                    const block128 *b) {
  vector_inn_prdt_sum_red(res, a, b, N);
}

/* inner product of two galois field vectors without reduction */
inline void vector_inn_prdt_sum_no_red(block128 *res, const block128 *a,
                                       const block128 *b, int sz) {
  block128 r1 = zero_block(), r2 = zero_block();
  block128 r11, r12;
  for (int i = 0; i < sz; i++) {
    mul128(a[i], b[i], &r11, &r12);
    r1 = r1 ^ r11;
    r2 = r2 ^ r12;
  }
  res[0] = r1;
  res[1] = r2;
}

/* inner product of two galois field vectors without reduction */
template <int N>
inline void vector_inn_prdt_sum_no_red(block128 *res, const block128 *a,
                                       const block128 *b) {
  vector_inn_prdt_sum_no_red(res, a, b, N);
}

/* coefficients of almost universal hash function */
inline void uni_hash_coeff_gen(block128 *coeff, block128 seed, int sz) {
  coeff[0] = seed;
  gfmul(seed, seed, &coeff[1]);
  gfmul(coeff[1], seed, &coeff[2]);
  block128 multiplier;
  gfmul(coeff[2], seed, &multiplier);
  coeff[3] = multiplier;
  int i = 4;
  for (; i < sz - 3; i += 4) {
    gfmul(coeff[i - 4], multiplier, &coeff[i]);
    gfmul(coeff[i - 3], multiplier, &coeff[i + 1]);
    gfmul(coeff[i - 2], multiplier, &coeff[i + 2]);
    gfmul(coeff[i - 1], multiplier, &coeff[i + 3]);
  }
  int remainder = sz % 4;
  if (remainder != 0) {
    i = sz - remainder;
    for (; i < sz; ++i)
      gfmul(coeff[i - 1], seed, &coeff[i]);
  }
}

/* coefficients of almost universal hash function */
template <int N>
inline void uni_hash_coeff_gen(block128 *coeff, block128 seed) {
  uni_hash_coeff_gen(coeff, seed, N);
}

/* packing in Galois field (v[i] * X^i for v of size 128) */
class GaloisFieldPacking {
public:
  block128 base[128];

  GaloisFieldPacking() { packing_base_gen(); }

  ~GaloisFieldPacking() {}

  void packing_base_gen() {
    uint64_t a = 0, b = 1;
    for (int i = 0; i < 64; i += 4) {
      base[i] = _mm_set_epi64x(a, b);
      base[i + 1] = _mm_set_epi64x(a, b << 1);
      base[i + 2] = _mm_set_epi64x(a, b << 2);
      base[i + 3] = _mm_set_epi64x(a, b << 3);
      b <<= 4;
    }
    a = 1, b = 0;
    for (int i = 64; i < 128; i += 4) {
      base[i] = _mm_set_epi64x(a, b);
      base[i + 1] = _mm_set_epi64x(a << 1, b);
      base[i + 2] = _mm_set_epi64x(a << 2, b);
      base[i + 3] = _mm_set_epi64x(a << 3, b);
      a <<= 4;
    }
  }

  void packing(block128 *res, block128 *data) {
    vector_inn_prdt_sum_red(res, data, base, 128);
  }
};

/* XOR of all elements in a vector */
inline void vector_self_xor(block128 *sum, block128 *data, int sz) {
  block128 res[4];
  res[0] = zero_block();
  res[1] = zero_block();
  res[2] = zero_block();
  res[3] = zero_block();
  for (int i = 0; i < (sz / 4) * 4; i += 4) {
    res[0] = data[i] ^ res[0];
    res[1] = data[i + 1] ^ res[1];
    res[2] = data[i + 2] ^ res[2];
    res[3] = data[i + 3] ^ res[3];
  }
  for (int i = (sz / 4) * 4, j = 0; i < sz; ++i, ++j)
    res[j] = data[i] ^ res[j];
  res[0] = res[0] ^ res[1];
  res[2] = res[2] ^ res[3];
  *sum = res[0] ^ res[2];
}

/* XOR of all elements in a vector */
template <int N> inline void vector_self_xor(block128 *sum, block128 *data) {
  vector_self_xor(sum, data, N);
}
} // namespace primihub::sci
#endif
