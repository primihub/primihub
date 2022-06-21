/*
 *  AESNI.c: AES using AES-NI instructions
 *
 * Written in 2013 by Sebastian Ramacher <sebastian@ramacher.at>
 *
 * ===================================================================
 * The contents of this file are dedicated to the public domain.  To
 * the extent that dedication to the public domain is not available,
 * everyone is granted a worldwide, perpetual, royalty-free,
 * non-exclusive license to exercise all rights associated with the
 * contents of this file for any purpose whatsoever.
 * No rights are reserved.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ===================================================================
 *
 * Modified for CrypTFlow2
 */

#ifndef LIBGARBLE_AES_NI_H
#define LIBGARBLE_AES_NI_H

#if !defined(__AES__)
#error "AES-NI instructions not enabled"
#endif

#include <immintrin.h>
#include <stdlib.h>
#include <wmmintrin.h>
#if defined(HAVE__ALIGNED_MALLOC)
#include <malloc.h>
#endif
#include "src/primihub/protocol/cryptflow2/utils/block.h"

#define MODULE_NAME _AESNI
#define BLOCK_SIZE 16
#define KEY_SIZE 0

#define MAXKC (256 / 32)
#define MAXKB (256 / 8)
#define MAXNR 14

namespace primihub::sci {

typedef unsigned char u8;

typedef struct {
  block128 rk[15];
  int rounds;
} AESNI_KEY;

static block128 one = makeBlock128(0L, 1L);
static block128 two = makeBlock128(0L, 2L);

#define ENC_256_round(t)                                                       \
  {                                                                            \
    b1 = _mm_aesenc_si128(b1, key[0].rk[t]);                                   \
    b2 = _mm_aesenc_si128(b2, key[1].rk[t]);                                   \
    b3 = _mm_aesenc_si128(b3, key[2].rk[t]);                                   \
    b4 = _mm_aesenc_si128(b4, key[3].rk[t]);                                   \
    b5 = _mm_aesenc_si128(b5, key[4].rk[t]);                                   \
    b6 = _mm_aesenc_si128(b6, key[5].rk[t]);                                   \
    b7 = _mm_aesenc_si128(b7, key[6].rk[t]);                                   \
    b8 = _mm_aesenc_si128(b8, key[7].rk[t]);                                   \
  }

/* Helper functions to expand keys */

static inline block128 aes128_keyexpand(block128 key) {
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, _mm_slli_si128(key, 4));
}

static inline block128 aes192_keyexpand_2(block128 key, block128 key2) {
  key = _mm_shuffle_epi32(key, 0xff);
  key2 = _mm_xor_si128(key2, _mm_slli_si128(key2, 4));
  return _mm_xor_si128(key, key2);
}

#define KEYEXP128_H(K1, K2, I, S)                                              \
  _mm_xor_si128(aes128_keyexpand(K1),                                          \
                _mm_shuffle_epi32(_mm_aeskeygenassist_si128(K2, I), S))

#define KEYEXP128(K, I) KEYEXP128_H(K, K, I, 0xff)
#define KEYEXP192(K1, K2, I) KEYEXP128_H(K1, K2, I, 0x55)
#define KEYEXP192_2(K1, K2) aes192_keyexpand_2(K1, K2)
#define KEYEXP256(K1, K2, I) KEYEXP128_H(K1, K2, I, 0xff)
#define KEYEXP256_2(K1, K2) KEYEXP128_H(K1, K2, 0x00, 0xaa)

