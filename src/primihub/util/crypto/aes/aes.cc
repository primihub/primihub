// Copyright [2021] <primihub.com>
#include "src/primihub/util/crypto/aes/aes.h"

#ifdef OC_ENABLE_AESNI
#include <wmmintrin.h>
#elif !defined(OC_ENABLE_PORTABLE_AES)
static_assert(0, "OC_ENABLE_PORTABLE_AES must be defined if ENABLE_AESNI is not.");
#endif

namespace primihub {


template<AESTypes type>
AES<type>::AES(const block& userKey) {
  setKey(userKey);
}

template<AESTypes type>
void AES<type>::ecbEncBlock(const block& plaintext, block& ciphertext) const {
  //std::cout << (type == NI ?"NI":"Port") << "\n";
  //auto print = [](int i, block s, span<const block> k)
  //{
  //    std::cout << "enc " << i << " " << s << " " << k[i] << std::endl;
  //};
  //print(0, plaintext, mRoundKey);
  ciphertext = plaintext ^ mRoundKey[0];
  for (uint64_t i = 1; i < 10; ++i) {
    ///print(i, ciphertext, mRoundKey);
    ciphertext = roundEnc(ciphertext, mRoundKey[i]);
  }
  //print(10, ciphertext, mRoundKey);
  ciphertext = finalEnc(ciphertext, mRoundKey[10]);

//            std::cout << "enc 11 " << ciphertext << std::endl;
}

template<AESTypes type>
block AES<type>::ecbEncBlock(const block& plaintext) const {
  block ret;
  ecbEncBlock(plaintext, ret);
  return ret;
}

template<AESTypes type>
void AES<type>::ecbEncBlocks(const block* plaintexts, uint64_t blockLength, block* ciphertext) const {
  const uint64_t step = 8;
  uint64_t idx = 0;
  uint64_t length = blockLength - blockLength % step;

  for (; idx < length; idx += step) {
    ecbEnc8Blocks(plaintexts + idx, ciphertext + idx);
  }

  for (; idx < blockLength; ++idx) {
    ciphertext[idx] = ecbEncBlock(plaintexts[idx]);
  }
}

template<AESTypes type>
void AES<type>::ecbEncTwoBlocks(const block* plaintexts, block* ciphertext) const {
  ciphertext[0] = plaintexts[0] ^ mRoundKey[0];
  ciphertext[1] = plaintexts[1] ^ mRoundKey[0];

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[1]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[1]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[2]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[2]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[3]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[3]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[4]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[4]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[5]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[5]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[6]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[6]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[7]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[7]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[8]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[8]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[9]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[9]);

  ciphertext[0] = finalEnc(ciphertext[0], mRoundKey[10]);
  ciphertext[1] = finalEnc(ciphertext[1], mRoundKey[10]);
}

template<AESTypes type>
void AES<type>::ecbEncFourBlocks(const block* plaintexts, block* ciphertext) const {
  ciphertext[0] = plaintexts[0] ^ mRoundKey[0];
  ciphertext[1] = plaintexts[1] ^ mRoundKey[0];
  ciphertext[2] = plaintexts[2] ^ mRoundKey[0];
  ciphertext[3] = plaintexts[3] ^ mRoundKey[0];

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[1]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[1]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[1]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[1]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[2]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[2]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[2]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[2]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[3]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[3]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[3]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[3]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[4]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[4]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[4]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[4]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[5]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[5]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[5]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[5]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[6]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[6]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[6]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[6]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[7]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[7]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[7]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[7]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[8]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[8]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[8]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[8]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[9]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[9]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[9]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[9]);

  ciphertext[0] = finalEnc(ciphertext[0], mRoundKey[10]);
  ciphertext[1] = finalEnc(ciphertext[1], mRoundKey[10]);
  ciphertext[2] = finalEnc(ciphertext[2], mRoundKey[10]);
  ciphertext[3] = finalEnc(ciphertext[3], mRoundKey[10]);
}

