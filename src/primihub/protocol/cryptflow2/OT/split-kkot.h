/*
Authors: Mayank Rathee, Deevashwer Rathee
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

#ifndef SPLIT_OT_KKOT_H__
#define SPLIT_OT_KKOT_H__

// In split functions, OT is split
// into offline and online phase.
// The KKOT functions without
// online offline split can
// be found in OT/kkot.h

#include "src/primihub/protocol/cryptflow2/OT/np.h"
#include "src/primihub/protocol/cryptflow2/OT/ot-utils.h"
#include "src/primihub/protocol/cryptflow2/OT/ot.h"
#include "src/primihub/protocol/cryptflow2/OT/split-utils.h"

namespace primihub::sci {
template <typename IO> class SplitKKOT : public OT<SplitKKOT<IO>> {
public:
  OTNP<IO> *base_ot;
  PRG128 prg;
  int party;
  const int lambda = 256;
#if __APPLE__
  int block_size = 1024 * 8;
#else
  int block_size = 1024 * 16;
#endif
  const int ro_batch_size = 2048;

  // This specifies how much OTs to preprocess in one go.
  // 0 means no preprocessing and all everything will be
  // run in the online phase.
  int precomp_batch_size = 0;

  // counter denotes the number of unused pre-generated OTs.
  int counter = precomp_batch_size;
  int N = 0, l;

  block256 *k0 = nullptr, *k1 = nullptr, *d = nullptr, *c_AND_s = nullptr,
           *qT = nullptr, *tT = nullptr, *tmp = nullptr, block_s;

  // h holds the precomputed hashes which can be used directly
  // in the online phase by xoring with the respective OT messages.
  uint8_t **h;
  uint64_t **h64;

  // r_off is the choice input set used for the offline random OTs.
  // This is corrected in the online phase when actual choice_input comes.
  uint8_t *r_off;
  PRG256 *G0, *G1;
  bool *s = nullptr, setup = false;
  bool precomp_masks = false;
  uint8_t *extended_r = nullptr;
  IO *io = nullptr;

  SplitKKOT(int party, IO *io, int N) {
    assert(party == ALICE || party == BOB);
    this->party = party;
    this->io = io;
    assert(N > 0);
    this->N = N;
    base_ot = new OTNP<IO>(io);
    s = new bool[lambda];
    k0 = new (std::align_val_t(32)) block256[lambda];
    k1 = new (std::align_val_t(32)) block256[lambda];
    d = new (std::align_val_t(32)) block256[block_size];
    c_AND_s = new (std::align_val_t(32)) block256[lambda];
    switch (party) {
    case ALICE:
      h = new uint8_t *[N];
      h64 = new uint64_t *[N];
      for (int i = 0; i < N; i++) {
        h[i] = new uint8_t[precomp_batch_size];
        h64[i] = new uint64_t[precomp_batch_size];
      }
      break;
    case BOB:
      h = new uint8_t *[1];
      h64 = new uint64_t *[1];
      r_off = new uint8_t[precomp_batch_size];
      for (int i = 0; i < 1; i++) {
        h[i] = new uint8_t[precomp_batch_size];
        h64[i] = new uint64_t[precomp_batch_size];
      }
      break;
    }
    G0 = new PRG256[lambda];
    G1 = new PRG256[lambda];
    tmp = new (std::align_val_t(32)) block256[block_size / 256];
    extended_r = new uint8_t[block_size];
  }

  ~SplitKKOT() {
    delete base_ot;
    delete[] s;
    delete[] k0;
    delete[] k1;
    delete[] d;
    if (precomp_masks)
      delete[] c_AND_s;
    switch (party) {
    case ALICE:
      for (int i = 0; i < N; i++) {
        delete[] h[i];
        delete[] h64[i];
      }
      delete[] h;
      delete[] h64;
      break;
    case BOB:
      delete[] h[0];
      delete[] h;
      delete[] h64[0];
      delete[] h64;
      delete[] r_off;
      break;
    }
    delete[] G0;
    delete[] G1;
    delete[] tmp;
    delete[] extended_r;
  }

  void set_precomp_batch_size(int batch_size) {
    this->precomp_batch_size = batch_size;
    this->counter = batch_size;
    if (precomp_masks) {
      delete[] c_AND_s;
      c_AND_s = new (std::align_val_t(32)) block256[lambda];
      precomp_masks = false;
    }
    switch (party) {
    case ALICE:
      for (int i = 0; i < N; i++) {
        delete[] h[i];
        delete[] h64[i];
      }
      delete[] h;
      delete[] h64;
      break;
    case BOB:
      delete[] h[0];
      delete[] h;
      delete[] h64[0];
      delete[] h64;
      delete[] r_off;
      break;
    }
    switch (party) {
    case ALICE:
      h = new uint8_t *[N];
      h64 = new uint64_t *[N];
      for (int i = 0; i < N; i++) {
        h[i] = new uint8_t[precomp_batch_size];
        h64[i] = new uint64_t[precomp_batch_size];
      }
      break;
    case BOB:
      h = new uint8_t *[1];
      h64 = new uint64_t *[1];
      r_off = new uint8_t[precomp_batch_size];
      for (int i = 0; i < 1; i++) {
        h[i] = new uint8_t[precomp_batch_size];
        h64[i] = new uint64_t[precomp_batch_size];
      }
      break;
    }
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
    precomp_masks = true;
    for (int i = 0; i < N; i++) {
      c_AND_s[i] =
          andBlocks(_mm256_lddqu_si256((const __m256i *)WH_Code[i]), block_s);
    }
  }

  int padded_length(int length) {
    return ((length + block_size - 1) / block_size) * block_size;
  }

  void preprocess() {
    switch (party) {
    case ALICE:
      send_pre(counter);
      got_send_offline(counter);
      break;
    case BOB:
      prg.random_data(r_off, counter);
      uint8_t mask = N - 1; // N is a power of 2
      for (int i = 0; i < counter; i++) {
        r_off[i] &= mask;
      }
      recv_pre(r_off, counter);
      got_recv_offline(counter);
      break;
    }
    counter = 0;
  }

  void send_pre(int length) {
    int old_block_size = this->block_size;
    this->block_size =
        std::min(old_block_size, int(ceil(length / 256.0)) * 256);
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
    this->block_size = old_block_size;
  }

  void recv_pre(const uint8_t *r, int length) {
    int old_block_size = this->block_size;
    this->block_size =
        std::min(old_block_size, int(ceil(length / 256.0)) * 256);
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

    this->block_size = old_block_size;
  }

  void got_send_offline(int length) {
    const int bsize = ro_batch_size;
    block256 *key = new (std::align_val_t(32)) block256[N * bsize];
    block128 *pad = new block128[N * bsize];

    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        for (int k = 0; k < N; k++) {
          key[N * (j - i) + k] = xorBlocks(qT[j], c_AND_s[k]);
        }
      }
      CCRF(pad, key, N * bsize);
      for (int j = i; j < i + bsize and j < length; ++j) {
        for (int k = 0; k < N; k++) {
          h[k][j] = ((uint8_t)_mm_extract_epi8(pad[N * (j - i) + k], 0));
          h64[k][j] = ((uint64_t)_mm_extract_epi64(pad[N * (j - i) + k], 0));
        }
      }
    }
    delete[] qT;
    delete[] key;
    delete[] pad;
  }

  void got_recv_offline(int length) {
    const int bsize = ro_batch_size;
    block128 *pad = new block128[N * bsize];

    for (int i = 0; i < length; i += bsize) {
      if (bsize <= length - i)
        CCRF(pad, tT + i, bsize);
      else
        CCRF(pad, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        h[0][i + j] = ((uint8_t)_mm_extract_epi8(pad[j], 0));
        h64[0][i + j] = ((uint64_t)_mm_extract_epi64(pad[j], 0));
      }
    }
    delete[] tT;
    delete[] pad;
  }

  template <typename T> void got_send_online(T **data, int length) {
    const int bsize = length; // ro_batch_size;
    uint32_t y_size = (uint32_t)ceil((N * bsize * l) / ((float)sizeof(T) * 8));
    int bits_in_sel_input = primihub::sci::bitlen(N);
    uint32_t a_size = (uint32_t)ceil((bsize * bits_in_sel_input) /
                                     ((float)sizeof(uint8_t) * 8));
    T y[y_size];
    uint8_t a_packed[a_size];
    uint8_t a[length];
    T **maskeddata = new T *[bsize];
    for (int i = 0; i < bsize; i++) {
      maskeddata[i] = new T[N];
    }
    int32_t corrected_bsize, corrected_y_size, corrected_a_size;
    uint8_t mask_a;
    mask_a = (1 << bits_in_sel_input) - 1;
    if (bits_in_sel_input == 8) {
      mask_a = -1;
    }

    for (int ctr = 0; ctr < length; ctr += bsize) {
      corrected_bsize = std::min(bsize, length - ctr);
      corrected_y_size =
          (uint32_t)ceil((N * corrected_bsize * l) / ((float)sizeof(T) * 8));
      corrected_a_size = (uint32_t)ceil((corrected_bsize * bits_in_sel_input) /
                                        ((float)sizeof(uint8_t) * 8));
      // Receive correction of choice input
      io->recv_data(a_packed, sizeof(uint8_t) * corrected_a_size);
      unpack_a<uint8_t>(a, a_packed, corrected_bsize, bits_in_sel_input);
      if (sizeof(T) == 8) {
        for (int i = 0; i < corrected_bsize; i++) {
          for (int k = 0; k < N; k++) {
            maskeddata[i][k] =
                (h64[(k + a[i]) & mask_a][counter] ^ data[ctr + i][k]);
          }
          counter++;
        }
      } else if (sizeof(T) == 1) {
        for (int i = 0; i < corrected_bsize; i++) {
          for (int k = 0; k < N; k++) {
            maskeddata[i][k] =
                (h[(k + a[i]) & mask_a][counter] ^ data[ctr + i][k]);
          }
          counter++;
        }
      } else {
        throw std::invalid_argument("Not yet implemented");
      }
      pack_messages<T>(y, maskeddata, corrected_y_size, corrected_bsize, l, N);
      io->send_data(y, sizeof(T) * corrected_y_size);
    }
    for (int i = 0; i < bsize; i++) {
      delete[] maskeddata[i];
    }
    delete[] maskeddata;
  }

  template <typename T>
  void got_recv_online(T *data, const uint8_t *r, int length) {
    const int bsize = length;
    uint32_t res_size =
        (uint32_t)ceil((N * length * l) / ((float)sizeof(T) * 8));
    int bits_in_sel_input = primihub::sci::bitlen(N);
    uint32_t a_size = (uint32_t)ceil((length * bits_in_sel_input) /
                                     ((float)sizeof(uint8_t) * 8));
    T res[res_size];
    uint8_t a_unpacked[length];
    uint8_t a[a_size];
    int32_t corrected_bsize, corrected_res_size, corrected_a_size;
    uint8_t mask_a;
    mask_a = (1 << bits_in_sel_input) - 1;
    if (bits_in_sel_input == 8) {
      mask_a = -1;
    }

    for (int ctr = 0; ctr < length; ctr += bsize) {
      corrected_bsize = std::min(bsize, length - ctr);
      corrected_res_size =
          (uint32_t)ceil((N * corrected_bsize * l) / ((float)sizeof(T) * 8));
      corrected_a_size = (uint32_t)ceil((corrected_bsize * bits_in_sel_input) /
                                        ((float)sizeof(uint8_t) * 8));

      int counter_memory = counter;
      // Send corrected choice inputs
      for (int i = 0; i < corrected_bsize; i++) {
        a_unpacked[i] =
            (r_off[counter++] - r[ctr + i]) & mask_a; // Replicates neg_mod
      }
      pack_a<uint8_t>(a, a_unpacked, corrected_a_size, corrected_bsize,
                      bits_in_sel_input);
      io->send_data(a, sizeof(uint8_t) * corrected_a_size);
      counter = counter_memory;
      // Receive OT messages
      io->recv_data(res, sizeof(T) * corrected_res_size);
      if (sizeof(T) == 8) {
        unpack_messages<uint64_t>((uint64_t *)data + ctr, r + ctr,
                                  (uint64_t *)res, h64[0], corrected_bsize, l,
                                  N, counter);
      } else if (sizeof(T) == 1) {
        unpack_messages<uint8_t>((uint8_t *)data + ctr, r + ctr, (uint8_t *)res,
                                 h[0], corrected_bsize, l, N, counter);
      } else {
        throw std::invalid_argument("Not yet implemented");
      }
    }
  }

  template <typename T> void got_send_post(T **data, int length) {
    const int bsize = ro_batch_size;
    block256 *key = new (std::align_val_t(32)) block256[N * bsize];
    block128 *pad = new block128[N * bsize];
    uint32_t y_size =
        (uint32_t)ceil((N * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t corrected_y_size, corrected_bsize;
    T *y = new T[y_size];

    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        for (int k = 0; k < N; k++) {
          key[N * (j - i) + k] = xorBlocks(qT[j], c_AND_s[k]);
        }
      }
      CCRF(pad, key, N * bsize);
      corrected_y_size = (uint32_t)ceil(
          (N * std::min(bsize, length - i) * this->l) / ((float)sizeof(T) * 8));
      corrected_bsize = std::min(bsize, length - i);
      if (sizeof(T) == 8) {
        pack_ot_messages<uint64_t>((uint64_t *)y, (uint64_t **)data + i, pad,
                                   corrected_y_size, corrected_bsize, this->l,
                                   this->N);
      } else if (sizeof(T) == 1) {
        pack_ot_messages<uint8_t>((uint8_t *)y, (uint8_t **)data + i, pad,
                                  corrected_y_size, corrected_bsize, this->l,
                                  this->N);
      } else {
        throw std::invalid_argument("Not implemented");
      }
      io->send_data(y, sizeof(T) * (corrected_y_size));
    }
    delete[] qT;
    delete[] y;
    delete[] key;
    delete[] pad;
  }

  template <typename T>
  void got_recv_post(T *data, const uint8_t *r, int length) {
    const int bsize = ro_batch_size;
    block128 *pad = new block128[N * bsize];
    uint32_t recvd_size =
        (uint32_t)ceil((N * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t corrected_recvd_size, corrected_bsize;
    T *recvd = new T[recvd_size];
    for (int i = 0; i < length; i += bsize) {
      uint32_t corrected_recvd_size = (uint32_t)ceil(
          (N * std::min(bsize, length - i) * this->l) / ((float)sizeof(T) * 8));
      corrected_bsize = std::min(bsize, length - i);
      io->recv_data(recvd, sizeof(T) * (corrected_recvd_size));
      if (bsize <= length - i)
        CCRF(pad, tT + i, bsize);
      else
        CCRF(pad, tT + i, length - i);

      if (sizeof(T) == 8) {
        unpack_ot_messages<uint64_t>((uint64_t *)data + i, r + i,
                                     (uint64_t *)recvd, pad, corrected_bsize,
                                     this->l, this->N);
      } else if (sizeof(T) == 1) {
        unpack_ot_messages<uint8_t>((uint8_t *)data + i, r + i,
                                    (uint8_t *)recvd, pad, corrected_bsize,
                                    this->l, this->N);
      } else {
        throw std::invalid_argument("Not implemented");
      }
    }
    delete[] tT;
    delete[] pad;
    delete[] recvd;
  }

  void send_impl(uint8_t **data, int length, int l) {
    assert(N <= lambda && N >= 2);
    assert(l <= 8 && l >= 1);
    // assert(((N*l*length) % 8) == 0);
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_send_online<uint8_t>(data, length);
    } else {
      send_pre(length);
      got_send_post<uint8_t>(data, length);
    }
  }

  void recv_impl(uint8_t *data, uint8_t *b, int length, int l) {
    assert(N <= lambda && N >= 2);
    assert(l <= 8 && l >= 1);
    // assert(((N*l*length) % 8) == 0);
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_recv_online<uint8_t>(data, b, length);
    } else {
      recv_pre(b, length);
      got_recv_post<uint8_t>(data, b, length);
    }
  }

  void send_impl(uint64_t **data, int length, int l) {
    assert(N <= lambda && N >= 2);
    // assert(l > 8);
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_send_online<uint64_t>(data, length);
    } else {
      send_pre(length);
      got_send_post<uint64_t>(data, length);
    }
  }

  void recv_impl(uint64_t *data, uint8_t *b, int length, int l) {
    assert(N <= lambda && N >= 2);
    // assert(l > 8);
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_recv_online<uint64_t>(data, b, length);
    } else {
      recv_pre(b, length);
      got_recv_post<uint64_t>(data, b, length);
    }
  }
};
} // namespace primihub::sci
#endif // SPLIT_OT_KKOT_H__