/* Encryption key setup */
static inline void aes_key_setup_enc(block128 rk[], const u8 *cipherKey,
                                     int keylen) {
  switch (keylen) {
  case 16: {
    /* 128 bit key setup */
    rk[0] = _mm_loadu_si128((const block128 *)cipherKey);
    rk[1] = KEYEXP128(rk[0], 0x01);
    rk[2] = KEYEXP128(rk[1], 0x02);
    rk[3] = KEYEXP128(rk[2], 0x04);
    rk[4] = KEYEXP128(rk[3], 0x08);
    rk[5] = KEYEXP128(rk[4], 0x10);
    rk[6] = KEYEXP128(rk[5], 0x20);
    rk[7] = KEYEXP128(rk[6], 0x40);
    rk[8] = KEYEXP128(rk[7], 0x80);
    rk[9] = KEYEXP128(rk[8], 0x1B);
    rk[10] = KEYEXP128(rk[9], 0x36);
    break;
  }
  case 24: {
    /* 192 bit key setup */
    block128 temp[2];
    rk[0] = _mm_loadu_si128((const block128 *)cipherKey);
    rk[1] = _mm_loadu_si128((const block128 *)(cipherKey + 16));
    temp[0] = KEYEXP192(rk[0], rk[1], 0x01);
    temp[1] = KEYEXP192_2(temp[0], rk[1]);
    rk[1] = (block128)_mm_shuffle_pd((__m128d)rk[1], (__m128d)temp[0], 0);
    rk[2] = (block128)_mm_shuffle_pd((__m128d)temp[0], (__m128d)temp[1], 1);
    rk[3] = KEYEXP192(temp[0], temp[1], 0x02);
    rk[4] = KEYEXP192_2(rk[3], temp[1]);
    temp[0] = KEYEXP192(rk[3], rk[4], 0x04);
    temp[1] = KEYEXP192_2(temp[0], rk[4]);
    rk[4] = (block128)_mm_shuffle_pd((__m128d)rk[4], (__m128d)temp[0], 0);
    rk[5] = (block128)_mm_shuffle_pd((__m128d)temp[0], (__m128d)temp[1], 1);
    rk[6] = KEYEXP192(temp[0], temp[1], 0x08);
    rk[7] = KEYEXP192_2(rk[6], temp[1]);
    temp[0] = KEYEXP192(rk[6], rk[7], 0x10);
    temp[1] = KEYEXP192_2(temp[0], rk[7]);
    rk[7] = (block128)_mm_shuffle_pd((__m128d)rk[7], (__m128d)temp[0], 0);
    rk[8] = (block128)_mm_shuffle_pd((__m128d)temp[0], (__m128d)temp[1], 1);
    rk[9] = KEYEXP192(temp[0], temp[1], 0x20);
    rk[10] = KEYEXP192_2(rk[9], temp[1]);
    temp[0] = KEYEXP192(rk[9], rk[10], 0x40);
    temp[1] = KEYEXP192_2(temp[0], rk[10]);
    rk[10] = (block128)_mm_shuffle_pd((__m128d)rk[10], (__m128d)temp[0], 0);
    rk[11] = (block128)_mm_shuffle_pd((__m128d)temp[0], (__m128d)temp[1], 1);
    rk[12] = KEYEXP192(temp[0], temp[1], 0x80);
    break;
  }
  case 32: {
    /* 256 bit key setup */
    rk[0] = _mm_loadu_si128((const block128 *)cipherKey);
    rk[1] = _mm_loadu_si128((const block128 *)(cipherKey + 16));
    rk[2] = KEYEXP256(rk[0], rk[1], 0x01);
    rk[3] = KEYEXP256_2(rk[1], rk[2]);
    rk[4] = KEYEXP256(rk[2], rk[3], 0x02);
    rk[5] = KEYEXP256_2(rk[3], rk[4]);
    rk[6] = KEYEXP256(rk[4], rk[5], 0x04);
    rk[7] = KEYEXP256_2(rk[5], rk[6]);
    rk[8] = KEYEXP256(rk[6], rk[7], 0x08);
    rk[9] = KEYEXP256_2(rk[7], rk[8]);
    rk[10] = KEYEXP256(rk[8], rk[9], 0x10);
    rk[11] = KEYEXP256_2(rk[9], rk[10]);
    rk[12] = KEYEXP256(rk[10], rk[11], 0x20);
    rk[13] = KEYEXP256_2(rk[11], rk[12]);
    rk[14] = KEYEXP256(rk[12], rk[13], 0x40);
    break;
  }
  }
}

/* Encryption key (256) setup */
static inline void aes_key_setup_enc(block128 rk[], const block256 cipherKey) {
  /* 256 bit key setup */
  _mm256_storeu2_m128i(rk + 1, rk, cipherKey);
  // rk[0] = _mm_loadu_si128((const block128*) cipherKey);
  // rk[1] = _mm_loadu_si128((const block128*) (cipherKey+16));
  rk[2] = KEYEXP256(rk[0], rk[1], 0x01);
  rk[3] = KEYEXP256_2(rk[1], rk[2]);
  rk[4] = KEYEXP256(rk[2], rk[3], 0x02);
  rk[5] = KEYEXP256_2(rk[3], rk[4]);
  rk[6] = KEYEXP256(rk[4], rk[5], 0x04);
  rk[7] = KEYEXP256_2(rk[5], rk[6]);
  rk[8] = KEYEXP256(rk[6], rk[7], 0x08);
  rk[9] = KEYEXP256_2(rk[7], rk[8]);
  rk[10] = KEYEXP256(rk[8], rk[9], 0x10);
  rk[11] = KEYEXP256_2(rk[9], rk[10]);
  rk[12] = KEYEXP256(rk[10], rk[11], 0x20);
  rk[13] = KEYEXP256_2(rk[11], rk[12]);
  rk[14] = KEYEXP256(rk[12], rk[13], 0x40);
}

