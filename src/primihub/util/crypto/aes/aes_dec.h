// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_CRYPTO_AES_AESDEC_H_
#define SRC_PRIMIHUB_UTIL_CRYPTO_AES_AESDEC_H_

#include <array>
#include <cstring>
#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/block.h"
#include "src/primihub/util/crypto/aes/aes.h"

namespace primihub {

// A class to perform AES decryption.
template<AESTypes type>
class AESDec {
 public:
  AESDec() = default;
  AESDec(const AESDec&) = default;
  AESDec(const block& userKey);

  void setKey(const block& userKey);
  void ecbDecBlock(const block& ciphertext, block& plaintext);
  block ecbDecBlock(const block& ciphertext);

  std::array<block,11> mRoundKey;

  static block roundDec(block state, const block& roundKey);
  static block finalDec(block state, const block& roundKey);
};
//void InvCipher(block& state, std::array<block, 11>& RoundKey);


#ifdef OC_ENABLE_PORTABLE_AES

// MixColumns function mixes the columns of the state matrix.
// The method used to multiply may be difficult to understand for the inexperienced.
// Please use the references to gain more information.
static void InvMixColumns(block& state_) {
  auto& state = stateView(state_);
  int i;
  uint8_t a, b, c, d;
  for (i = 0; i < 4; ++i) {
    a = state[i][0];
    b = state[i][1];
    c = state[i][2];
    d = state[i][3];

    state[i][0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
    state[i][1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
    state[i][2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
    state[i][3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
  }
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void InvSubBytes(block& state_) {
  u8* state = (u8*)&state_;
  for (auto i = 0; i < 16; ++i)
    state[i] = getSBoxInvert(state[i]);
}

static void InvShiftRows(block& state_) {
  uint8_t temp;
  auto& state = stateView(state_);

  // Rotate first row 1 columns to right  
  temp = state[3][1];
  state[3][1] = state[2][1];
  state[2][1] = state[1][1];
  state[1][1] = state[0][1];
  state[0][1] = temp;

  // Rotate second row 2 columns to right 
  temp = state[0][2];
  state[0][2] = state[2][2];
  state[2][2] = temp;

  temp = state[1][2];
  state[1][2] = state[3][2];
  state[3][2] = temp;

  // Rotate third row 3 columns to right
  temp = state[0][3];
  state[0][3] = state[1][3];
  state[1][3] = state[2][3];
  state[2][3] = state[3][3];
  state[3][3] = temp;
}
#endif


#ifdef OC_ENABLE_AESNI
using AESDec_Type = AESDec<NI>;
#else
using AESDec_Type = AESDec<Portable>;
#endif

}

#endif // SRC_PRIMIHUB_UTIL_CRYPTO_AES_AESDEC_H_
