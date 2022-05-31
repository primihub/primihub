// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_CRYPTO_AES_MULTIKEYAES_H_
#define SRC_PRIMIHUB_UTIL_CRYPTO_AES_MULTIKEYAES_H_

#include <array>
#include <cstring>
#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/block.h"
#include "src/primihub/util/crypto/aes/aes.h"

namespace primihub {

// Specialization of the AES class to support encryption of N values under N different keys
template<int N>
class MultiKeyAES {
 public:
  std::array<AES_Type, N> mAESs;

  // Default constructor leave the class in an invalid state
  // until setKey(...) is called.
  MultiKeyAES() = default;

  // Constructor to initialize the class with the given key
  MultiKeyAES(span<block> keys) {
    setKeys(keys);
  }
#ifdef OC_ENABLE_AESNI

  template<int SS>
  void keyGenHelper8(std::array<block,8>& key) {
    std::array<block, 8> keyRcon, t, p;
    keyRcon[0] = _mm_aeskeygenassist_si128(key[0], SS);
    keyRcon[1] = _mm_aeskeygenassist_si128(key[1], SS);
    keyRcon[2] = _mm_aeskeygenassist_si128(key[2], SS);
    keyRcon[3] = _mm_aeskeygenassist_si128(key[3], SS);
    keyRcon[4] = _mm_aeskeygenassist_si128(key[4], SS);
    keyRcon[5] = _mm_aeskeygenassist_si128(key[5], SS);
    keyRcon[6] = _mm_aeskeygenassist_si128(key[6], SS);
    keyRcon[7] = _mm_aeskeygenassist_si128(key[7], SS);

    keyRcon[0] = _mm_shuffle_epi32(keyRcon[0], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[1] = _mm_shuffle_epi32(keyRcon[1], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[2] = _mm_shuffle_epi32(keyRcon[2], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[3] = _mm_shuffle_epi32(keyRcon[3], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[4] = _mm_shuffle_epi32(keyRcon[4], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[5] = _mm_shuffle_epi32(keyRcon[5], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[6] = _mm_shuffle_epi32(keyRcon[6], _MM_SHUFFLE(3, 3, 3, 3));
    keyRcon[7] = _mm_shuffle_epi32(keyRcon[7], _MM_SHUFFLE(3, 3, 3, 3));

    p[0] = key[0];
    p[1] = key[1];
    p[2] = key[2];
    p[3] = key[3];
    p[4] = key[4];
    p[5] = key[5];
    p[6] = key[6];
    p[7] = key[7];

    for (uint64_t i = 0; i < 3; ++i) {
      t[0] = _mm_slli_si128(p[0], 4);
      t[1] = _mm_slli_si128(p[1], 4);
      t[2] = _mm_slli_si128(p[2], 4);
      t[3] = _mm_slli_si128(p[3], 4);
      t[4] = _mm_slli_si128(p[4], 4);
      t[5] = _mm_slli_si128(p[5], 4);
      t[6] = _mm_slli_si128(p[6], 4);
      t[7] = _mm_slli_si128(p[7], 4);

      p[0] = _mm_xor_si128(p[0], t[0]);
      p[1] = _mm_xor_si128(p[1], t[1]);
      p[2] = _mm_xor_si128(p[2], t[2]);
      p[3] = _mm_xor_si128(p[3], t[3]);
      p[4] = _mm_xor_si128(p[4], t[4]);
      p[5] = _mm_xor_si128(p[5], t[5]);
      p[6] = _mm_xor_si128(p[6], t[6]);
      p[7] = _mm_xor_si128(p[7], t[7]);
    }

    key[0] = _mm_xor_si128(p[0], keyRcon[0]);
    key[1] = _mm_xor_si128(p[1], keyRcon[1]);
    key[2] = _mm_xor_si128(p[2], keyRcon[2]);
    key[3] = _mm_xor_si128(p[3], keyRcon[3]);
    key[4] = _mm_xor_si128(p[4], keyRcon[4]);
    key[5] = _mm_xor_si128(p[5], keyRcon[5]);
    key[6] = _mm_xor_si128(p[6], keyRcon[6]);
    key[7] = _mm_xor_si128(p[7], keyRcon[7]);
  };
#endif

  // Set the N keys to be used for encryption.
  void setKeys(span<block> keys) {
#ifdef OC_ENABLE_AESNI
    constexpr uint64_t main = N / 8 * 8;

    auto cp = [&](u8 i, AES* aes, std::array<block, 8>& buff) {
      aes[0].mRoundKey[i] = buff[0];
      aes[1].mRoundKey[i] = buff[1];
      aes[2].mRoundKey[i] = buff[2];
      aes[3].mRoundKey[i] = buff[3];
      aes[4].mRoundKey[i] = buff[4];
      aes[5].mRoundKey[i] = buff[5];
      aes[6].mRoundKey[i] = buff[6];
      aes[7].mRoundKey[i] = buff[7];
    };
    std::array<block, 8> buff;

    for (uint64_t i = 0; i < main; i += 8) {
      auto* aes = &mAESs[i];

      buff[0] = keys[i + 0];
      buff[1] = keys[i + 1];
      buff[2] = keys[i + 2];
      buff[3] = keys[i + 3];
      buff[4] = keys[i + 4];
      buff[5] = keys[i + 5];
      buff[6] = keys[i + 6];
      buff[7] = keys[i + 7];
      cp(0, aes, buff);

      keyGenHelper8<0x01>(buff);
      cp(1, aes, buff);
      keyGenHelper8<0x02>(buff);
      cp(2, aes, buff);
      keyGenHelper8<0x04>(buff);
      cp(3, aes, buff);
      keyGenHelper8<0x08>(buff);
      cp(4, aes, buff);
      keyGenHelper8<0x10>(buff);
      cp(5, aes, buff);
      keyGenHelper8<0x20>(buff);
      cp(6, aes, buff);
      keyGenHelper8<0x40>(buff);
      cp(7, aes, buff);
      keyGenHelper8<0x80>(buff);
      cp(8, aes, buff);
      keyGenHelper8<0x1B>(buff);
      cp(9, aes, buff);
      keyGenHelper8<0x36>(buff);
      cp(10, aes, buff);
    }
#else
    uint64_t main = 0;
#endif  
    for (uint64_t i = main; i < N; ++i) {
      mAESs[i].setKey(keys[i]);
    }
  }

  // Computes the encrpytion of N blocks pointed to by plaintext 
  // and stores the result at ciphertext.
  void ecbEncNBlocks(const block* plaintext, block* ciphertext) const {
    for (int i = 0; i < N; ++i)
      ciphertext[i] = plaintext[i] ^ mAESs[i].mRoundKey[0];
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[1]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[2]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[3]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[4]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[5]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[6]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[7]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[8]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::roundEnc(ciphertext[i], mAESs[i].mRoundKey[9]);
    
    for (int i = 0; i < N; ++i)
      ciphertext[i] = AES::finalEnc(ciphertext[i], mAESs[i].mRoundKey[10]);
  }

  // Computes the hash of N blocks pointed to by plaintext 
  // and stores the result at ciphertext.
  void hashNBlocks(const block* plaintext, block* hashes) const {
    std::array<block, N> buff;
    for (int i = 0; i < N; ++i)
      buff[i] = plaintext[i];
    //memcpy(buff.data(), plaintext, 16 * N);
    ecbEncNBlocks(buff.data(), buff.data());
    for (int i = 0; i < N; ++i)
      hashes[i] = buff[i] ^ plaintext[i];
  }

  // Utility to compare the keys.
  const MultiKeyAES<N>& operator=(const MultiKeyAES<N>& rhs) {
    for (uint64_t i = 0; i < N; ++i)
      for (uint64_t j = 0; j < 11; ++j)
        mAESs[i].mRoundKey[j] = rhs.mAESs[i].mRoundKey[j];

    return rhs;
  }
};

}

#endif  // SRC_PRIMIHUB_UTIL_CRYPTO_AES_MULTIKEYAES_H_
