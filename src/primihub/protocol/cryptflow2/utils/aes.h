/* crypto/aes/aes.h -*- mode:C; c-file-style: "eay" -*- */
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#ifndef LIBGARBLE_AES_H
#define LIBGARBLE_AES_H

#include "src/primihub/protocol/cryptflow2/utils/block.h"

namespace primihub::sci {

typedef struct {
  block128 rd_key[11];
  unsigned int rounds;
} AES_KEY;

#define EXPAND_ASSIST(v1, v2, v3, v4, shuff_const, aes_const)                  \
  v2 = _mm_aeskeygenassist_si128(v4, aes_const);                               \
  v3 = _mm_castps_si128(                                                       \
      _mm_shuffle_ps(_mm_castsi128_ps(v3), _mm_castsi128_ps(v1), 16));         \
  v1 = _mm_xor_si128(v1, v3);                                                  \
  v3 = _mm_castps_si128(                                                       \
      _mm_shuffle_ps(_mm_castsi128_ps(v3), _mm_castsi128_ps(v1), 140));        \
  v1 = _mm_xor_si128(v1, v3);                                                  \
  v2 = _mm_shuffle_epi32(v2, shuff_const);                                     \
  v1 = _mm_xor_si128(v1, v2)

inline void __attribute__((target("aes,sse2")))
AES_set_encrypt_key(const block128 userkey, AES_KEY *key) {
  block128 x0, x1, x2;
  block128 *kp = key->rd_key;
  kp[0] = x0 = userkey;
  x2 = _mm_setzero_si128();
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 1);
  kp[1] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 2);
  kp[2] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 4);
  kp[3] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 8);
  kp[4] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 16);
  kp[5] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 32);
  kp[6] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 64);
  kp[7] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 128);
  kp[8] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 27);
  kp[9] = x0;
  EXPAND_ASSIST(x0, x1, x2, x0, 255, 54);
  kp[10] = x0;
  key->rounds = 10;
}

inline void __attribute__((target("aes,sse2")))
AES_ecb_encrypt_blks(block128 *blks, unsigned int nblks, const AES_KEY *key) {
  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_xor_si128(blks[i], key->rd_key[0]);
  for (unsigned int j = 1; j < key->rounds; ++j)
    for (unsigned int i = 0; i < nblks; ++i)
      blks[i] = _mm_aesenc_si128(blks[i], key->rd_key[j]);
  for (unsigned int i = 0; i < nblks; ++i)
    blks[i] = _mm_aesenclast_si128(blks[i], key->rd_key[key->rounds]);
}

inline void __attribute__((target("aes,sse2")))
AES_set_decrypt_key_fast(AES_KEY *dkey, const AES_KEY *ekey) {
  int j = 0;
  int i = ekey->rounds;
#if (OCB_KEY_LEN == 0)
  dkey->rounds = i;
#endif
  dkey->rd_key[i--] = ekey->rd_key[j++];
  while (i)
    dkey->rd_key[i--] = _mm_aesimc_si128(ekey->rd_key[j++]);
  dkey->rd_key[i] = ekey->rd_key[j];
}

inline void __attribute__((target("aes,sse2")))
AES_set_decrypt_key(block128 userkey, AES_KEY *key) {
  AES_KEY temp_key;
  AES_set_encrypt_key(userkey, &temp_key);
  AES_set_decrypt_key_fast(key, &temp_key);
}

inline void __attribute__((target("aes,sse2")))
AES_ecb_decrypt_blks(block128 *blks, unsigned nblks, const AES_KEY *key) {
  unsigned i, j, rnds = key->rounds;
  for (i = 0; i < nblks; ++i)
    blks[i] = _mm_xor_si128(blks[i], key->rd_key[0]);
  for (j = 1; j < rnds; ++j)
    for (i = 0; i < nblks; ++i)
      blks[i] = _mm_aesdec_si128(blks[i], key->rd_key[j]);
  for (i = 0; i < nblks; ++i)
    blks[i] = _mm_aesdeclast_si128(blks[i], key->rd_key[j]);
}
} // namespace primihub::sci
#endif
