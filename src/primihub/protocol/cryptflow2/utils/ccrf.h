/*
Authors: Deevashwer Rathee
Copyright:
Copyright (c) 2020 Microsoft Research
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
*/

#ifndef CCRF_H__
#define CCRF_H__
#include "src/primihub/protocol/cryptflow2/utils/aes-ni.h"
#include "src/primihub/protocol/cryptflow2/utils/aes_opt.h"
#include "src/primihub/protocol/cryptflow2/utils/prg.h"
#include <stdio.h>
/** @addtogroup BP
  @{
  */
namespace primihub::sci {

inline void CCRF(block128 *y, block256 *k, int n) {
  AESNI_KEY aes[8];
  int r = n % 8;
  if (r == 0) {
    for (int i = 0; i < n / 8; i++) {
      AES_256_ks8(k + i * 8, aes);
      // AESNI_set_encrypt_key(&aes[j], k[i*8 + j]);
      AESNI_ecb_encrypt_blks_ks_x8(y + i * 8, 8, aes);
    }
  } else {
    for (int i = 0; i < (n - r) / 8; i++) {
      AES_256_ks8(k + i * 8, aes);
      // AESNI_set_encrypt_key(&aes[j], k[i*8 + j]);
      AESNI_ecb_encrypt_blks_ks_x8(y + i * 8, 8, aes);
    }
    for (int i = n - r; i < n; i++) {
      y[i] = one;
      AESNI_set_encrypt_key(&aes[0], k[i]);
      AESNI_ecb_encrypt_blks(y + i, 1, aes);
    }
  }
}

} // namespace primihub::sci
/**@}*/
#endif // CCRF_H__
