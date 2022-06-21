/*
Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)

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
*/

#ifndef PRP_H__
#define PRP_H__
#include "src/primihub/protocol/cryptflow2/utils/aes.h"
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/constants.h"
#include <stdio.h>
/** @addtogroup BP
  @{
 */
namespace primihub::sci {

class PRP {
public:
  AES_KEY aes;

  PRP(const char *seed = fix_key) { aes_set_key(seed); }

  PRP(const block128 &seed) : PRP((const char *)&seed) {}

  void aes_set_key(const char *key) {
    aes_set_key(_mm_loadu_si128((block128 *)key));
  }

  void aes_set_key(const block128 &v) { AES_set_encrypt_key(v, &aes); }

  void permute_block(block128 *data, int nblocks) {
    int i = 0;
    for (; i < nblocks - AES_BATCH_SIZE; i += AES_BATCH_SIZE) {
      AES_ecb_encrypt_blks(data + i, AES_BATCH_SIZE, &aes);
    }
    AES_ecb_encrypt_blks(
        data + i, (AES_BATCH_SIZE > nblocks - i) ? nblocks - i : AES_BATCH_SIZE,
        &aes);
  }

  void permute_data(void *data, int nbytes) {
    permute_block((block128 *)data, nbytes / 16);
    if (nbytes % 16 != 0) {
      uint8_t extra[16];
      memset(extra, 0, 16);
      memcpy(extra, (nbytes / 16 * 16) + (char *)data, nbytes % 16);
      permute_block((block128 *)extra, 1);
      memcpy((nbytes / 16 * 16) + (char *)data, &extra, nbytes % 16);
    }
  }

  block128 H(block128 in, uint64_t id) {
    in = double_block(in);
    __m128i k_128 = _mm_loadl_epi64((__m128i const *)(&id));
    in = xorBlocks(in, k_128);
    block128 t = in;
    permute_block(&t, 1);
    in = xorBlocks(in, t);
    return in;
  }
  template <int n> void H(block128 out[n], block128 in[n], uint64_t id) {
    block128 scratch[n];
    for (int i = 0; i < n; ++i) {
      out[i] = scratch[i] = xorBlocks(double_block(in[i]),
                                      _mm_loadl_epi64((__m128i const *)(&id)));
      ++id;
    }
    permute_block(scratch, n);
    xorBlocks_arr(out, scratch, out, n);
  }

  void Hn(block128 *out, block128 *in, uint64_t id, int length,
          block128 *scratch = nullptr) {
    bool del = false;
    if (scratch == nullptr) {
      del = true;
      scratch = new block128[length];
    }
    for (int i = 0; i < length; ++i) {
      out[i] = scratch[i] = xorBlocks(double_block(in[i]),
                                      _mm_loadl_epi64((__m128i const *)(&id)));
      ++id;
    }
    permute_block(scratch, length);
    xorBlocks_arr(out, scratch, out, length);
    if (del) {
      delete[] scratch;
      scratch = nullptr;
    }
  }
};
} // namespace primihub::sci
/**@}*/
#endif // PRP_H__
