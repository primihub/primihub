// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_CRYPTO_AES_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_CRYPTO_AES_H_

#include <array>
#include <cstring>
#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/block.h"

namespace primihub {

enum AESTypes {
  NI,
  Portable
};

template<AESTypes types>
class AES {
 public:
  // Default constructor leave the class in an invalid state
  // until setKey(...) is called.
  AES() = default;
  AES(const AES&) = default;

  // Constructor to initialize the class with the given key
  AES(const block& userKey);

  // Set the key to be used for encryption.
  void setKey(const block& userKey);

  // Encrypts the plaintext block and stores the result in ciphertext
  void ecbEncBlock(const block& plaintext, block& ciphertext) const;

  // Encrypts the plaintext block and returns the result 
  block ecbEncBlock(const block& plaintext) const;

  // Encrypts blockLength starting at the plaintexts pointer and writes the result
  // to the ciphertext pointer
  void ecbEncBlocks(const block* plaintexts, uint64_t blockLength, block* ciphertext) const;

  void ecbEncBlocks(span<const block> plaintexts, span<block> ciphertext) const {
    if (plaintexts.size() != ciphertext.size())
      throw RTE_LOC;
    ecbEncBlocks(plaintexts.data(), plaintexts.size(), ciphertext.data());
  }

  // Encrypts 2 blocks pointer to by plaintexts and writes the result to ciphertext
  void ecbEncTwoBlocks(const block* plaintexts, block* ciphertext) const;

  // Encrypts 4 blocks pointer to by plaintexts and writes the result to ciphertext
  void ecbEncFourBlocks(const block* plaintexts, block* ciphertext) const;

  // Encrypts 8 blocks pointer to by plaintexts and writes the result to ciphertext
  void ecbEnc8Blocks(const block* plaintexts, block* ciphertext) const;

  inline block hashBlock(const block& plaintexts) const {
    return ecbEncBlock(plaintexts) ^ plaintexts;
  }

  inline void hash8Blocks(const block* plaintexts, block* ciphertext) const {
    std::array<block, 8> buff;
    ecbEnc8Blocks(plaintexts, buff.data());
    ciphertext[0] = plaintexts[0] ^ buff[0];
    ciphertext[1] = plaintexts[1] ^ buff[1];
    ciphertext[2] = plaintexts[2] ^ buff[2];
    ciphertext[3] = plaintexts[3] ^ buff[3];
    ciphertext[4] = plaintexts[4] ^ buff[4];
    ciphertext[5] = plaintexts[5] ^ buff[5];
    ciphertext[6] = plaintexts[6] ^ buff[6];
    ciphertext[7] = plaintexts[7] ^ buff[7];
  }

  inline void hashBlocks(span<const block> plaintexts, span<block> ciphertext) const {
    //assert(plaintexts.size() == ciphertext.size());
    auto main = uint64_t(plaintexts.size() / 8) * 8;
    uint64_t i = 0;
    auto o = ciphertext.data();
    auto p = plaintexts.data();
    for (; i < main; i += 8) {
      hash8Blocks(p + i, o + i);
    }

    for (; i < uint64_t(ciphertext.size()); ++i) {
      o[i] = hashBlock(p[i]);
    }
  }

  // Encrypts 16 blocks pointer to by plaintexts and writes the result to ciphertext
  void ecbEnc16Blocks(const block* plaintexts, block* ciphertext) const;

  // Encrypts the vector of blocks {baseIdx, baseIdx + 1, ..., baseIdx + length - 1} 
  // and writes the result to ciphertext.
  void ecbEncCounterMode(uint64_t baseIdx, uint64_t length, block* ciphertext) const {
    ecbEncCounterMode(toBlock(baseIdx), length, ciphertext);
  }

  void ecbEncCounterMode(uint64_t baseIdx, span<block> ciphertext) const {
    ecbEncCounterMode(toBlock(baseIdx), ciphertext.size(), ciphertext.data());
  }

  void ecbEncCounterMode(block baseIdx, span<block> ciphertext) const {
    ecbEncCounterMode(baseIdx, ciphertext.size(), ciphertext.data());
  }

  void ecbEncCounterMode(block baseIdx, uint64_t length, block* ciphertext) const;

  // Returns the current key.
  const block& getKey() const { return mRoundKey[0]; }

  static block roundEnc(block state, const block& roundKey);
  static block finalEnc(block state, const block& roundKey);

