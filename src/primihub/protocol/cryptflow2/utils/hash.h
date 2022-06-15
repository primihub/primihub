/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

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

#ifndef HASH_H__
#define HASH_H__

#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/constants.h"
#include <openssl/sha.h>
#include <stdio.h>

/** @addtogroup BP
  @{
 */
namespace primihub::sci {
class Hash {
public:
  SHA256_CTX hash;
  char buffer[HASH_BUFFER_SIZE];
  int size = 0;
  static const int DIGEST_SIZE = 32;
  Hash() { SHA256_Init(&hash); }
  ~Hash() {}
  void put(const void *data, int nbyte) {
    if (nbyte > HASH_BUFFER_SIZE)
      SHA256_Update(&hash, data, nbyte);
    else if (size + nbyte < HASH_BUFFER_SIZE) {
      memcpy(buffer + size, data, nbyte);
      size += nbyte;
    } else {
      SHA256_Update(&hash, (char *)buffer, size);
      memcpy(buffer, data, nbyte);
      size = nbyte;
    }
  }
  void put_block(const block128 *block128, int nblock = 1) {
    put(block128, sizeof(block128) * nblock);
  }
  void digest(char *a) {
    if (size > 0) {
      SHA256_Update(&hash, (char *)buffer, size);
      size = 0;
    }
    SHA256_Final((unsigned char *)a, &hash);
  }
  void reset() {
    SHA256_Init(&hash);
    size = 0;
  }
  static void hash_once(void *digest, const void *data, int nbyte) {
    (void)SHA256((const unsigned char *)data, nbyte, (unsigned char *)digest);
  }
  __attribute__((target("sse2"))) static block128
  hash_for_block128(const void *data, int nbyte) {
    // even though stack is aligned to 16 byte, we don't know the order of
    // locals.
    alignas(16) char digest[DIGEST_SIZE];
    hash_once(digest, data, nbyte);
    return _mm_load_si128((__m128i *)&digest[0]);
  }

  __attribute__((target("avx"))) static block256
  hash_for_block256(const void *data, int nbyte) {
    alignas(32) char digest[DIGEST_SIZE];
    hash_once(digest, data, nbyte);
    return _mm256_load_si256((__m256i *)&digest[0]);
  }

  static block128 KDF128(primihub::emp::Point &in, uint64_t id = 1) {
    size_t len = in.size();
    in.group->resize_scratch(len + 8);
    unsigned char *tmp = in.group->scratch;
    in.to_bin(tmp, len);
    memcpy(tmp + len, &id, 8);
    block128 ret = hash_for_block128(tmp, len + 8);
    return ret;
  }

  static block256 KDF256(primihub::emp::Point &in, uint64_t id = 1) {
    size_t len = in.size();
    in.group->resize_scratch(len + 8);
    unsigned char *tmp = in.group->scratch;
    in.to_bin(tmp, len);
    memcpy(tmp + len, &id, 8);
    alignas(32) block256 ret = hash_for_block256(tmp, len + 8);
    return ret;
  }
};
} // namespace primihub::sci
/**@}*/
#endif // HASH_H__
