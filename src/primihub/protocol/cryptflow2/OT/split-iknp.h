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

#ifndef SPLIT_OT_IKNP_H__
#define SPLIT_OT_IKNP_H__

// In split functions, OT is split
// into offline and online phase.

#include "src/primihub/protocol/cryptflow2/OT/np.h"
#include "src/primihub/protocol/cryptflow2/OT/ot-utils.h"
#include "src/primihub/protocol/cryptflow2/OT/ot.h"
#include "split-utils.h"

namespace primihub::sci {
template <typename IO> class SplitIKNP : public OT<SplitIKNP<IO>> {
public:
  OTNP<IO> *base_ot;
  PRG128 prg;
  int party;
  const int lambda = 128;
  int block_size = 1024 * 16;

  // This specifies how much OTs to preprocess in one go.
  // 0 means no preprocessing and all everything will be
  // run in the online phase.
  int precomp_batch_size = 0;
  // counter denotes the number of pre-generated OTs used
  int counter = precomp_batch_size;
  int l;

  block128 *k0 = nullptr, *k1 = nullptr, *qT = nullptr, *tT = nullptr,
           *tmp = nullptr, block_s;
  PRG128 *G0, *G1;
  bool *s = nullptr, *extended_r = nullptr, setup = false;
  IO *io = nullptr;
  CRH crh;

  // h holds the precomputed hashes which can be used directly in the online
  // phase by xoring with the respective OT messages.
  uint8_t **h;
  uint64_t **h64;

  // r_off is the choice input set used for the offline random OT.
  // This is corrected in the online phase when actual choice_input comes.
  uint8_t *r_off;
  int N = 2;
  SplitIKNP(int party, IO *io) {
    assert(party == ALICE || party == BOB);
    this->party = party;
    this->io = io;
    base_ot = new OTNP<IO>(io);
    s = new bool[lambda];
    k0 = new block128[lambda];
    k1 = new block128[lambda];
    switch (party) {
    case ALICE: {
      h = new uint8_t *[N];
      h64 = new uint64_t *[N];
      for (int i = 0; i < N; i++) {
        h[i] = new uint8_t[precomp_batch_size];
        h64[i] = new uint64_t[precomp_batch_size];
      }
      break;
    }
    case BOB: {
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
    G0 = new PRG128[lambda];
    G1 = new PRG128[lambda];
    tmp = new block128[block_size / 128];
    extended_r = new bool[block_size];
  }

  ~SplitIKNP() {
    delete base_ot;
    delete[] s;
    delete[] k0;
    delete[] k1;
    switch (party) {
    case ALICE: {
      for (int i = 0; i < N; i++) {
        delete[] h[i];
        delete[] h64[i];
      }
      delete[] h;
      delete[] h64;
      break;
    }
    case BOB: {
      delete[] h[0];
      delete[] h;
      delete[] h64[0];
      delete[] h64;
      delete[] r_off;
      break;
    }
    }
    delete[] G0;
    delete[] G1;
    delete[] tmp;
    delete[] extended_r;
  }

  void set_precomp_batch_size(int batch_size) {
    this->precomp_batch_size = batch_size;
    this->counter = batch_size;
    switch (party) {
    case ALICE: {
      for (int i = 0; i < N; i++) {
        delete[] h[i];
        delete[] h64[i];
      }
      delete[] h;
      delete[] h64;
      break;
    }
    case BOB: {
      delete[] h[0];
      delete[] h;
      delete[] h64[0];
      delete[] h64;
      delete[] r_off;
      break;
    }
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

  void setup_send(block128 *in_k0 = nullptr, bool *in_s = nullptr) {
    setup = true;
    if (in_s != nullptr) {
      memcpy(k0, in_k0, lambda * sizeof(block128));
      memcpy(s, in_s, lambda);
      block_s = bool_to128(s);
    } else {
      prg.random_bool(s, lambda);
      base_ot->recv(k0, s, lambda);
      block_s = bool_to128(s);
    }
    for (int i = 0; i < lambda; ++i)
      G0[i].reseed(&k0[i]);
  }

  void setup_recv(block128 *in_k0 = nullptr, block128 *in_k1 = nullptr) {
    setup = true;
    if (in_k0 != nullptr) {
      memcpy(k0, in_k0, lambda * sizeof(block128));
      memcpy(k1, in_k1, lambda * sizeof(block128));
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

  int padded_length(int length) {
    return ((length + block_size - 1) / block_size) * block_size;
  }

  void send_pre(int length) {
    int old_block_size = this->block_size;
    this->block_size =
        std::min(old_block_size, int(ceil(length / 256.0)) * 256);
    length = padded_length(length);
    block128 q[block_size];
    qT = new block128[length];
    if (!setup)
      setup_send();

    for (int j = 0; j < length / block_size; ++j) {
      for (int i = 0; i < lambda; ++i) {
        G0[i].random_data(q + (i * block_size / 128), block_size / 8);
        io->recv_data(tmp, block_size / 8);
        if (s[i])
          xorBlocks_arr(q + (i * block_size / 128), q + (i * block_size / 128),
                        tmp, block_size / 128);
      }
      sse_trans((uint8_t *)(qT + j * block_size), (uint8_t *)q, 128,
                block_size);
    }
    this->block_size = old_block_size;
  }

  void recv_pre(bool *r, int length) {
    int old_block_size = this->block_size;
    this->block_size =
        std::min(old_block_size, int(ceil(length / 256.0)) * 256);
    int old_length = length;
    length = padded_length(length);
    block128 t[block_size];
    tT = new block128[length];

    if (not setup)
      setup_recv();

    bool *r2 = new bool[length];
    prg.random_bool(extended_r, block_size);
    memcpy(r2, r, old_length);
    memcpy(r2 + old_length, extended_r, length - old_length);

    block128 *block_r = new block128[length / 128];
    for (int i = 0; i < length / 128; ++i) {
      block_r[i] = bool_to128(r2 + i * 128);
    }

    for (int j = 0; j * block_size < length; ++j) {
      for (int i = 0; i < lambda; ++i) {
        G0[i].random_data(t + (i * block_size / 128), block_size / 8);
        G1[i].random_data(tmp, block_size / 8);
        xorBlocks_arr(tmp, t + (i * block_size / 128), tmp, block_size / 128);
        xorBlocks_arr(tmp, block_r + (j * block_size / 128), tmp,
                      block_size / 128);
        io->send_data(tmp, block_size / 8);
      }
      sse_trans((uint8_t *)(tT + j * block_size), (uint8_t *)t, 128,
                block_size);
    }

    delete[] block_r;
    delete[] r2;

    this->block_size = old_block_size;
  }

  /*********************************************************
   *         Online Offline GOT functions                  *
   ********************************************************/

  void preprocess() {
    switch (party) {
    case ALICE: {
      send_pre(counter);
      got_send_offline(counter);
      break;
    }
    case BOB: {
      prg.random_data(r_off, counter);
      uint8_t mask = N - 1; // N is a power of 2
      for (int i = 0; i < counter; i++) {
        r_off[i] &= mask;
      }
      recv_pre((bool *)r_off, counter);
      got_recv_offline(counter);
      break;
    }
    }
    counter = 0;
  }

  void got_send_offline(int length) {
    const int bsize = AES_BATCH_SIZE;
    block128 *pad = new block128[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        h64[0][j] = ((uint64_t)_mm_extract_epi64(pad[2 * (j - i)], 0));
        h[0][j] = ((uint8_t)_mm_extract_epi8(pad[2 * (j - i)], 0));
        h64[1][j] = ((uint64_t)_mm_extract_epi64(pad[2 * (j - i) + 1], 0));
        h[1][j] = ((uint8_t)_mm_extract_epi8(pad[2 * (j - i) + 1], 0));
      }
    }
    delete[] qT;
    delete[] pad;
  }

  void got_recv_offline(int length) {
    const int bsize = AES_BATCH_SIZE;
    block128 *pad = new block128[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      if (bsize <= length - i)
        crh.H<bsize>(pad, tT + i);
      else
        crh.Hn(pad, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        h64[0][i + j] = ((uint64_t)_mm_extract_epi64(pad[j], 0));
        h[0][i + j] = ((uint8_t)_mm_extract_epi8(pad[j], 0));
      }
    }
    delete[] tT;
    delete[] pad;
  }

  template <typename T> void got_send_online(T **data, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    int bits_in_sel_input = 1;
    uint32_t y_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t a_size = (uint32_t)ceil((bsize * bits_in_sel_input) /
                                     ((float)sizeof(uint8_t) * 8));
    uint32_t corrected_bsize, corrected_y_size, corrected_a_size;
    ;
    T y[y_size];
    uint8_t a_packed[a_size];
    uint8_t a[length];
    T **maskeddata = new T *[bsize];
    for (int i = 0; i < bsize; i++) {
      maskeddata[i] = new T[2];
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

      for (uint32_t i = 0; i < corrected_bsize; i++) {
        for (int k = 0; k < 2; k++) {
          if (sizeof(T) == 8) {
            maskeddata[i][k] = (h64[k ^ a[i]][counter] ^ data[ctr + i][k]);
          } else if (sizeof(T) == 1) {
            maskeddata[i][k] = (h[k ^ a[i]][counter] ^ data[ctr + i][k]);
          } else {
            throw std::invalid_argument("Not implemented");
          }
        }
        counter++;
      }
      pack_messages<T>(y, maskeddata, corrected_y_size, corrected_bsize, l, 2);
      io->send_data(y, sizeof(T) * corrected_y_size);
    }
    for (int i = 0; i < bsize; i++) {
      delete[] maskeddata[i];
    }
    delete[] maskeddata;
  }

  template <typename T>
  void got_recv_online(T *data, const uint8_t *r, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    int bits_in_sel_input = 1;
    uint32_t res_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t a_size = (uint32_t)ceil((bsize * bits_in_sel_input) /
                                     ((float)sizeof(uint8_t) * 8));
    uint32_t corrected_bsize, corrected_res_size, corrected_a_size;
    ;
    T res[res_size];
    uint8_t a_unpacked[length];
    uint8_t a[a_size];
    for (int ctr = 0; ctr < length; ctr += bsize) {
      corrected_bsize = std::min(bsize, length - ctr);
      corrected_res_size =
          (uint32_t)ceil((N * corrected_bsize * l) / ((float)sizeof(T) * 8));
      corrected_a_size = (uint32_t)ceil((corrected_bsize * bits_in_sel_input) /
                                        ((float)sizeof(uint8_t) * 8));

      int counter_memory = counter;
      // Send corrected choice inputs
      for (uint32_t i = 0; i < corrected_bsize; i++) {
        a_unpacked[i] = r_off[counter++] ^ r[ctr + i];
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
                                  2, counter);
      } else if (sizeof(T) == 1) {
        unpack_messages<uint8_t>((uint8_t *)data + ctr, r + ctr, (uint8_t *)res,
                                 h[0], corrected_bsize, l, 2, counter);
      } else {
        throw std::invalid_argument("Not implemented");
      }
    }
  }

  /*********************************************************
   *                Normal GOT functions                  *
   ********************************************************/

  void got_send_post(const block128 *data0, const block128 *data1, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = xorBlocks(pad[2 * (j - i)], data0[j]);
        pad[2 * (j - i) + 1] = xorBlocks(pad[2 * (j - i) + 1], data1[j]);
      }
      io->send_data(pad, 2 * sizeof(block128) * std::min(bsize, length - i));
    }
    delete[] qT;
  }

  void got_recv_post(block128 *data, const bool *r, int length) {
    const int bsize = AES_BATCH_SIZE;
    block128 res[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      io->recv_data(res, 2 * sizeof(block128) * std::min(bsize, length - i));
      if (bsize <= length - i)
        crh.H<bsize>(tT + i, tT + i);
      else
        crh.Hn(tT + i, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        data[i + j] = xorBlocks(res[2 * j + r[i + j]], tT[i + j]);
      }
    }
    delete[] tT;
  }

  template <typename T> void got_send_post(T **data, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    uint32_t y_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t corrected_y_size, corrected_bsize;
    T y[y_size];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      corrected_y_size = (uint32_t)ceil(
          (2 * std::min(bsize, length - i) * this->l) / ((float)sizeof(T) * 8));
      corrected_bsize = std::min(bsize, length - i);
      if (sizeof(T) == 8) {
        pack_ot_messages<uint64_t>((uint64_t *)y, (uint64_t **)data + i, pad,
                                   corrected_y_size, corrected_bsize, this->l,
                                   2);
      } else if (sizeof(T) == 1) {
        pack_ot_messages<uint8_t>((uint8_t *)y, (uint8_t **)data + i, pad,
                                  corrected_y_size, corrected_bsize, this->l,
                                  2);
      } else {
        throw std::invalid_argument("Not implemented");
      }
      io->send_data(y, sizeof(T) * (corrected_y_size));
    }
    delete[] qT;
  }

  template <typename T>
  void got_recv_post(T *data, const uint8_t *r, int length) {
    const int bsize = AES_BATCH_SIZE;
    uint32_t recvd_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(T) * 8));
    uint32_t corrected_recvd_size, corrected_bsize;
    uint64_t recvd[recvd_size];

    for (int i = 0; i < length; i += bsize) {
      corrected_recvd_size = (uint32_t)ceil(
          (2 * std::min(bsize, length - i) * this->l) / ((float)sizeof(T) * 8));
      corrected_bsize = std::min(bsize, length - i);
      io->recv_data(recvd, sizeof(T) * (corrected_recvd_size));
      if (bsize <= length - i)
        crh.H<bsize>(tT + i, tT + i);
      else
        crh.Hn(tT + i, tT + i, length - i);
      if (sizeof(T) == 8) {
        unpack_ot_messages<uint64_t>((uint64_t *)data + i, r + i,
                                     (uint64_t *)recvd, tT + i, corrected_bsize,
                                     this->l, 2);
      } else if (sizeof(T) == 1) {
        unpack_ot_messages<uint8_t>((uint8_t *)data + i, r + i,
                                    (uint8_t *)recvd, tT + i, corrected_bsize,
                                    this->l, 2);
      } else {
        throw std::invalid_argument("Not implemented");
      }
    }
    delete[] tT;
  }