  // The expanded key.
  std::array<block,11> mRoundKey;
};


#ifdef OC_ENABLE_PORTABLE_AES

// The lookup-tables are marked const so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM - 
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
static const u8 sbox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const u8 rsbox[256] = {
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

inline u8 getSBoxValue(int num) {
  return sbox[num];
}

inline u8 getSBoxInvert(int num) {
  return rsbox[num];
}

static const uint8_t Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};
#endif

#ifdef OC_ENABLE_PORTABLE_AES
inline std::array<std::array<u8, 4>, 4>& stateView(block& state) {
  return *(std::array<std::array<u8, 4>, 4>*) & state;
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
inline void SubBytes(block& state_) {
  u8* state = (u8*)&state_;
  for (uint64_t i = 0; i < 16; ++i)
    state[i] = getSBoxValue(state[i]);
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
inline void ShiftRows(block& state_) {
  uint8_t temp;
  auto& state = stateView(state_);

  // Rotate first row 1 columns to left  
  temp = state[0][1];
  state[0][1] = state[1][1];
  state[1][1] = state[2][1];
  state[2][1] = state[3][1];
  state[3][1] = temp;

  // Rotate second row 2 columns to left  
  temp = state[0][2];
  state[0][2] = state[2][2];
  state[2][2] = temp;

  temp = state[1][2];
  state[1][2] = state[3][2];
  state[3][2] = temp;

  // Rotate third row 3 columns to left
  temp = state[0][3];
  state[0][3] = state[3][3];
  state[3][3] = state[2][3];
  state[2][3] = state[1][3];
  state[1][3] = temp;
}

inline uint8_t xtime(uint8_t x) {
  return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

inline uint8_t Multiply(uint8_t x, uint8_t y) {
  return (((y & 1) * x) ^
    ((y >> 1 & 1)* xtime(x)) ^
    ((y >> 2 & 1)* xtime(xtime(x))) ^
    ((y >> 3 & 1)* xtime(xtime(xtime(x)))) ^
    ((y >> 4 & 1)* xtime(xtime(xtime(xtime(x)))))); /* this last call to xtime() can be omitted */
}

// MixColumns function mixes the columns of the state matrix
inline void MixColumns(block& state_) {
  auto& state = stateView(state_);
  uint8_t i;
  uint8_t Tmp, Tm, t;
  for (i = 0; i < 4; ++i) {
    t = state[i][0];
    Tmp = state[i][0] ^ state[i][1] ^ state[i][2] ^ state[i][3];
    Tm = state[i][0] ^ state[i][1];
    Tm = xtime(Tm);
    state[i][0] ^= Tm ^ Tmp;
    Tm = state[i][1] ^ state[i][2];
    Tm = xtime(Tm);
    state[i][1] ^= Tm ^ Tmp;
    Tm = state[i][2] ^ state[i][3];
    Tm = xtime(Tm);
    state[i][2] ^= Tm ^ Tmp;
    Tm = state[i][3] ^ t;
    Tm = xtime(Tm);
    state[i][3] ^= Tm ^ Tmp;
  }
}
#endif


#ifdef OC_ENABLE_AESNI
block keyGenHelper(block key, block keyRcon) {
  keyRcon = _mm_shuffle_epint32_t(keyRcon, _MM_SHUFFLE(3, 3, 3, 3));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, keyRcon);
};

template<>
void AES<NI>::setKey(const block& userKey) {
  mRoundKey[0] = userKey;
  mRoundKey[1] = keyGenHelper(mRoundKey[0], _mm_aeskeygenassist_si128(mRoundKey[0], 0x01));
  mRoundKey[2] = keyGenHelper(mRoundKey[1], _mm_aeskeygenassist_si128(mRoundKey[1], 0x02));
  mRoundKey[3] = keyGenHelper(mRoundKey[2], _mm_aeskeygenassist_si128(mRoundKey[2], 0x04));
  mRoundKey[4] = keyGenHelper(mRoundKey[3], _mm_aeskeygenassist_si128(mRoundKey[3], 0x08));
  mRoundKey[5] = keyGenHelper(mRoundKey[4], _mm_aeskeygenassist_si128(mRoundKey[4], 0x10));
  mRoundKey[6] = keyGenHelper(mRoundKey[5], _mm_aeskeygenassist_si128(mRoundKey[5], 0x20));
  mRoundKey[7] = keyGenHelper(mRoundKey[6], _mm_aeskeygenassist_si128(mRoundKey[6], 0x40));
  mRoundKey[8] = keyGenHelper(mRoundKey[7], _mm_aeskeygenassist_si128(mRoundKey[7], 0x80));
  mRoundKey[9] = keyGenHelper(mRoundKey[8], _mm_aeskeygenassist_si128(mRoundKey[8], 0x1B));
  mRoundKey[10] = keyGenHelper(mRoundKey[9], _mm_aeskeygenassist_si128(mRoundKey[9], 0x36));
}
#endif

#ifdef OC_ENABLE_AESNI
template<>
inline block AES<NI>::finalEnc(block state, const block& roundKey) {
  return _mm_aesenclast_si128(state, roundKey);
}

template<>
inline block AES<NI>::roundEnc(block state, const block& roundKey) {
  return _mm_aesenc_si128(state, roundKey);
}
#endif

// An AES instance with a fixed and public key.
// extern const AES mAesFixedKey;

#ifdef OC_ENABLE_AESNI
using AES_Type = AES<NI>;
#else
using AES_Type = AES<Portable>;
#endif

extern const AES_Type mAesFixedKey;

block PRF(const block& b, u64 i);

}

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_CRYPTO_AES_H_