// Encrypts 8 blocks pointer to by plaintexts and writes the result to ciphertext
template<AESTypes type>
void AES<type>::ecbEnc8Blocks(const block* plaintexts, block* ciphertext) const {
  block temp[8];

  temp[0] = plaintexts[0] ^ mRoundKey[0];
  temp[1] = plaintexts[1] ^ mRoundKey[0];
  temp[2] = plaintexts[2] ^ mRoundKey[0];
  temp[3] = plaintexts[3] ^ mRoundKey[0];
  temp[4] = plaintexts[4] ^ mRoundKey[0];
  temp[5] = plaintexts[5] ^ mRoundKey[0];
  temp[6] = plaintexts[6] ^ mRoundKey[0];
  temp[7] = plaintexts[7] ^ mRoundKey[0];

  temp[0] = roundEnc(temp[0], mRoundKey[1]);
  temp[1] = roundEnc(temp[1], mRoundKey[1]);
  temp[2] = roundEnc(temp[2], mRoundKey[1]);
  temp[3] = roundEnc(temp[3], mRoundKey[1]);
  temp[4] = roundEnc(temp[4], mRoundKey[1]);
  temp[5] = roundEnc(temp[5], mRoundKey[1]);
  temp[6] = roundEnc(temp[6], mRoundKey[1]);
  temp[7] = roundEnc(temp[7], mRoundKey[1]);

  temp[0] = roundEnc(temp[0], mRoundKey[2]);
  temp[1] = roundEnc(temp[1], mRoundKey[2]);
  temp[2] = roundEnc(temp[2], mRoundKey[2]);
  temp[3] = roundEnc(temp[3], mRoundKey[2]);
  temp[4] = roundEnc(temp[4], mRoundKey[2]);
  temp[5] = roundEnc(temp[5], mRoundKey[2]);
  temp[6] = roundEnc(temp[6], mRoundKey[2]);
  temp[7] = roundEnc(temp[7], mRoundKey[2]);

  temp[0] = roundEnc(temp[0], mRoundKey[3]);
  temp[1] = roundEnc(temp[1], mRoundKey[3]);
  temp[2] = roundEnc(temp[2], mRoundKey[3]);
  temp[3] = roundEnc(temp[3], mRoundKey[3]);
  temp[4] = roundEnc(temp[4], mRoundKey[3]);
  temp[5] = roundEnc(temp[5], mRoundKey[3]);
  temp[6] = roundEnc(temp[6], mRoundKey[3]);
  temp[7] = roundEnc(temp[7], mRoundKey[3]);

  temp[0] = roundEnc(temp[0], mRoundKey[4]);
  temp[1] = roundEnc(temp[1], mRoundKey[4]);
  temp[2] = roundEnc(temp[2], mRoundKey[4]);
  temp[3] = roundEnc(temp[3], mRoundKey[4]);
  temp[4] = roundEnc(temp[4], mRoundKey[4]);
  temp[5] = roundEnc(temp[5], mRoundKey[4]);
  temp[6] = roundEnc(temp[6], mRoundKey[4]);
  temp[7] = roundEnc(temp[7], mRoundKey[4]);

  temp[0] = roundEnc(temp[0], mRoundKey[5]);
  temp[1] = roundEnc(temp[1], mRoundKey[5]);
  temp[2] = roundEnc(temp[2], mRoundKey[5]);
  temp[3] = roundEnc(temp[3], mRoundKey[5]);
  temp[4] = roundEnc(temp[4], mRoundKey[5]);
  temp[5] = roundEnc(temp[5], mRoundKey[5]);
  temp[6] = roundEnc(temp[6], mRoundKey[5]);
  temp[7] = roundEnc(temp[7], mRoundKey[5]);

  temp[0] = roundEnc(temp[0], mRoundKey[6]);
  temp[1] = roundEnc(temp[1], mRoundKey[6]);
  temp[2] = roundEnc(temp[2], mRoundKey[6]);
  temp[3] = roundEnc(temp[3], mRoundKey[6]);
  temp[4] = roundEnc(temp[4], mRoundKey[6]);
  temp[5] = roundEnc(temp[5], mRoundKey[6]);
  temp[6] = roundEnc(temp[6], mRoundKey[6]);
  temp[7] = roundEnc(temp[7], mRoundKey[6]);

  temp[0] = roundEnc(temp[0], mRoundKey[7]);
  temp[1] = roundEnc(temp[1], mRoundKey[7]);
  temp[2] = roundEnc(temp[2], mRoundKey[7]);
  temp[3] = roundEnc(temp[3], mRoundKey[7]);
  temp[4] = roundEnc(temp[4], mRoundKey[7]);
  temp[5] = roundEnc(temp[5], mRoundKey[7]);
  temp[6] = roundEnc(temp[6], mRoundKey[7]);
  temp[7] = roundEnc(temp[7], mRoundKey[7]);

  temp[0] = roundEnc(temp[0], mRoundKey[8]);
  temp[1] = roundEnc(temp[1], mRoundKey[8]);
  temp[2] = roundEnc(temp[2], mRoundKey[8]);
  temp[3] = roundEnc(temp[3], mRoundKey[8]);
  temp[4] = roundEnc(temp[4], mRoundKey[8]);
  temp[5] = roundEnc(temp[5], mRoundKey[8]);
  temp[6] = roundEnc(temp[6], mRoundKey[8]);
  temp[7] = roundEnc(temp[7], mRoundKey[8]);

  temp[0] = roundEnc(temp[0], mRoundKey[9]);
  temp[1] = roundEnc(temp[1], mRoundKey[9]);
  temp[2] = roundEnc(temp[2], mRoundKey[9]);
  temp[3] = roundEnc(temp[3], mRoundKey[9]);
  temp[4] = roundEnc(temp[4], mRoundKey[9]);
  temp[5] = roundEnc(temp[5], mRoundKey[9]);
  temp[6] = roundEnc(temp[6], mRoundKey[9]);
  temp[7] = roundEnc(temp[7], mRoundKey[9]);

  ciphertext[0] = finalEnc(temp[0], mRoundKey[10]);
  ciphertext[1] = finalEnc(temp[1], mRoundKey[10]);
  ciphertext[2] = finalEnc(temp[2], mRoundKey[10]);
  ciphertext[3] = finalEnc(temp[3], mRoundKey[10]);
  ciphertext[4] = finalEnc(temp[4], mRoundKey[10]);
  ciphertext[5] = finalEnc(temp[5], mRoundKey[10]);
  ciphertext[6] = finalEnc(temp[6], mRoundKey[10]);
  ciphertext[7] = finalEnc(temp[7], mRoundKey[10]);
}

