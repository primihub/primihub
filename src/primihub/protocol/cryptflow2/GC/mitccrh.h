/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2021 Microsoft Research

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

Modified by Deevashwer Rathee
*/

#ifndef EMP_MITCCRH_H__
#define EMP_MITCCRH_H__
#include "src/primihub/protocol/cryptflow2/GC/aes_opt.h"
#include <stdio.h>

namespace primihub::sci {

/*
 * [REF] Implementation of "Better Concrete Security for Half-Gates Garbling (in
 * the Multi-Instance Setting)" https://eprint.iacr.org/2019/1168.pdf
 */

template <int BatchSize = 8> class MITCCRH {
public:
  AES_KEY scheduled_key[BatchSize];
  block128 keys[BatchSize];
  int key_used = BatchSize;
  block128 start_point;
  uint64_t gid = 0;

  void setS(block128 sin) { this->start_point = sin; }

  void renew_ks(uint64_t gid) {
    this->gid = gid;
    renew_ks();
  }

  void renew_ks() {
    for (int i = 0; i < BatchSize; ++i)
      keys[i] = start_point ^ makeBlock128(gid++, 0);
    AES_opt_key_schedule<BatchSize>(keys, scheduled_key);
    key_used = 0;
  }

  template <int K, int H> void hash_cir(block128 *blks) {
    for (int i = 0; i < K * H; ++i)
      blks[i] = sigma(blks[i]);
    hash<K, H>(blks);
  }

  template <int K, int H> void hash(block128 *blks) {
    assert(K <= BatchSize);
    assert(BatchSize % K == 0);
    if (key_used == BatchSize)
      renew_ks();

    block128 tmp[K * H];
    for (int i = 0; i < K * H; ++i)
      tmp[i] = blks[i];

    ParaEnc<K, H>(tmp, scheduled_key + key_used);
    key_used += K;

    for (int i = 0; i < K * H; ++i)
      blks[i] = blks[i] ^ tmp[i];
  }
};
} // namespace primihub::sci
#endif // MITCCRH_H__