/* Decryption key setup */
static inline void aes_key_setup_dec(block128 dk[], const block128 ek[],
                                     int rounds) {
  dk[rounds] = ek[0];
  for (int i = 1; i < rounds; ++i) {
    dk[rounds - i] = _mm_aesimc_si128(ek[i]);
  }
  dk[0] = ek[rounds];
}

static inline void AESNI_set_encrypt_key(AESNI_KEY *self, unsigned char *key,
                                         int keylen) {
  switch (keylen) {
  case 16:
    self->rounds = 10;
    break;
  case 24:
    self->rounds = 12;
    break;
  case 32:
    self->rounds = 14;
    break;
  }

  aes_key_setup_enc(self->rk, key, keylen);
}

static inline void AESNI_set_encrypt_key(AESNI_KEY *self, const block256 key) {
  self->rounds = 14;
  aes_key_setup_enc(self->rk, key);
}

static inline void AESNI_set_decrypt_key(AESNI_KEY *self, unsigned char *key,
                                         int keylen) {
  switch (keylen) {
  case 16:
    self->rounds = 10;
    break;
  case 24:
    self->rounds = 12;
    break;
  case 32:
    self->rounds = 14;
    break;
  }

  AESNI_KEY temp_key;
  aes_key_setup_enc(temp_key.rk, key, keylen);
  aes_key_setup_dec(self->rk, temp_key.rk, self->rounds);
}

/*
static inline void block_encrypt(AESNI_KEY* self, const u8* in, u8* out)
{
    block128 m = _mm_loadu_si128((const block128*) in);
    m = _mm_xor_si128(m, self->rk[0]);
    m = _mm_aesenc_si128(m, self->rk[1]);
    m = _mm_aesenc_si128(m, self->rk[2]);
    m = _mm_aesenc_si128(m, self->rk[3]);
    m = _mm_aesenc_si128(m, self->rk[4]);
    m = _mm_aesenc_si128(m, self->rk[5]);
    m = _mm_aesenc_si128(m, self->rk[6]);
    m = _mm_aesenc_si128(m, self->rk[7]);
    m = _mm_aesenc_si128(m, self->rk[8]);
    m = _mm_aesenc_si128(m, self->rk[9]);
    if (self->rounds != 10) {
        m = _mm_aesenc_si128(m, self->rk[10]);
        m = _mm_aesenc_si128(m, self->rk[11]);
        if (self->rounds == 14) {
            m = _mm_aesenc_si128(m, self->rk[12]);
            m = _mm_aesenc_si128(m, self->rk[13]);
        }
    }
    m = _mm_aesenclast_si128(m, self->rk[self->rounds]);
    _mm_storeu_si128((block128*) out, m);
}

static inline void block_decrypt(AESNI_KEY* self, const u8* in, u8* out)
{
    block128 m = _mm_loadu_si128((const block128*) in);
    m = _mm_xor_si128(m, self->rk[0]);
    m = _mm_aesdec_si128(m, self->rk[1]);
    m = _mm_aesdec_si128(m, self->rk[2]);
    m = _mm_aesdec_si128(m, self->rk[3]);
    m = _mm_aesdec_si128(m, self->rk[4]);
    m = _mm_aesdec_si128(m, self->rk[5]);
    m = _mm_aesdec_si128(m, self->rk[6]);
    m = _mm_aesdec_si128(m, self->rk[7]);
    m = _mm_aesdec_si128(m, self->rk[8]);
    m = _mm_aesdec_si128(m, self->rk[9]);
    if (self->rounds != 10) {
        m = _mm_aesdec_si128(m, self->rk[10]);
        m = _mm_aesdec_si128(m, self->rk[11]);
        if (self->rounds == 14) {
            m = _mm_aesdec_si128(m, self->rk[12]);
            m = _mm_aesdec_si128(m, self->rk[13]);
        }
    }
    m = _mm_aesdeclast_si128(m, self->rk[self->rounds]);
    _mm_storeu_si128((block128*) out, m);
}
*/

#ifdef __GNUC__
#ifndef __clang__
#pragma GCC push_options
#pragma GCC optimize("unroll-loops")
#endif
#endif
static inline void __attribute__((target("aes,sse2")))
AESNI_ecb_encrypt_blks_8(block128 *blks, const AESNI_KEY *keys) {
  for (unsigned int i = 0; i < 8; ++i)
    blks[i] = _mm_xor_si128(blks[i], keys[i].rk[0]);
  for (int j = 1; j < 14; ++j)
    for (unsigned int i = 0; i < 8; ++i)
      blks[i] = _mm_aesenc_si128(blks[i], keys[i].rk[j]);
  for (unsigned int i = 0; i < 8; ++i)
    blks[i] = _mm_aesenclast_si128(blks[i], keys[i].rk[keys->rounds]);
}