template<AESTypes type>
void AES<type>::ecbEnc16Blocks(const block* plaintexts, block* ciphertext) const {
  ciphertext[0] = plaintexts[0] ^ mRoundKey[0];
  ciphertext[1] = plaintexts[1] ^ mRoundKey[0];
  ciphertext[2] = plaintexts[2] ^ mRoundKey[0];
  ciphertext[3] = plaintexts[3] ^ mRoundKey[0];
  ciphertext[4] = plaintexts[4] ^ mRoundKey[0];
  ciphertext[5] = plaintexts[5] ^ mRoundKey[0];
  ciphertext[6] = plaintexts[6] ^ mRoundKey[0];
  ciphertext[7] = plaintexts[7] ^ mRoundKey[0];
  ciphertext[8] = plaintexts[8] ^ mRoundKey[0];
  ciphertext[9] = plaintexts[9] ^ mRoundKey[0];
  ciphertext[10] = plaintexts[10] ^ mRoundKey[0];
  ciphertext[11] = plaintexts[11] ^ mRoundKey[0];
  ciphertext[12] = plaintexts[12] ^ mRoundKey[0];
  ciphertext[13] = plaintexts[13] ^ mRoundKey[0];
  ciphertext[14] = plaintexts[14] ^ mRoundKey[0];
  ciphertext[15] = plaintexts[15] ^ mRoundKey[0];

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[1]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[1]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[1]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[1]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[1]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[1]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[1]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[1]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[1]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[1]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[1]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[1]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[1]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[1]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[1]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[1]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[2]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[2]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[2]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[2]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[2]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[2]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[2]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[2]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[2]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[2]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[2]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[2]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[2]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[2]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[2]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[2]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[3]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[3]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[3]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[3]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[3]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[3]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[3]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[3]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[3]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[3]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[3]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[3]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[3]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[3]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[3]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[3]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[4]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[4]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[4]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[4]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[4]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[4]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[4]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[4]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[4]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[4]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[4]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[4]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[4]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[4]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[4]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[4]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[5]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[5]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[5]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[5]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[5]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[5]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[5]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[5]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[5]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[5]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[5]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[5]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[5]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[5]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[5]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[5]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[6]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[6]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[6]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[6]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[6]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[6]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[6]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[6]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[6]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[6]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[6]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[6]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[6]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[6]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[6]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[6]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[7]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[7]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[7]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[7]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[7]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[7]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[7]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[7]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[7]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[7]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[7]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[7]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[7]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[7]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[7]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[7]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[8]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[8]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[8]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[8]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[8]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[8]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[8]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[8]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[8]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[8]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[8]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[8]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[8]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[8]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[8]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[8]);

  ciphertext[0] = roundEnc(ciphertext[0], mRoundKey[9]);
  ciphertext[1] = roundEnc(ciphertext[1], mRoundKey[9]);
  ciphertext[2] = roundEnc(ciphertext[2], mRoundKey[9]);
  ciphertext[3] = roundEnc(ciphertext[3], mRoundKey[9]);
  ciphertext[4] = roundEnc(ciphertext[4], mRoundKey[9]);
  ciphertext[5] = roundEnc(ciphertext[5], mRoundKey[9]);
  ciphertext[6] = roundEnc(ciphertext[6], mRoundKey[9]);
  ciphertext[7] = roundEnc(ciphertext[7], mRoundKey[9]);
  ciphertext[8] = roundEnc(ciphertext[8], mRoundKey[9]);
  ciphertext[9] = roundEnc(ciphertext[9], mRoundKey[9]);
  ciphertext[10] = roundEnc(ciphertext[10], mRoundKey[9]);
  ciphertext[11] = roundEnc(ciphertext[11], mRoundKey[9]);
  ciphertext[12] = roundEnc(ciphertext[12], mRoundKey[9]);
  ciphertext[13] = roundEnc(ciphertext[13], mRoundKey[9]);
  ciphertext[14] = roundEnc(ciphertext[14], mRoundKey[9]);
  ciphertext[15] = roundEnc(ciphertext[15], mRoundKey[9]);

  ciphertext[0] = finalEnc(ciphertext[0], mRoundKey[10]);
  ciphertext[1] = finalEnc(ciphertext[1], mRoundKey[10]);
  ciphertext[2] = finalEnc(ciphertext[2], mRoundKey[10]);
  ciphertext[3] = finalEnc(ciphertext[3], mRoundKey[10]);
  ciphertext[4] = finalEnc(ciphertext[4], mRoundKey[10]);
  ciphertext[5] = finalEnc(ciphertext[5], mRoundKey[10]);
  ciphertext[6] = finalEnc(ciphertext[6], mRoundKey[10]);
  ciphertext[7] = finalEnc(ciphertext[7], mRoundKey[10]);
  ciphertext[8] = finalEnc(ciphertext[8], mRoundKey[10]);
  ciphertext[9] = finalEnc(ciphertext[9], mRoundKey[10]);
  ciphertext[10] = finalEnc(ciphertext[10], mRoundKey[10]);
  ciphertext[11] = finalEnc(ciphertext[11], mRoundKey[10]);
  ciphertext[12] = finalEnc(ciphertext[12], mRoundKey[10]);
  ciphertext[13] = finalEnc(ciphertext[13], mRoundKey[10]);
  ciphertext[14] = finalEnc(ciphertext[14], mRoundKey[10]);
  ciphertext[15] = finalEnc(ciphertext[15], mRoundKey[10]);
}

