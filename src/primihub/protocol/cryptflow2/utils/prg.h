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

#ifndef PRG_H__
#define PRG_H__
#include "src/primihub/protocol/cryptflow2/utils/aes-ni.h"
#include "src/primihub/protocol/cryptflow2/utils/aes.h"
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/constants.h"
#include <gmp.h>
#include <random>

#ifdef EMP_USE_RANDOM_DEVICE
#else
#include <x86intrin.h>
#endif

/** @addtogroup BP
  @{
 */
namespace primihub::sci {

class PRG128 {
public:
  uint64_t counter = 0;
  AES_KEY aes;
  PRG128(const void *seed = nullptr, int id = 0) {
    if (seed != nullptr) {
      reseed(seed, id);
    } else {
      block128 v;
#ifdef EMP_USE_RANDOM_DEVICE
      int *data = (int *)(&v);
      std::random_device rand_div;
      for (size_t i = 0; i < sizeof(block128) / sizeof(int); ++i)
        data[i] = rand_div();
#else
      unsigned long long r0, r1;
      _rdseed64_step(&r0);
      _rdseed64_step(&r1);
      v = makeBlock128(r0, r1);
#endif
      reseed(&v);
    }
  }
  void reseed(const void *key, uint64_t id = 0) {
    block128 v = _mm_loadu_si128((block128 *)key);
    v = xorBlocks(v, makeBlock128(0LL, id));
    AES_set_encrypt_key(v, &aes);
    counter = 0;
  }

  void random_data(void *data, int nbytes) {
    random_block((block128 *)data, nbytes / 16);
    if (nbytes % 16 != 0) {
      block128 extra;
      random_block(&extra, 1);
      memcpy((nbytes / 16 * 16) + (char *)data, &extra, nbytes % 16);
    }
  }

  void random_bool(bool *data, int length) {
    uint8_t *uint_data = (uint8_t *)data;
    random_data(uint_data, length);
    for (int i = 0; i < length; ++i)
      data[i] = uint_data[i] & 1;
  }

  void random_data_unaligned(void *data, int nbytes) {
    block128 tmp[AES_BATCH_SIZE];
    for (int i = 0; i < nbytes / (AES_BATCH_SIZE * 16); i++) {
      random_block(tmp, AES_BATCH_SIZE);
      memcpy((16 * i * AES_BATCH_SIZE) + (uint8_t *)data, tmp,
             16 * AES_BATCH_SIZE);
    }
    if (nbytes % (16 * AES_BATCH_SIZE) != 0) {
      random_block(tmp, AES_BATCH_SIZE);
      memcpy((nbytes / (16 * AES_BATCH_SIZE) * (16 * AES_BATCH_SIZE)) +
                 (uint8_t *)data,
             tmp, nbytes % (16 * AES_BATCH_SIZE));
    }
  }

  void random_block(block128 *data, int nblocks = 1) {
    for (int i = 0; i < nblocks; ++i) {
      data[i] = makeBlock128(0LL, counter++);
    }
    int i = 0;
    for (; i < nblocks - AES_BATCH_SIZE; i += AES_BATCH_SIZE) {
      AES_ecb_encrypt_blks(data + i, AES_BATCH_SIZE, &aes);
    }
    AES_ecb_encrypt_blks(
        data + i, (AES_BATCH_SIZE > nblocks - i) ? nblocks - i : AES_BATCH_SIZE,
        &aes);
  }

  void random_block(block256 *data, int nblocks = 1) {
    nblocks = nblocks * 2;
    block128 tmp[nblocks];
    for (int i = 0; i < nblocks; ++i) {
      tmp[i] = makeBlock128(0LL, counter++);
    }
    int i = 0;
    for (; i < nblocks - AES_BATCH_SIZE; i += AES_BATCH_SIZE) {
      AES_ecb_encrypt_blks(tmp + i, AES_BATCH_SIZE, &aes);
    }
    AES_ecb_encrypt_blks(
        tmp + i, (AES_BATCH_SIZE > nblocks - i) ? nblocks - i : AES_BATCH_SIZE,
        &aes);
    for (int i = 0; i < nblocks / 2; ++i) {
      data[i] = makeBlock256(tmp[2 * i], tmp[2 * i + 1]);
    }
  }

  void random_mpz(mpz_t out, int nbits) {
    int nbytes = (nbits + 7) / 8;
    uint8_t *data = (uint8_t *)new block128[(nbytes + 15) / 16];
    random_data(data, nbytes);
    if (nbits % 8 != 0)
      data[0] %= (1 << (nbits % 8));
    mpz_import(out, nbytes, 1, 1, 0, 0, data);
    delete[] data;
  }

  void random_mpz(mpz_t rop, const mpz_t n) {
    auto size = mpz_sizeinbase(n, 2);
    while (1) {
      random_mpz(rop, (int)size);
      if (mpz_cmp(rop, n) < 0) {
        break;
      }
    }
  }