static inline void __attribute__((target("aes,sse2")))
AESNI_ecb_encrypt_blks_ks(block128 *blks, unsigned int nblks,
                          const AESNI_KEY *key) {

  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_xor_si128(one, key[i].rk[0]);
  // blks[i] = _mm_xor_si128(blks[i], key[i].rk[0]);
  for (int j = 1; j < key->rounds; ++j)
    for (unsigned int i = 0; i < nblks; ++i)
      blks[i] = _mm_aesenc_si128(blks[i], key[i].rk[j]);
  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_aesenclast_si128(blks[i], key[i].rk[key->rounds]);
}

static inline void __attribute__((target("aes,sse2")))
AESNI_ecb_encrypt_blks_ks_x8(block128 *blks, unsigned int nblks,
                             const AESNI_KEY *key) {
  __m128i b1, b2, b3, b4, b5, b6, b7, b8;
  assert((nblks == 8) &&
         "AESNI_ecb_encrypt_blks_ks_x8 only works for 8 blocks");
  b1 = _mm_xor_si128(one, key[0].rk[0]);
  b2 = _mm_xor_si128(one, key[1].rk[0]);
  b3 = _mm_xor_si128(one, key[2].rk[0]);
  b4 = _mm_xor_si128(one, key[3].rk[0]);
  b5 = _mm_xor_si128(one, key[4].rk[0]);
  b6 = _mm_xor_si128(one, key[5].rk[0]);
  b7 = _mm_xor_si128(one, key[6].rk[0]);
  b8 = _mm_xor_si128(one, key[7].rk[0]);

  ENC_256_round(1);
  ENC_256_round(2);
  ENC_256_round(3);
  ENC_256_round(4);
  ENC_256_round(5);
  ENC_256_round(6);
  ENC_256_round(7);
  ENC_256_round(8);
  ENC_256_round(9);
  ENC_256_round(10);
  ENC_256_round(11);
  ENC_256_round(12);
  ENC_256_round(13);

  blks[0] = _mm_aesenclast_si128(b1, key[0].rk[key->rounds]);
  blks[1] = _mm_aesenclast_si128(b2, key[1].rk[key->rounds]);
  blks[2] = _mm_aesenclast_si128(b3, key[2].rk[key->rounds]);
  blks[3] = _mm_aesenclast_si128(b4, key[3].rk[key->rounds]);
  blks[4] = _mm_aesenclast_si128(b5, key[4].rk[key->rounds]);
  blks[5] = _mm_aesenclast_si128(b6, key[5].rk[key->rounds]);
  blks[6] = _mm_aesenclast_si128(b7, key[6].rk[key->rounds]);
  blks[7] = _mm_aesenclast_si128(b8, key[7].rk[key->rounds]);
}

static inline void __attribute__((target("aes,sse2")))
AESNI_ecb_encrypt_blks(block128 *blks, unsigned int nblks,
                       const AESNI_KEY *key) {
  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_xor_si128(blks[i], key->rk[0]);
  for (int j = 1; j < key->rounds; ++j)
    for (unsigned int i = 0; i < nblks; ++i)
      blks[i] = _mm_aesenc_si128(blks[i], key->rk[j]);
  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_aesenclast_si128(blks[i], key->rk[key->rounds]);
}
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC pop_options
#endif
#endif

static inline void __attribute__((target("aes,sse2")))
AESNI_ecb_decrypt_blks(block128 *blks, unsigned nblks, const AESNI_KEY *key) {
  unsigned i, j, rnds = key->rounds;
  for (i = 0; i < nblks; ++i)
    blks[i] = _mm_xor_si128(blks[i], key->rk[0]);
  for (j = 1; j < rnds; ++j)
    for (i = 0; i < nblks; ++i)
      blks[i] = _mm_aesdec_si128(blks[i], key->rk[j]);
  for (i = 0; i < nblks; ++i)
    blks[i] = _mm_aesdeclast_si128(blks[i], key->rk[j]);
}

/*
static inline void AESNI_encrypt_label(Label& lb, const AESNI_KEY* key, uint64_t
gid, uint8_t entry, uint8_t pos){
    // Label counters(one, two);
    uint64_t tweak_hi = gid;
    uint64_t tweak_lo = (entry << 2) || (pos << 1);
    block128 counters[2];
    counters[0] = makeBlock128(tweak_hi, tweak_lo | 0);
    counters[1] = makeBlock128(tweak_hi, tweak_lo | 1);

    AESNI_ecb_encrypt_blks((block128*) &counters, 2, key);
    lb.hi = _mm_xor_si128(counters[1], lb.hi);
    lb.lo = _mm_xor_si128(counters[0], lb.lo);
}
*/
} // namespace primihub::sci
#endif
