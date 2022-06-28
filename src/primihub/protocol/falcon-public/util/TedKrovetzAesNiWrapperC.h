/**
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *
 * Copyright(c) 2013 Ted Krovetz.
 * This file is part of the SCAPI project, is was taken from the file ocb.c written by Ted Krovetz.
 * Some changes and additions have been made and only part of the file written by Ted Krovetz has been copied
 * only for the use of this project.
 *
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *
 */

// Copyright(c) 2013 Ted Krovetz.

#ifndef TED_FILE
#define TED_FILE

#ifdef ENABLE_SSE
#include <wmmintrin.h>
#endif

#include "Config.h"
#include "src/primihub/protocol/falcon-public/globals.h"

#ifndef ENABLE_SSE
#include "block.h"
#endif

#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free free
namespace primihub{
    namespace falcon
{
    typedef struct
    {
        block rd_key[15];
        int rounds;
    } AES_KEY_TED;
#define ROUNDS(ctx) ((ctx)->rounds)

#ifdef ENABLE_SSE
#define EXPAND_ASSIST(v1, v2, v3, v4, shuff_const, aes_const)         \
    v2 = _mm_aeskeygenassist_si128(v4, aes_const);                    \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),        \
                                         _mm_castsi128_ps(v1), 16));  \
    v1 = _mm_xor_si128(v1, v3);                                       \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),        \
                                         _mm_castsi128_ps(v1), 140)); \
    v1 = _mm_xor_si128(v1, v3);                                       \
    v2 = _mm_shuffle_epi32(v2, shuff_const);                          \
    v1 = _mm_xor_si128(v1, v2)

#define EXPAND192_STEP(idx, aes_const)                                        \
    EXPAND_ASSIST(x0, x1, x2, x3, 85, aes_const);                             \
    x3 = _mm_xor_si128(x3, _mm_slli_si128(x3, 4));                            \
    x3 = _mm_xor_si128(x3, _mm_shuffle_epi32(x0, 255));                       \
    kp[idx] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(tmp),          \
                                              _mm_castsi128_ps(x0), 68));     \
    kp[idx + 1] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x0),       \
                                                  _mm_castsi128_ps(x3), 78)); \
    EXPAND_ASSIST(x0, x1, x2, x3, 85, (aes_const * 2));                       \
    x3 = _mm_xor_si128(x3, _mm_slli_si128(x3, 4));                            \
    x3 = _mm_xor_si128(x3, _mm_shuffle_epi32(x0, 255));                       \
    kp[idx + 2] = x0;                                                         \
    tmp = x3
#else
#define EXPAND_ASSIST(v1, v2, v3, v4, shuff_const, aes_const) TODO("Implement it.")
#define EXPAND192_STEP(idx, aes_const) TODO("Implement it.")
#endif

    extern AES_KEY_TED fixed_aes_key;

    void AES_128_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey);
    void AES_192_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey);
    void AES_256_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey);
    void AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY_TED *aesKey);

    void AES_encryptC(block *in, block *out, AES_KEY_TED *aesKey);
    void AES_ecb_encrypt(block *blk, AES_KEY_TED *aesKey);

    void AES_ecb_encrypt_blks(block *blks, unsigned nblks, AES_KEY_TED *aesKey);
    void AES_ecb_encrypt_blks_4(block *blk, AES_KEY_TED *aesKey);
    void AES_ecb_encrypt_blks_4_in_out(block *in, block *out, AES_KEY_TED *aesKey);
    void AES_ecb_encrypt_chunk_in_out(block *in, block *out, unsigned nblks, AES_KEY_TED *aesKey);

    void AES_ctr_hash_gate(block *in, block *out, int gate, int numOfParties, AES_KEY_TED *aesKey);
    void AES_ctr_hash_gate(block *in, block *out, int gate, int numOfParties, __m128i seedIn1, __m128i seedIn2);
    __m128i *pseudoRandomFunction(__m128i seed1, __m128i seed2, int gate, int numOfParties);
    void pseudoRandomFunctionNew(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i *out);
    void AES_init(int numOfParties);
    void AES_free();
    bool firstBit(__m128i input);
    bool pseudoRandomFunctionwPipelining(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i *out);

    extern AES_KEY_TED fixed_aes_key;

    bool fixedKeyPseudoRandomFunctionwPipelining(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i *out);

    // assumes nblks==1
    block AES_ecb_encrypt_for_1(block in, AES_KEY_TED *aesKey);
    // assumes nblks==3
    void AES_ecb_encrypt_for_3(block *in, block *out, int nblks, AES_KEY_TED *aesKey);
    // assumes nblks==4
    void AES_ecb_encrypt_for_4(block *in, block *out, int nblks, AES_KEY_TED *aesKey);
    // assumes nblks==5
    void AES_ecb_encrypt_for_5(block *in, block *out, int nblks, AES_KEY_TED *aesKey);
    // assumes nblks==7
    void AES_ecb_encrypt_for_7(block *in, block *out, int nblks, AES_KEY_TED *aesKey);
}
}//primihub
#endif
