// Copyright [2021] <primihub.com>
#include "src/primihub/util/crypto/aes/aes_dec.h"

namespace primihub {

#ifdef OC_ENABLE_PORTABLE_AES
template class AESDec<Portable>;
#endif

#ifdef OC_ENABLE_AESNI
template class AESDec<NI>;
#endif

#ifdef OC_ENABLE_PORTABLE_AES
template<>
block AESDec<Portable>::roundDec(block state, const block& roundKey) {
  InvShiftRows(state);
  InvSubBytes(state);
  state = state ^ roundKey;
  InvMixColumns(state);
  return state;
}

template<>
block AESDec<Portable>::finalDec(block state, const block& roundKey) {
  InvShiftRows(state);
  InvSubBytes(state);
  state = state ^ roundKey;
  return state;
}

template<>
void AESDec<Portable>::setKey(const block& userKey) {
  // same as enc but in reverse
  AES<Portable> aes;
  aes.setKey(userKey);
  std::copy(aes.mRoundKey.begin(), aes.mRoundKey.end(), mRoundKey.rbegin());
}

#endif

#ifdef OC_ENABLE_AESNI
template<>
block AESDec<NI>::roundDec(block state, const block& roundKey) {
  return _mm_aesdec_si128(state, roundKey);
}

template<>
block AESDec<NI>::finalDec(block state, const block& roundKey) {
  return _mm_aesdeclast_si128(state, roundKey);
}

template<>
void AESDec<NI>::setKey(const block& userKey) {
  const block& v0 = userKey;
  const block  v1 = keyGenHelper(v0, _mm_aeskeygenassist_si128(v0, 0x01));
  const block  v2 = keyGenHelper(v1, _mm_aeskeygenassist_si128(v1, 0x02));
  const block  v3 = keyGenHelper(v2, _mm_aeskeygenassist_si128(v2, 0x04));
  const block  v4 = keyGenHelper(v3, _mm_aeskeygenassist_si128(v3, 0x08));
  const block  v5 = keyGenHelper(v4, _mm_aeskeygenassist_si128(v4, 0x10));
  const block  v6 = keyGenHelper(v5, _mm_aeskeygenassist_si128(v5, 0x20));
  const block  v7 = keyGenHelper(v6, _mm_aeskeygenassist_si128(v6, 0x40));
  const block  v8 = keyGenHelper(v7, _mm_aeskeygenassist_si128(v7, 0x80));
  const block  v9 = keyGenHelper(v8, _mm_aeskeygenassist_si128(v8, 0x1B));
  const block  v10 = keyGenHelper(v9, _mm_aeskeygenassist_si128(v9, 0x36));


  _mm_storeu_si128(&mRoundKey[0].m128i(), v10);
  _mm_storeu_si128(&mRoundKey[1].m128i(), _mm_aesimc_si128(v9));
  _mm_storeu_si128(&mRoundKey[2].m128i(), _mm_aesimc_si128(v8));
  _mm_storeu_si128(&mRoundKey[3].m128i(), _mm_aesimc_si128(v7));
  _mm_storeu_si128(&mRoundKey[4].m128i(), _mm_aesimc_si128(v6));
  _mm_storeu_si128(&mRoundKey[5].m128i(), _mm_aesimc_si128(v5));
  _mm_storeu_si128(&mRoundKey[6].m128i(), _mm_aesimc_si128(v4));
  _mm_storeu_si128(&mRoundKey[7].m128i(), _mm_aesimc_si128(v3));
  _mm_storeu_si128(&mRoundKey[8].m128i(), _mm_aesimc_si128(v2));
  _mm_storeu_si128(&mRoundKey[9].m128i(), _mm_aesimc_si128(v1));
  _mm_storeu_si128(&mRoundKey[10].m128i(), v0);
}
#endif

template<AESTypes type>
void AESDec<type>::ecbDecBlock(const block& ciphertext, block& plaintext) {
  //std::cout << "\ndec[0] " << ciphertext << " ^ " << mRoundKey[0] << std::endl;

  plaintext = ciphertext ^ mRoundKey[0];
  //std::cout << "dec[1] " << plaintext << " ^ " << mRoundKey[1] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[1]);
  //std::cout << "dec[2] " << plaintext << " ^ " << mRoundKey[2] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[2]);
  //std::cout << "dec[3] " << plaintext << " ^ " << mRoundKey[3] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[3]);
  //std::cout << "dec[4] " << plaintext << " ^ " << mRoundKey[4] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[4]);
  //std::cout << "dec[5] " << plaintext << " ^ " << mRoundKey[5] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[5]);
  //std::cout << "dec[6] " << plaintext << " ^ " << mRoundKey[6] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[6]);
  //std::cout << "dec[7] " << plaintext << " ^ " << mRoundKey[7] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[7]);
  //std::cout << "dec[8] " << plaintext << " ^ " << mRoundKey[8] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[8]);
  //std::cout << "dec[9] " << plaintext << " ^ " << mRoundKey[9] << std::endl;
  plaintext = roundDec(plaintext, mRoundKey[9]);
  //std::cout << "dec[10] " << plaintext << " ^ " << mRoundKey[10] << std::endl;
  plaintext = finalDec(plaintext, mRoundKey[10]);
  //std::cout << "dec[11] " << plaintext << std::endl;
}

template<AESTypes type>
block AESDec<type>::ecbDecBlock(const block& plaintext) {
  block ret;
  ecbDecBlock(plaintext, ret);
  return ret;
}

template<AESTypes type>
AESDec<type>::AESDec(const block& key) {
  setKey(key);
}

}