template<AESTypes type>
void AES<type>::ecbEncCounterMode(block baseIdx, uint64_t blockLength, block* ciphertext) const {
  const int32_t step = 8;
  int32_t idx = 0;
  int32_t length = int32_t(blockLength - blockLength % step);
  const auto b0 = toBlock(0,0);
  const auto b1 = toBlock(1ull);
  const auto b2 = toBlock(2ull);
  const auto b3 = toBlock(3ull);
  const auto b4 = toBlock(4ull);
  const auto b5 = toBlock(5ull);
  const auto b6 = toBlock(6ull);
  const auto b7 = toBlock(7ull);

  block temp[step];
  for (; idx < length; idx += step) {
    temp[0] = (baseIdx + b0) ^ mRoundKey[0];
    temp[1] = (baseIdx + b1) ^ mRoundKey[0];
    temp[2] = (baseIdx + b2) ^ mRoundKey[0];
    temp[3] = (baseIdx + b3) ^ mRoundKey[0];
    temp[4] = (baseIdx + b4) ^ mRoundKey[0];
    temp[5] = (baseIdx + b5) ^ mRoundKey[0];
    temp[6] = (baseIdx + b6) ^ mRoundKey[0];
    temp[7] = (baseIdx + b7) ^ mRoundKey[0];
    baseIdx = baseIdx + toBlock(step);

    temp[0] = roundEnc(temp[0], mRoundKey[1]);
    temp[1] = roundEnc(temp[1], mRoundKey[1]);
    temp[2] = roundEnc(temp[2], mRoundKey[1]);
    temp[3] = roundEnc(temp[3], mRoundKey[1]);
    temp[4] = roundEnc(temp[4], mRoundKey[1]);
    temp[5] = roundEnc(temp[5], mRoundKey[1]);
    temp[6] = roundEnc(temp[6], mRoundKey[1]);
    temp[7] = roundEnc(temp[7], mRoundKey[1]);

    temp[0] = roundEnc(temp[0], mRoundKey[2]);
    temp[1] = roundEnc(temp[1], mRoundKey[2]);
    temp[2] = roundEnc(temp[2], mRoundKey[2]);
    temp[3] = roundEnc(temp[3], mRoundKey[2]);
    temp[4] = roundEnc(temp[4], mRoundKey[2]);
    temp[5] = roundEnc(temp[5], mRoundKey[2]);
    temp[6] = roundEnc(temp[6], mRoundKey[2]);
    temp[7] = roundEnc(temp[7], mRoundKey[2]);

    temp[0] = roundEnc(temp[0], mRoundKey[3]);
    temp[1] = roundEnc(temp[1], mRoundKey[3]);
    temp[2] = roundEnc(temp[2], mRoundKey[3]);
    temp[3] = roundEnc(temp[3], mRoundKey[3]);
    temp[4] = roundEnc(temp[4], mRoundKey[3]);
    temp[5] = roundEnc(temp[5], mRoundKey[3]);
    temp[6] = roundEnc(temp[6], mRoundKey[3]);
    temp[7] = roundEnc(temp[7], mRoundKey[3]);

    temp[0] = roundEnc(temp[0], mRoundKey[4]);
    temp[1] = roundEnc(temp[1], mRoundKey[4]);
    temp[2] = roundEnc(temp[2], mRoundKey[4]);
    temp[3] = roundEnc(temp[3], mRoundKey[4]);
    temp[4] = roundEnc(temp[4], mRoundKey[4]);
    temp[5] = roundEnc(temp[5], mRoundKey[4]);
    temp[6] = roundEnc(temp[6], mRoundKey[4]);
    temp[7] = roundEnc(temp[7], mRoundKey[4]);

    temp[0] = roundEnc(temp[0], mRoundKey[5]);
    temp[1] = roundEnc(temp[1], mRoundKey[5]);
    temp[2] = roundEnc(temp[2], mRoundKey[5]);
    temp[3] = roundEnc(temp[3], mRoundKey[5]);
    temp[4] = roundEnc(temp[4], mRoundKey[5]);
    temp[5] = roundEnc(temp[5], mRoundKey[5]);
    temp[6] = roundEnc(temp[6], mRoundKey[5]);
    temp[7] = roundEnc(temp[7], mRoundKey[5]);

    temp[0] = roundEnc(temp[0], mRoundKey[6]);
    temp[1] = roundEnc(temp[1], mRoundKey[6]);
    temp[2] = roundEnc(temp[2], mRoundKey[6]);
    temp[3] = roundEnc(temp[3], mRoundKey[6]);
    temp[4] = roundEnc(temp[4], mRoundKey[6]);
    temp[5] = roundEnc(temp[5], mRoundKey[6]);
    temp[6] = roundEnc(temp[6], mRoundKey[6]);
    temp[7] = roundEnc(temp[7], mRoundKey[6]);

    temp[0] = roundEnc(temp[0], mRoundKey[7]);
    temp[1] = roundEnc(temp[1], mRoundKey[7]);
    temp[2] = roundEnc(temp[2], mRoundKey[7]);
    temp[3] = roundEnc(temp[3], mRoundKey[7]);
    temp[4] = roundEnc(temp[4], mRoundKey[7]);
    temp[5] = roundEnc(temp[5], mRoundKey[7]);
    temp[6] = roundEnc(temp[6], mRoundKey[7]);
    temp[7] = roundEnc(temp[7], mRoundKey[7]);

    temp[0] = roundEnc(temp[0], mRoundKey[8]);
    temp[1] = roundEnc(temp[1], mRoundKey[8]);
    temp[2] = roundEnc(temp[2], mRoundKey[8]);
    temp[3] = roundEnc(temp[3], mRoundKey[8]);
    temp[4] = roundEnc(temp[4], mRoundKey[8]);
    temp[5] = roundEnc(temp[5], mRoundKey[8]);
    temp[6] = roundEnc(temp[6], mRoundKey[8]);
    temp[7] = roundEnc(temp[7], mRoundKey[8]);

    temp[0] = roundEnc(temp[0], mRoundKey[9]);
    temp[1] = roundEnc(temp[1], mRoundKey[9]);
    temp[2] = roundEnc(temp[2], mRoundKey[9]);
    temp[3] = roundEnc(temp[3], mRoundKey[9]);
    temp[4] = roundEnc(temp[4], mRoundKey[9]);
    temp[5] = roundEnc(temp[5], mRoundKey[9]);
    temp[6] = roundEnc(temp[6], mRoundKey[9]);
    temp[7] = roundEnc(temp[7], mRoundKey[9]);

    temp[0] = finalEnc(temp[0], mRoundKey[10]);
    temp[1] = finalEnc(temp[1], mRoundKey[10]);
    temp[2] = finalEnc(temp[2], mRoundKey[10]);
    temp[3] = finalEnc(temp[3], mRoundKey[10]);
    temp[4] = finalEnc(temp[4], mRoundKey[10]);
    temp[5] = finalEnc(temp[5], mRoundKey[10]);
    temp[6] = finalEnc(temp[6], mRoundKey[10]);
    temp[7] = finalEnc(temp[7], mRoundKey[10]);

    memcpy((u8*)(ciphertext + idx), temp, sizeof(temp));
  }

  for (; idx < static_cast<int32_t>(blockLength); ++idx) {
    auto temp = baseIdx ^ mRoundKey[0];
    baseIdx = baseIdx + toBlock(1);
    temp = roundEnc(temp, mRoundKey[1]);
    temp = roundEnc(temp, mRoundKey[2]);
    temp = roundEnc(temp, mRoundKey[3]);
    temp = roundEnc(temp, mRoundKey[4]);
    temp = roundEnc(temp, mRoundKey[5]);
    temp = roundEnc(temp, mRoundKey[6]);
    temp = roundEnc(temp, mRoundKey[7]);
    temp = roundEnc(temp, mRoundKey[8]);
    temp = roundEnc(temp, mRoundKey[9]);
    temp = finalEnc(temp, mRoundKey[10]);

    memcpy((u8*)(ciphertext + idx), &temp, sizeof(temp));
  }
}

