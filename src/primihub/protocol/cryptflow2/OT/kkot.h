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

#ifndef OT_KKOT_H__
#define OT_KKOT_H__
#include "src/primihub/protocol/cryptflow2/OT/np.h"
#include "src/primihub/protocol/cryptflow2/OT/ot.h"

namespace primihub::sci {
template <typename IO> class KKOT : public OT<KKOT<IO>> {
public:
  OTNP<IO> *base_ot;
  PRG128 prg;
  const int lambda = 256;
  int block_size = 1024 * 16;
  int ro_batch_size = 2048;
  int N, l, max_N = 0;

  block256 *k0 = nullptr, *k1 = nullptr, *d = nullptr, *c_AND_s = nullptr,
           *qT = nullptr, *tT = nullptr, *tmp = nullptr, block_s;
  PRG256 *G0, *G1;
  bool *s = nullptr, setup = false, precomp_masks = false;
  uint8_t *extended_r = nullptr;
  IO *io = nullptr;

  KKOT(IO *io) {
    this->io = io;
    base_ot = new OTNP<IO>(io);
    s = new bool[lambda];
    k0 = new (std::align_val_t(32)) block256[lambda];
    k1 = new (std::align_val_t(32)) block256[lambda];
    d = new (std::align_val_t(32)) block256[block_size];
    c_AND_s = new (std::align_val_t(32)) block256[lambda];
    G0 = new PRG256[lambda];
    G1 = new PRG256[lambda];
    tmp = new (std::align_val_t(32)) block256[block_size / 256];
    extended_r = new uint8_t[block_size];
  }

  ~KKOT() {
    delete base_ot;
    delete[] s;
    delete[] k0;
    delete[] k1;
    delete[] d;
    delete[] c_AND_s;
    delete[] G0;
    delete[] G1;
    delete[] tmp;
    delete[] extended_r;
  }

  void setup_send(block256 *in_k0 = nullptr, bool *in_s = nullptr) {
    setup = true;
    if (in_s != nullptr) {
      memcpy(k0, in_k0, lambda * sizeof(block256));
      memcpy(s, in_s, lambda);
      block_s = bool_to256(s);
    } else {
      prg.random_bool(s, lambda);
      base_ot->recv(k0, s, lambda);
      block_s = bool_to256(s);
    }
    for (int i = 0; i < lambda; ++i)
      G0[i].reseed(&k0[i]);
  }

  void setup_recv(block256 *in_k0 = nullptr, block256 *in_k1 = nullptr) {
    setup = true;
    if (in_k0 != nullptr) {
      memcpy(k0, in_k0, lambda * sizeof(block256));
      memcpy(k1, in_k1, lambda * sizeof(block256));
    } else {
      prg.random_block(k0, lambda);
      prg.random_block(k1, lambda);
      base_ot->send(k0, k1, lambda);
    }
    for (int i = 0; i < lambda; ++i) {
      G0[i].reseed(&k0[i]);
      G1[i].reseed(&k1[i]);
    }
  }

  void precompute_masks() {
    assert(setup == true);
    assert(N > 0);
    precomp_masks = true;
    for (int i = max_N; i < N; i++) {
      c_AND_s[i] =
          andBlocks(_mm256_lddqu_si256((const __m256i *)WH_Code[i]), block_s);
    }
    max_N = N;
  }

  int padded_length(int length) {
    return ((length + block_size - 1) / block_size) * block_size;
  }

  void send_pre(int length) {
    length = padded_length(length);
    alignas(32) block256 q[block_size];
    qT = new (std::align_val_t(32)) block256[length];
    if (!setup)
      setup_send();
    if (!precomp_masks)
      precompute_masks();

    for (int j = 0; j < length / block_size; ++j) {
      for (int i = 0; i < lambda; ++i) {
        G0[i].random_data(q + (i * block_size / 256), block_size / 8);
        io->recv_data(tmp, block_size / 8);
        if (s[i])
          xorBlocks_arr(q + (i * block_size / 256), q + (i * block_size / 256),
                        tmp, block_size / 256);
      }
      sse_trans((uint8_t *)(qT + j * block_size), (uint8_t *)q, 256,
                block_size);
    }
  }

  void recv_pre(const uint8_t *r, int length) {
    int old_length = length;
    length = padded_length(length);
    alignas(32) block256 t[block_size];
    tT = new (std::align_val_t(32)) block256[length];

    if (not setup)
      setup_recv();

    uint8_t *r2 = new uint8_t[length];
    prg.random_data(extended_r, block_size);
    memcpy(r2, r, old_length);
    memcpy(r2 + old_length, extended_r, length - old_length);

    block256 *dT = new (std::align_val_t(32)) block256[length];
    for (int i = 0; i < length; i++)
      dT[i] = _mm256_lddqu_si256((const __m256i *)WH_Code[r2[i]]);

    for (int j = 0; j * block_size < length; ++j) {
      sse_trans((uint8_t *)d, (uint8_t *)(dT + j * block_size), block_size,
                256);
      for (int i = 0; i < lambda; ++i) {
        G0[i].random_data(t + (i * block_size / 256), block_size / 8);
        G1[i].random_data(tmp, block_size / 8);
        xorBlocks_arr(tmp, t + (i * block_size / 256), tmp, block_size / 256);
        xorBlocks_arr(tmp, d + (i * block_size / 256), tmp, block_size / 256);
        io->send_data(tmp, block_size / 8);
      }
      sse_trans((uint8_t *)(tT + j * block_size), (uint8_t *)t, 256,
                block_size);
    }

    delete[] dT;
    delete[] r2;
  }

  void got_send_post(block128 **data, int length) {
    const int bsize = ro_batch_size;
    block256 *key = new (std::align_val_t(32)) block256[N * bsize];
    block128 *y = new block128[N * bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        for (int k = 0; k < N; k++) {
          key[N * (j - i) + k] = xorBlocks(qT[j], c_AND_s[k]);
        }
      }
      CCRF(y, key, N * bsize);
      for (int j = i; j < i + bsize and j < length; ++j) {
        for (int k = 0; k < N; k++) {
          y[N * (j - i) + k] = xorBlocks(y[N * (j - i) + k], data[j][k]);
        }
      }
      io->send_data(y, N * sizeof(block128) * std::min(bsize, length - i));
    }
    delete[] key;
    delete[] y;
    delete[] qT;
  }

  void got_recv_post(block128 *data, const uint8_t *r, int length) {
    const int bsize = ro_batch_size;
    block128 *pad = new block128[bsize];
    block128 *res = new block128[N * bsize];
    for (int i = 0; i < length; i += bsize) {
      io->recv_data(res, N * sizeof(block128) * std::min(bsize, length - i));
      if (bsize <= length - i)
        CCRF(pad, tT + i, bsize);
      else
        CCRF(pad, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        data[i + j] = xorBlocks(res[N * j + r[i + j]], pad[j]);
      }
    }
    delete[] pad;
    delete[] res;
    delete[] tT;
  }

  void send_impl(block128 **data, int length, int N) {
    if (length < 1)
      return;
    assert(N <= lambda && N >= 2);
    this->N = N;
    if ((this->max_N < N) && setup == true)
      precompute_masks();
    send_pre(length);
    got_send_post(data, length);
  }

  void recv_impl(block128 *data, const uint8_t *b, int length, int N) {
    if (length < 1)
      return;
    assert(N <= lambda && N >= 2);
    this->N = N;
    recv_pre(b, length);
    got_recv_post(data, b, length);
  }
};

} // namespace primihub::sci
#endif // OT_KKOT_H__