  template <typename T> void random_mod_p(T *arr, uint64_t size, T prime_mod) {
    T boundary = (((-1 * prime_mod) / prime_mod) + 1) *
                 prime_mod; // prime_mod*floor((2^l)/prime_mod)
    int tries_before_resampling = 2;
    uint64_t size_total = tries_before_resampling * size;
    T *randomness = new T[size_total];
    uint64_t rptr = 0, arrptr = 0;
    while (arrptr < size) {
      this->random_data(randomness, sizeof(T) * size_total);
      rptr = 0;
      for (; (arrptr < size) && (rptr < size_total); arrptr++, rptr++) {
        while (randomness[rptr] > boundary) {
          rptr++;
          if (rptr >= size_total) {
            this->random_data(randomness, sizeof(T) * size_total);
            rptr = 0;
          }
        }
        arr[arrptr] = randomness[rptr] % prime_mod;
      }
    }
    delete[] randomness;
  }
};

class PRG256 {
public:
  uint64_t counter = 0;
  AESNI_KEY aes;
  PRG256(const void *seed = nullptr, int id = 0) {
    if (seed != nullptr) {
      reseed(seed, id);
    } else {
      alignas(32) block256 v;
#ifdef EMP_USE_RANDOM_DEVICE
      int *data = (int *)(&v);
      std::random_device rand_div;
      for (size_t i = 0; i < sizeof(block256) / sizeof(int); ++i)
        data[i] = rand_div();
#else
      unsigned long long r0, r1, r2, r3;
      _rdseed64_step(&r0);
      _rdseed64_step(&r1);
      _rdseed64_step(&r2);
      _rdseed64_step(&r3);
      v = makeBlock256(r0, r1, r2, r3);
#endif
      reseed(&v);
    }
  }
  void reseed(const void *key, uint64_t id = 0) {
    alignas(32) block256 v = _mm256_load_si256((block256 *)key);
    v = xorBlocks(v, makeBlock256(0LL, 0LL, 0LL, id));
    AESNI_set_encrypt_key(&aes, v);
    counter = 0;
  }

  void random_data(void *data, int nbytes) {
    random_block((block128 *)data, nbytes / 16);
    if (nbytes % 16 != 0) {
      block128 extra;
      random_block(&extra, 1);
      memcpy((nbytes / 16 * 16) + (char *)data, &extra, nbytes % 16);
    }
  }

  void random_bool(bool *data, int length) {
    uint8_t *uint_data = (uint8_t *)data;
    random_data(uint_data, length);
    for (int i = 0; i < length; ++i)
      data[i] = uint_data[i] & 1;
  }

  void random_data_unaligned(void *data, int nbytes) {
    block128 tmp[AES_BATCH_SIZE];
    for (int i = 0; i < nbytes / (AES_BATCH_SIZE * 16); i++) {
      random_block(tmp, AES_BATCH_SIZE);
      memcpy((16 * i * AES_BATCH_SIZE) + (uint8_t *)data, tmp,
             16 * AES_BATCH_SIZE);
    }
    if (nbytes % (16 * AES_BATCH_SIZE) != 0) {
      random_block(tmp, AES_BATCH_SIZE);
      memcpy((nbytes / (16 * AES_BATCH_SIZE) * (16 * AES_BATCH_SIZE)) +
                 (uint8_t *)data,
             tmp, nbytes % (16 * AES_BATCH_SIZE));
    }
  }

  void random_block(block128 *data, int nblocks = 1) {
    for (int i = 0; i < nblocks; ++i) {
      data[i] = makeBlock128(0LL, counter++);
    }
    int i = 0;
    for (; i < nblocks - AES_BATCH_SIZE; i += AES_BATCH_SIZE) {
      AESNI_ecb_encrypt_blks(data + i, AES_BATCH_SIZE, &aes);
    }
    AESNI_ecb_encrypt_blks(
        data + i, (AES_BATCH_SIZE > nblocks - i) ? nblocks - i : AES_BATCH_SIZE,
        &aes);
  }

  void random_block(block256 *data, int nblocks = 1) {
    nblocks = nblocks * 2;
    block128 tmp[nblocks];
    for (int i = 0; i < nblocks; ++i) {
      tmp[i] = makeBlock128(0LL, counter++);
    }
    int i = 0;
    for (; i < nblocks - AES_BATCH_SIZE; i += AES_BATCH_SIZE) {
      AESNI_ecb_encrypt_blks(tmp + i, AES_BATCH_SIZE, &aes);
    }
    AESNI_ecb_encrypt_blks(
        tmp + i, (AES_BATCH_SIZE > nblocks - i) ? nblocks - i : AES_BATCH_SIZE,
        &aes);
    for (int i = 0; i < nblocks / 2; ++i) {
      data[i] = makeBlock256(tmp[2 * i], tmp[2 * i + 1]);
    }
  }

  void random_mpz(mpz_t out, int nbits) {
    int nbytes = (nbits + 7) / 8;
    uint8_t *data = (uint8_t *)new block128[(nbytes + 15) / 16];
    random_data(data, nbytes);
    if (nbits % 8 != 0)
      data[0] %= (1 << (nbits % 8));
    mpz_import(out, nbytes, 1, 1, 0, 0, data);
    delete[] data;
  }

  void random_mpz(mpz_t rop, const mpz_t n) {
    auto size = mpz_sizeinbase(n, 2);
    while (1) {
      random_mpz(rop, (int)size);
      if (mpz_cmp(rop, n) < 0) {
        break;
      }
    }
  }
};

} // namespace primihub::sci
#endif // PRG_H__