#ifdef OC_ENABLE_PORTABLE_AES

// The number of columns comprising a state in AES. This is a constant in AES. Value=4
const uint64_t Nb = 4;
const uint64_t Nk = 4;        // The number of 32 bit words in a key.
const uint64_t Nr = 10;       // The number of rounds in AES Cipher.

template<>
void AES<Portable>::setKey(const block& userKey) {
  // This function produces 4(4+1) round keys. The round keys are used in each round to decrypt the states. 
  auto RoundKey = (u8*)mRoundKey.data();
  auto Key = (const u8*)&userKey;

  unsigned i, j, k;
  uint8_t tempa[4]; // Used for the column/row operations

  // The first round key is the key itself.
  for (i = 0; i < 4; ++i) {
    RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
    RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
    RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
    RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
  }

  // All other round keys are found from the previous round keys.
  for (i = 4; i < 4 * (11); ++i) {
    {
      k = (i - 1) * 4;
      tempa[0] = RoundKey[k + 0];
      tempa[1] = RoundKey[k + 1];
      tempa[2] = RoundKey[k + 2];
      tempa[3] = RoundKey[k + 3];

    }

    if (i % 4 == 0) {
      // This function shifts the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

      // Function RotWord()
      {
        const uint8_t u8tmp = tempa[0];
        tempa[0] = tempa[1];
        tempa[1] = tempa[2];
        tempa[2] = tempa[3];
        tempa[3] = u8tmp;
      }

      // SubWord() is a function that takes a four-byte input word and 
      // applies the S-box to each of the four bytes to produce an output word.

      // Function Subword()
      {
        tempa[0] = getSBoxValue(tempa[0]);
        tempa[1] = getSBoxValue(tempa[1]);
        tempa[2] = getSBoxValue(tempa[2]);
        tempa[3] = getSBoxValue(tempa[3]);
      }

      tempa[0] = tempa[0] ^ Rcon[i / 4];
    }

    j = i * 4; k = (i - 4) * 4;
    RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
    RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
    RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
    RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
  }
}

template<>
block AES<Portable>::roundEnc(block state, const block& roundKey) {
  SubBytes(state);
  ShiftRows(state);
  MixColumns(state);
  state = state ^ roundKey;
  return state;
}

template<>
block AES<Portable>::finalEnc(block state, const block& roundKey) {
  SubBytes(state);
  ShiftRows(state);
  state = state ^ roundKey;
  return state;
}
#endif

#ifdef OC_ENABLE_PORTABLE_AES
template class AES<Portable>;
#endif

#ifdef OC_ENABLE_AESNI
template class AES<NI>;
#endif

const AES_Type mAesFixedKey(toBlock(45345336, -103343430));

block PRF(const block& b, u64 i) {
  return AES_Type(b).ecbEncBlock(toBlock(i));
}

}