  // General OT sender with message length > 64
  void send_batched_got(uint64_t *data, int num_ot, int l,
                        int msgs_per_ot = 1) {
    this->l = l;
    send_pre(num_ot);

    int dim = num_ot;
    uint64_t modulo_mask = (l == 64 ? -1 : ((1ULL << l) - 1));
    int max_num_hashes = ceil((l * msgs_per_ot) / 128.0);
    int max_pad_len = dim * max_num_hashes;
    block128 *pad = new block128[2 * max_pad_len];
    block128 *y0_per_ot = new block128[msgs_per_ot];
    block128 *y1_per_ot = new block128[msgs_per_ot];
    uint8_t *y0 =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];
    uint8_t *y1 =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];

    int num_hashes = ceil((l * msgs_per_ot) / 128.0);
    int bsize = std::min(int(ceil(AES_BATCH_SIZE / double(num_hashes))), dim) *
                num_hashes;
    for (int j = 0; j < dim; j += (bsize / num_hashes)) {
      for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
        int ot_idx = k;
        int pad1_offset = std::min(bsize / num_hashes, dim - j) * num_hashes;
        for (int h = 0; h < num_hashes; h++) {
          pad[(k - j) * num_hashes + h] = xorBlocks(qT[ot_idx], toBlock(h));
          pad[pad1_offset + (k - j) * num_hashes + h] =
              xorBlocks(pad[(k - j) * num_hashes + h], block_s);
        }
      }
      if (bsize <= (dim - j) * num_hashes)
        crh.Hn(pad, pad, 2 * bsize);
      else
        crh.Hn(pad, pad, 2 * (dim - j) * num_hashes);

      int lnum_ot = std::min(bsize / num_hashes, dim - j);
      int32_t ysize_per_ot = ceil(msgs_per_ot * l / (8.0));

      for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
        int ot_idx = k;
        int pad1_offset = std::min(bsize / num_hashes, dim - j) * num_hashes;
        block128 *pad0_ptr = pad + (k - j) * num_hashes;
        block128 *pad1_ptr = pad + pad1_offset + (k - j) * num_hashes;
        for (int h = 0; h < msgs_per_ot; h++) {
          int msg_idx = ot_idx * msgs_per_ot + h;
          writeToPackedArr((uint8_t *)y0_per_ot, ysize_per_ot, h * l, l,
                           data[msg_idx * 2]);
          writeToPackedArr((uint8_t *)y1_per_ot, ysize_per_ot, h * l, l,
                           data[msg_idx * 2 + 1]);
        }
        for (int h = 0; h < num_hashes; h++) {
          y0_per_ot[h] = xorBlocks(y0_per_ot[h], pad0_ptr[h]);
          y1_per_ot[h] = xorBlocks(y1_per_ot[h], pad1_ptr[h]);
        }
        memcpy(y0 + (ysize_per_ot * (k - j)), y0_per_ot, ysize_per_ot);
        memcpy(y1 + (ysize_per_ot * (k - j)), y1_per_ot, ysize_per_ot);
      }
      io->send_data(y0, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
      io->send_data(y1, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
    }
    delete[] pad;
    // delete[] unpacked_pad0;
    // delete[] unpacked_pad1;
    // delete[] corr_data;
    delete[] y0;
    delete[] y0_per_ot;
    delete[] y1;
    delete[] y1_per_ot;
    delete[] qT;

    /*
            const int bsize = AES_BATCH_SIZE/2;
            block128 pad[2*bsize];
            uint32_t y_size = (uint32_t)ceil((2*bsize*l)/(float(64)));
            uint32_t corrected_y_size, corrected_bsize;
            uint64_t y[y_size];
            for(int i = 0; i < num_ot*msgs_per_ot; i+=bsize) {
                    for(int j = i; j < i+bsize and j < num_ot*msgs_per_ot; ++j)
       { int ot_idx = j/msgs_per_ot; int hash_idx = j % msgs_per_ot;
                            pad[2*(j-i)] = xorBlocks(qT[ot_idx],
       toBlock(hash_idx)); pad[2*(j-i)+1] = xorBlocks(pad[2*(j-i)], block_s);
                    }
                    crh.H<2*bsize>(pad, pad);
                    corrected_y_size =
       ceil((2*std::min(bsize,num_ot*msgs_per_ot-i)*l)/(float(64)));
                    corrected_bsize = std::min(bsize, num_ot*msgs_per_ot-i);
        pack_ot_messages<uint64_t>(y, data+i, pad, corrected_y_size,
       corrected_bsize, l, 2); io->send_data(y,
       sizeof(uint64_t)*(corrected_y_size));
            }
            delete[] qT;
    */
  }

  // General OT receiver with message length > 64
  void recv_batched_got(uint64_t *data, const uint8_t *r, int num_ot, int l,
                        int msgs_per_ot = 1) {
    this->l = l;
    recv_pre((bool *)r, num_ot);

    int dim = num_ot;
    uint64_t modulo_mask = (l == 64 ? -1 : ((1ULL << l) - 1));

    int max_num_hashes = ceil((l * msgs_per_ot) / 128.0);
    int max_pad_len = dim * max_num_hashes;
    block128 *pad = new block128[max_pad_len];
    uint8_t *y0 =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];
    uint8_t *y1 =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];

    int num_hashes = ceil((l * msgs_per_ot) / 128.0);
    int bsize = std::min(int(ceil(AES_BATCH_SIZE / double(num_hashes))), dim) *
                num_hashes;
    for (int j = 0; j < dim; j += (bsize / num_hashes)) {
      int lnum_ot = std::min(bsize / num_hashes, dim - j);
      int32_t ysize_per_ot = ceil(msgs_per_ot * l / (8.0));

      io->recv_data(y0, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
      io->recv_data(y1, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
      for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
        int ot_idx = k;
        for (int h = 0; h < num_hashes; h++) {
          pad[(k - j) * num_hashes + h] = xorBlocks(tT[ot_idx], toBlock(h));
        }
      }
      if (bsize <= (dim - j) * num_hashes)
        crh.Hn(pad, pad, bsize);
      else
        crh.Hn(pad, pad, (dim - j) * num_hashes);
      for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
        int ot_idx = k;
        block128 *pad_ptr = pad + (k - j) * num_hashes;
        block128 *y0_ptr = (block128 *)(y0 + (ysize_per_ot * (k - j)));
        block128 *y1_ptr = (block128 *)(y1 + (ysize_per_ot * (k - j)));
        if (r[ot_idx]) {
          for (int h = 0; h < num_hashes; h++) {
            pad_ptr[h] = xorBlocks(pad_ptr[h], _mm_loadu_si128(y1_ptr + h));
          }
        } else {
          for (int h = 0; h < num_hashes; h++) {
            pad_ptr[h] = xorBlocks(pad_ptr[h], _mm_loadu_si128(y0_ptr + h));
          }
        }
        for (int h = 0; h < msgs_per_ot; h++) {
          int msg_idx = ot_idx * msgs_per_ot + h;
          int corr_idx = (k - j) * msgs_per_ot + h;
          data[msg_idx] =
              readFromPackedArr((uint8_t *)pad_ptr, num_hashes, h * l, l);
        }
      }
    }
    delete[] pad;
    delete[] y0;
    delete[] y1;
    delete[] tT;

    /*
            const int bsize = AES_BATCH_SIZE;
            uint32_t recvd_size = (uint32_t)ceil((2*bsize*l)/(float(64)));
            uint32_t corrected_recvd_size, corrected_bsize;
            uint64_t recvd[recvd_size];
    block128 pad[bsize];
    uint8_t* r_ext = new uint8_t[num_ot*msgs_per_ot];
    for (int i = 0; i < num_ot; i++) {
        memset(r_ext + i*msgs_per_ot, r[i], msgs_per_ot);
    }
            for(int i = 0; i < num_ot*msgs_per_ot; i+=bsize) {
                    corrected_recvd_size =
    ceil((2*std::min(bsize,num_ot*msgs_per_ot-i)*l)/(float(64)));
                    corrected_bsize = std::min(bsize, num_ot*msgs_per_ot-i);
                    io->recv_data(recvd,
    sizeof(uint64_t)*(corrected_recvd_size)); for(int j = i; j < i+bsize and j <
    num_ot*msgs_per_ot; ++j) { int ot_idx = j/msgs_per_ot; int hash_idx = j %
    msgs_per_ot; pad[j-i] = xorBlocks(tT[ot_idx], toBlock(hash_idx));
                    }
                    if (bsize <= num_ot*msgs_per_ot-i) crh.H<bsize>(pad, pad);
                    else crh.Hn(pad, pad, num_ot*msgs_per_ot-i);
        unpack_ot_messages<uint64_t>(data+i, r_ext+i, recvd, pad,
    corrected_bsize, l, 2);
            }
            delete[] tT;
    delete[] r_ext;
    */
  }

  /*********************************************************
   *                   COT functions                       *
   ********************************************************/

  void cot_send_post(uint64_t *data0, uint64_t *corr, int length) {
    uint64_t modulo_mask = (1ULL << this->l) - 1;
    if (this->l == 64)
      modulo_mask = -1;
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    uint32_t y_size = (uint32_t)ceil((bsize * this->l) / (float(64)));
    uint32_t corrected_y_size, corrected_bsize;
    uint64_t y[y_size];
    uint64_t corr_data[bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        data0[j] = _mm_extract_epi64(pad[2 * (j - i)], 0) & modulo_mask;
        corr_data[j - i] =
            (corr[j] + data0[j] + _mm_extract_epi64(pad[2 * (j - i) + 1], 0)) &
            modulo_mask;
      }
      corrected_y_size =
          (uint32_t)ceil((std::min(bsize, length - i) * this->l) /
                         ((float)sizeof(uint64_t) * 8));
      corrected_bsize = std::min(bsize, length - i);
      pack_cot_messages(y, corr_data, corrected_y_size, corrected_bsize,
                        this->l);
      io->send_data(y, sizeof(uint64_t) * (corrected_y_size));
    }
    delete[] qT;
  }

  void cot_recv_post(uint64_t *data, const bool *r, int length) {
    uint64_t modulo_mask = (1ULL << this->l) - 1;
    if (this->l == 64)
      modulo_mask = -1;
    const int bsize = AES_BATCH_SIZE;
    uint32_t recvd_size = (uint32_t)ceil((bsize * this->l) / (float(64)));
    uint32_t corrected_recvd_size, corrected_bsize;
    uint64_t corr_data[bsize];
    uint64_t recvd[recvd_size];

    for (int i = 0; i < length; i += bsize) {
      corrected_recvd_size =
          (uint32_t)ceil((std::min(bsize, length - i) * this->l) / (float(64)));
      corrected_bsize = std::min(bsize, length - i);
      io->recv_data(recvd, sizeof(uint64_t) * corrected_recvd_size);
      if (bsize <= length - i)
        crh.H<bsize>(tT + i, tT + i);
      else
        crh.Hn(tT + i, tT + i, length - i);
      unpack_cot_messages(corr_data, recvd, corrected_bsize, this->l);
      for (int j = i; j < i + bsize and j < length; ++j) {
        if (r[j])
          data[j] =
              (corr_data[j - i] - _mm_extract_epi64(tT[j], 0)) & modulo_mask;
        else
          data[j] = _mm_extract_epi64(tT[j], 0) & modulo_mask;
      }
    }
    delete[] tT;
  }

  // Batched COT sender with messages of different bitlengths
  // msgs_per_ot specifies the number of OTs with same choice bit (to be
  // batched)
  void send_batched_cot(uint64_t *data0, uint64_t *corr,
                        std::vector<int> msg_len, int num_ot,
                        int msgs_per_ot = 1) {
    send_pre(num_ot);

    int num_msg_len = msg_len.size();
    // The number of OTs of a particular message bitlength
    // Simplifying assumption: Each message bitlength has equal number of OTs
    int dim = num_ot / num_msg_len;
    uint64_t modulo_mask[num_msg_len];
    for (int bit_idx = 0; bit_idx < num_msg_len; bit_idx++) {
      modulo_mask[bit_idx] =
          (msg_len[bit_idx] == 64 ? -1 : ((1ULL << msg_len[bit_idx]) - 1));
    }
    int max_num_hashes = ceil((64 * msgs_per_ot) / 128.0);
    int max_pad_len = dim * max_num_hashes;
    block128 *pad = new block128[2 * max_pad_len];
    block128 *y_per_ot = new block128[msgs_per_ot];
    // uint64_t* unpacked_pad0 = new uint64_t[msgs_per_ot];
    // uint64_t* unpacked_pad1 = new uint64_t[msgs_per_ot];
    // uint64_t* corr_data = new uint64_t[dim*msgs_per_ot];
    uint8_t *y =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];
    for (int i = 0; i < num_ot / dim; i++) {
      int bit_idx = i;
      int lmsg_len = msg_len[bit_idx];
      uint64_t lmodulo_mask = modulo_mask[bit_idx];
      int num_hashes = ceil((lmsg_len * msgs_per_ot) / 128.0);
      int bsize =
          std::min(int(ceil(AES_BATCH_SIZE / double(num_hashes))), dim) *
          num_hashes;
      for (int j = 0; j < dim; j += (bsize / num_hashes)) {
        for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
          int ot_idx = i * dim + k;
          int pad1_offset = std::min(bsize / num_hashes, dim - j) * num_hashes;
          for (int h = 0; h < num_hashes; h++) {
            pad[(k - j) * num_hashes + h] = xorBlocks(qT[ot_idx], toBlock(h));
            pad[pad1_offset + (k - j) * num_hashes + h] =
                xorBlocks(pad[(k - j) * num_hashes + h], block_s);
          }
        }
        if (bsize <= (dim - j) * num_hashes)
          crh.Hn(pad, pad, 2 * bsize);
        else
          crh.Hn(pad, pad, 2 * (dim - j) * num_hashes);

        int lnum_ot = std::min(bsize / num_hashes, dim - j);
        int32_t ysize_per_ot = ceil(msgs_per_ot * lmsg_len / (8.0));

        for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
          int ot_idx = i * dim + k;
          int pad1_offset = std::min(bsize / num_hashes, dim - j) * num_hashes;
          block128 *pad0_ptr = pad + (k - j) * num_hashes;
          block128 *pad1_ptr = pad + pad1_offset + (k - j) * num_hashes;
          for (int h = 0; h < msgs_per_ot; h++) {
            int msg_idx = ot_idx * msgs_per_ot + h;
            int32_t corr_idx = (k - j) * msgs_per_ot + h;
            uint64_t unpacked_pad0 = readFromPackedArr(
                (uint8_t *)pad0_ptr, num_hashes, h * lmsg_len, lmsg_len);
            // unpacked_pad1[h] = readFromPackedArr((uint8_t*)pad1_ptr,
            // num_hashes,
            //         h*msg_len[bit_idx], msg_len[bit_idx]);
            data0[msg_idx] = unpacked_pad0 & lmodulo_mask;
            uint64_t corr_data = (corr[msg_idx] + unpacked_pad0) & lmodulo_mask;
            writeToPackedArr((uint8_t *)y_per_ot, ysize_per_ot, h * lmsg_len,
                             lmsg_len, corr_data);
          }
          for (int h = 0; h < num_hashes; h++) {
            y_per_ot[h] = xorBlocks(y_per_ot[h], pad1_ptr[h]);
          }
          memcpy(y + (ysize_per_ot * (k - j)), y_per_ot, ysize_per_ot);
        }
        io->send_data(y, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
      }
    }
    delete[] pad;
    // delete[] unpacked_pad0;
    // delete[] unpacked_pad1;
    // delete[] corr_data;
    delete[] y;
    delete[] y_per_ot;
    delete[] qT;
  }

  // Batched COT receiver with messages of different bitlengths
  // msgs_per_ot specifies the number of OTs with same choice bit (to be
  // batched)
  void recv_batched_cot(uint64_t *data, bool *b, std::vector<int> msg_len,
                        int num_ot, int msgs_per_ot = 1) {
    recv_pre(b, num_ot);

    int num_msg_len = msg_len.size();
    // The number of OTs of a particular message bitlength
    // Simplifying assumption: Each message bitlength has equal number of OTs
    int dim = num_ot / num_msg_len;
    uint64_t modulo_mask[num_msg_len];
    for (int bit_idx = 0; bit_idx < num_msg_len; bit_idx++) {
      modulo_mask[bit_idx] =
          (msg_len[bit_idx] == 64 ? -1 : ((1ULL << msg_len[bit_idx]) - 1));
    }
    int max_num_hashes = ceil((64 * msgs_per_ot) / 128.0);
    int max_pad_len = dim * max_num_hashes;
    block128 *pad = new block128[max_pad_len];
    // uint64_t* unpacked_pad = new uint64_t[msgs_per_ot];
    // uint64_t* corr_data = new uint64_t[dim*msgs_per_ot];
    uint8_t *y =
        new uint8_t[dim * msgs_per_ot * (sizeof(uint64_t) / sizeof(uint8_t))];
    for (int i = 0; i < num_ot / dim; i++) {
      int bit_idx = i;
      int lmsg_len = msg_len[bit_idx];
      int num_hashes = ceil((lmsg_len * msgs_per_ot) / 128.0);
      int bsize =
          std::min(int(ceil(AES_BATCH_SIZE / double(num_hashes))), dim) *
          num_hashes;
      for (int j = 0; j < dim; j += (bsize / num_hashes)) {
        int lnum_ot = std::min(bsize / num_hashes, dim - j);
        int32_t ysize_per_ot = ceil(msgs_per_ot * lmsg_len / (8.0));

        io->recv_data(y, sizeof(uint8_t) * ysize_per_ot * lnum_ot);
        for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
          int ot_idx = i * dim + k;
          for (int h = 0; h < num_hashes; h++) {
            pad[(k - j) * num_hashes + h] = xorBlocks(tT[ot_idx], toBlock(h));
          }
        }
        if (bsize <= (dim - j) * num_hashes)
          crh.Hn(pad, pad, bsize);
        else
          crh.Hn(pad, pad, (dim - j) * num_hashes);
        for (int k = j; k < j + (bsize / num_hashes) and k < dim; k++) {
          int ot_idx = i * dim + k;
          block128 *pad_ptr = pad + (k - j) * num_hashes;
          block128 *y_ptr = (block128 *)(y + (ysize_per_ot * (k - j)));
          if (b[ot_idx]) {
            for (int h = 0; h < num_hashes; h++) {
              pad_ptr[h] = xorBlocks(pad_ptr[h], _mm_loadu_si128(y_ptr + h));
            }
          }
          for (int h = 0; h < msgs_per_ot; h++) {
            int msg_idx = ot_idx * msgs_per_ot + h;
            int corr_idx = (k - j) * msgs_per_ot + h;
            uint64_t unpacked_pad = readFromPackedArr(
                (uint8_t *)pad_ptr, num_hashes, h * lmsg_len, lmsg_len);
            data[msg_idx] = unpacked_pad; // & modulo_mask[bit_idx];
          }
        }
      }
    }
    delete[] pad;
    // delete[] unpacked_pad;
    // delete[] corr_data;
    delete[] y;
    delete[] tT;
  }

  /*********************************************************
   *            Send/Recv wrapper functions                *
   ********************************************************/

  void send_impl(const block128 *data0, const block128 *data1, int length) {
    send_pre(length);
    got_send_post(data0, data1, length);
  }

  void recv_impl(block128 *data, const bool *b, int length) {
    recv_pre((bool *)b, length);
    got_recv_post(data, b, length);
  }

  void send_impl(uint64_t **data, int length, int l) {
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
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_recv_online<uint64_t>(data, b, length);
    } else {
      recv_pre((bool *)b, length);
      got_recv_post<uint64_t>(data, b, length);
    }
  }

  void send_impl(uint8_t **data, int length, int l) {
    assert(l <= 8 && l >= 1);
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
    assert(l <= 8 && l >= 1);
    this->l = l;
    if (length <= precomp_batch_size) {
      if (length > (precomp_batch_size - counter)) {
        preprocess();
      }
      got_recv_online<uint8_t>(data, b, length);
    } else {
      recv_pre((bool *)b, length);
      got_recv_post<uint8_t>(data, b, length);
    }
  }

  void send_cot(uint64_t *data0, uint64_t *corr, int length, int l) {
    this->l = l;
    send_pre(length);
    cot_send_post(data0, corr, length);
  }

  void recv_cot(uint64_t *data, bool *b, int length, int l) {
    this->l = l;
    recv_pre(b, length);
    cot_recv_post(data, b, length);
  }
};
} // namespace primihub::sci
#endif // SPLIT_OT_IKNP_H__
