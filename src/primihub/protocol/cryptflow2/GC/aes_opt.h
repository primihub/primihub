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

#ifndef EMP_AES_OPT_KS_H__
#define EMP_AES_OPT_KS_H__

#include "src/primihub/protocol/cryptflow2/utils/aes.h"

namespace primihub::sci {
template <int NumKeys>
static inline void ks_rounds(AES_KEY *keys, block128 con, block128 con3,
                             block128 mask, int r) {
  for (int i = 0; i < NumKeys; ++i) {
    block128 key = keys[i].rd_key[r - 1];
    block128 x2 = _mm_shuffle_epi8(key, mask);
    block128 aux = _mm_aesenclast_si128(x2, con);

    block128 globAux = _mm_slli_epi64(key, 32);
    key = _mm_xor_si128(globAux, key);
    globAux = _mm_shuffle_epi8(key, con3);
    key = _mm_xor_si128(globAux, key);
    keys[i].rd_key[r] = _mm_xor_si128(aux, key);
  }
}
/*
 * AES key scheduling for 8 keys
 * [REF] Implementation of "Fast Garbling of Circuits Under Standard
 * Assumptions" https://eprint.iacr.org/2015/751.pdf
 */
template <int NumKeys>
static inline void AES_opt_key_schedule(block128 *user_key, AES_KEY *keys) {
  block128 con = _mm_set_epi32(1, 1, 1, 1);
  block128 con2 = _mm_set_epi32(0x1b, 0x1b, 0x1b, 0x1b);
  block128 con3 =
      _mm_set_epi32(0x07060504, 0x07060504, 0x0ffffffff, 0x0ffffffff);
  block128 mask = _mm_set_epi32(0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d);

  for (int i = 0; i < NumKeys; ++i) {
    keys[i].rounds = 10;
    keys[i].rd_key[0] = user_key[i];
  }

  ks_rounds<NumKeys>(keys, con, con3, mask, 1);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 2);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 3);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 4);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 5);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 6);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 7);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con, con3, mask, 8);
  con = _mm_slli_epi32(con, 1);
  ks_rounds<NumKeys>(keys, con2, con3, mask, 9);
  con2 = _mm_slli_epi32(con2, 1);
  ks_rounds<NumKeys>(keys, con2, con3, mask, 10);
}

/*
 * With numKeys keys, use each key to encrypt numEncs blocks.
 */
#ifdef __x86_64__
template <int numKeys, int numEncs>
static inline void ParaEnc(block128 *blks, AES_KEY *keys) {
  block128 *first = blks;
  for (size_t i = 0; i < numKeys; ++i) {
    block128 K = keys[i].rd_key[0];
    for (size_t j = 0; j < numEncs; ++j) {
      *blks = *blks ^ K;
      ++blks;
    }
  }

  for (unsigned int r = 1; r < 10; ++r) {
    blks = first;
    for (size_t i = 0; i < numKeys; ++i) {
      block128 K = keys[i].rd_key[r];
      for (size_t j = 0; j < numEncs; ++j) {
        *blks = _mm_aesenc_si128(*blks, K);
        ++blks;
      }
    }
  }

  blks = first;
  for (size_t i = 0; i < numKeys; ++i) {
    block128 K = keys[i].rd_key[10];
    for (size_t j = 0; j < numEncs; ++j) {
      *blks = _mm_aesenclast_si128(*blks, K);
      ++blks;
    }
  }
}
#elif __aarch64__
template <int numKeys, int numEncs>
static inline void ParaEnc(block128 *_blks, AES_KEY *keys) {
  uint8x16_t *first = (uint8x16_t *)(_blks);

  for (unsigned int r = 0; r < 9; ++r) {
    auto blks = first;
    for (size_t i = 0; i < numKeys; ++i) {
      uint8x16_t K = vreinterpretq_u8_m128i(keys[i].rd_key[r]);
      for (size_t j = 0; j < numEncs; ++j, ++blks)
        *blks = vaeseq_u8(*blks, K);
    }
    blks = first;
    for (size_t i = 0; i < numKeys; ++i) {
      for (size_t j = 0; j < numEncs; ++j, ++blks)
        *blks = vaesmcq_u8(*blks);
    }
  }

  auto blks = first;
  for (size_t i = 0; i < numKeys; ++i) {
    uint8x16_t K = vreinterpretq_u8_m128i(keys[i].rd_key[9]);
    for (size_t j = 0; j < numEncs; ++j, ++blks)
      *blks = vaeseq_u8(*blks, K) ^ K;
  }
}
#endif

} // namespace primihub::sci
#endif
