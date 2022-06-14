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

Modified by Nishant Kumar
*/

#ifndef OT_IKNP_H__
#define OT_IKNP_H__
#include "src/primihub/protocol/cryptflow2/OT/np.h"
#include "src/primihub/protocol/cryptflow2/OT/ot.h"
#include <algorithm>
namespace primihub::sci {
template <typename IO> class IKNP : public OT<IKNP<IO>> {
public:
  OTNP<IO> *base_ot;
  PRG128 prg;
  const int lambda = 128;
  const int block_size = 1024 * 16;
  int l;

  block128 *k0 = nullptr, *k1 = nullptr, *qT = nullptr, *tT = nullptr,
           *tmp = nullptr, block_s;
  PRG128 *G0, *G1;
  bool *s = nullptr, *extended_r = nullptr, setup = false;
  IO *io = nullptr;
  CRH crh;

  IKNP(IO *io) {
    this->io = io;
    base_ot = new OTNP<IO>(io);
    s = new bool[lambda];
    k0 = new block128[lambda];
    k1 = new block128[lambda];
    G0 = new PRG128[lambda];
    G1 = new PRG128[lambda];
    tmp = new block128[block_size / 128];
    extended_r = new bool[block_size];
  }

  ~IKNP() {
    delete base_ot;
    delete[] s;
    delete[] k0;
    delete[] k1;
    delete[] G0;
    delete[] G1;
    delete[] tmp;
    delete[] extended_r;
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
  }

  void recv_pre(const bool *r, int length) {
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
  }

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

  void got_send_post(uint64_t **data, int length) {
    assert(this->l <= 64 && this->l > 8);
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    uint32_t pad2_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(uint64_t) * 8));
    uint64_t pad2[pad2_size];
    int start_pos = 0;
    int end_pos = 0;
    int start_block64 = 0;
    int end_block64 = 0;
    uint64_t temp_bl = 0;
    uint64_t mask;
    if (this->l < 64) {
      mask = (1ULL << this->l) - 1ULL;
    } else {
      mask = -1ULL;
    }
    int temp = 0;
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      for (int j = 0; j < (int)pad2_size; ++j) {
        pad2[j] = 0ULL;
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        // OT message 0
        start_pos = this->l * 2 * (j - i); // inclusive
        end_pos = start_pos + this->l;     // exclusive
        end_pos -= 1;                      // inclusive
        start_block64 = start_pos / (8 * sizeof(uint64_t));
        end_block64 = end_pos / (8 * sizeof(uint64_t));
        if (start_block64 == end_block64) {
          pad2[start_block64] ^=
              ((((uint64_t)_mm_extract_epi64(pad[2 * (j - i)], 0)) ^
                data[j][0]) &
               mask)
              << (start_pos % (8 * sizeof(uint64_t)));
        } else {
          temp_bl = ((((uint64_t)_mm_extract_epi64(pad[2 * (j - i)], 0)) ^
                      data[j][0]) &
                     mask);
          pad2[start_block64] ^= (temp_bl)
                                 << (start_pos % (8 * sizeof(uint64_t)));
          pad2[end_block64] ^=
              (temp_bl) >>
              ((8 * sizeof(uint64_t)) - (start_pos % (8 * sizeof(uint64_t))));
        }
        // OT message 1
        start_pos = this->l * 2 * (j - i) + this->l; // inclusive
        end_pos = start_pos + this->l;               // exclusive
        end_pos -= 1;                                // inclusive
        start_block64 = start_pos / (8 * sizeof(uint64_t));
        end_block64 = end_pos / (8 * sizeof(uint64_t));
        if (start_block64 == end_block64) {
          pad2[start_block64] ^=
              ((((uint64_t)_mm_extract_epi64(pad[2 * (j - i) + 1], 0)) ^
                data[j][1]) &
               mask)
              << (start_pos % (8 * sizeof(uint64_t)));
        } else {
          temp_bl = ((((uint64_t)_mm_extract_epi64(pad[2 * (j - i) + 1], 0)) ^
                      data[j][1]) &
                     mask);
          pad2[start_block64] ^= (temp_bl)
                                 << (start_pos % (8 * sizeof(uint64_t)));
          pad2[end_block64] ^=
              (temp_bl) >>
              ((8 * sizeof(uint64_t)) - (start_pos % (8 * sizeof(uint64_t))));
        }
        temp = (temp > end_block64) ? temp : end_block64;
      }
      uint32_t pad2_size_correct =
          (uint32_t)ceil((2 * std::min(bsize, length - i) * this->l) /
                         ((float)sizeof(uint64_t) * 8));
      io->send_data(pad2, sizeof(uint64_t) * (pad2_size_correct));
    }
    delete[] qT;
  }

  void got_recv_post(uint64_t *data, const uint8_t *r, int length) {
    assert(this->l <= 64 && this->l > 8);
    const int bsize = AES_BATCH_SIZE;
    uint32_t res_size =
        (uint32_t)ceil((2 * bsize * this->l) / ((float)sizeof(uint64_t) * 8));
    uint64_t res[res_size];
    int start_pos = 0;
    int end_pos = 0;
    int start_block64 = 0;
    int end_block64 = 0;
    uint64_t mask;
    if (this->l < 64) {
      mask = (1ULL << this->l) - 1ULL;
    } else {
      mask = -1ULL;
    }
    for (int i = 0; i < length; i += bsize) {
      uint32_t res_size_correct =
          (uint32_t)ceil((2 * std::min(bsize, length - i) * this->l) /
                         ((float)sizeof(uint64_t) * 8));
      io->recv_data(res, sizeof(uint64_t) * (res_size_correct));
      if (bsize <= length - i)
        crh.H<bsize>(tT + i, tT + i);
      else
        crh.Hn(tT + i, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        start_pos = 2 * j * this->l + r[i + j] * this->l; // inclusive
        end_pos = start_pos + this->l - 1;                // inclusive
        start_block64 = start_pos / (8 * sizeof(uint64_t));
        end_block64 = end_pos / (8 * sizeof(uint64_t));
        if (start_block64 == end_block64) {
          data[i + j] =
              (((res[start_block64] >> (start_pos % (8 * sizeof(uint64_t))))) ^
               ((uint64_t)_mm_extract_epi64(tT[i + j], 0))) &
              mask;
        } else {
          data[i + j] = 0ULL;
          data[i + j] ^=
              (res[start_block64] >> (start_pos % (8 * sizeof(uint64_t))));
          data[i + j] ^=
              (res[end_block64] << (8 * sizeof(uint64_t) -
                                    (start_pos % (8 * sizeof(uint64_t)))));
          data[i + j] =
              (data[i + j] ^ ((uint64_t)_mm_extract_epi64(tT[i + j], 0))) &
              mask;
        }
      }
    }
    delete[] tT;
  }

  void cot_send_post(block128 *data0, block128 delta, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    block128 tmp[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        data0[j] = pad[2 * (j - i)];
        pad[2 * (j - i)] = xorBlocks(pad[2 * (j - i)], delta);
        tmp[j - i] = xorBlocks(pad[2 * (j - i) + 1], pad[2 * (j - i)]);
      }
      io->send_data(tmp, sizeof(block128) * std::min(bsize, length - i));
    }
    delete[] qT;
  }

  void cot_recv_post(block128 *data, const bool *r, int length) {
    const int bsize = AES_BATCH_SIZE;
    block128 res[bsize];
    for (int i = 0; i < length; i += bsize) {
      io->recv_data(res, sizeof(block128) * std::min(bsize, length - i));
      if (bsize <= length - i)
        crh.H<bsize>(data + i, tT + i);
      else
        crh.Hn(data + i, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        if (r[i + j])
          data[i + j] = xorBlocks(res[j], data[i + j]);
      }
    }
    delete[] tT;
  }

  void rot_send_post(block128 *data0, block128 *data1, int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        data0[j] = pad[2 * (j - i)];
        data1[j] = pad[2 * (j - i) + 1];
      }
    }
    delete[] qT;
  }

  void rot_recv_post(block128 *data, const bool *r, int length) {
    const int bsize = AES_BATCH_SIZE;
    for (int i = 0; i < length; i += bsize) {
      if (bsize <= length - i)
        crh.H<bsize>(data + i, tT + i);
      else
        crh.Hn(data + i, tT + i, length - i);
    }
    delete[] tT;
  }

  void send_impl(const block128 *data0, const block128 *data1, int length) {
    if (length < 1)
      return;
    send_pre(length);
    got_send_post(data0, data1, length);
  }

  void recv_impl(block128 *data, const bool *b, int length) {
    if (length < 1)
      return;
    recv_pre(b, length);
    got_recv_post(data, b, length);
  }

  void send_impl(uint64_t **data, int length, int l) {
    if (length < 1)
      return;
    this->l = l;
    send_pre(length);
    got_send_post(data, length);
  }

  void recv_impl(uint64_t *data, const uint8_t *b, int length, int l) {
    if (length < 1)
      return;
    this->l = l;
    recv_pre((bool *)b, length);
    got_recv_post(data, b, length);
  }

  void send_cot(block128 *data0, block128 delta, int length) {
    if (length < 1)
      return;
    send_pre(length);
    cot_send_post(data0, delta, length);
  }

  void recv_cot(block128 *data, const bool *b, int length) {
    if (length < 1)
      return;
    recv_pre(b, length);
    cot_recv_post(data, b, length);
  }

  void send_rot(block128 *data0, block128 *data1, int length) {
    if (length < 1)
      return;
    send_pre(length);
    rot_send_post(data0, data1, length);
  }

  void recv_rot(block128 *data, const bool *b, int length) {
    if (length < 1)
      return;
    recv_pre(b, length);
    rot_recv_post(data, b, length);
  }

  template <typename intType>
  void cot_send_post_matmul(intType *rdata, const intType *corr,
                            const uint32_t *chunkSizes,
                            const uint32_t *numChunks, const int numOTs,
                            int senderMatmulDims) {
    // TODO(nishkum): replace this maxing logic by replacing array of numChunks
    // by a constant (row size of sender)
    uint64_t maxHashLen = 0;
    for (int i = 0; i < numOTs; i++) {
      maxHashLen =
          std::max(maxHashLen,
                   uint64_t(2 * ceil_val(chunkSizes[i] * numChunks[i], 128)));
    }

    block128 hashArr[maxHashLen +
                     1]; // First half of this will contain H(q_i) and second
                         // half H(q_i \xor s) The +1 is there because
                         // readFromPackedArr will be called on this arry
                         // and while reading it reads a uint64_t directly. So,
                         // to leave sufficient space ahead and to prevent
                         // reading out of bounds, the +1.
    block128 corrHashArr
        [(maxHashLen / 2) +
         1]; // Since maxHashLen is max of even values, its divisible by 2
             // This will contain the final packed correlation(H(q_i))
             // The +1 is there because writePackedArr requires one extra
             // uint64_t space at end.
    block128 scratchArr[maxHashLen /
                        2]; // This will temporarily contain the value to be
                            // copied in dataToBeSent
                            // Since one OT is treated as atomic (as in fully
                            // sent or not sent),
                            //  need some bound on the data to be sent.
                            //  Maximum #blocks coming from one OT is
                            //  ceil(numChunks*bitlen/128). Taking bitlen as
                            //  maximum = 64, we get ceil(numChunks/2). So,
                            //  assuming numChunks<=256, we get 128 -- which is
                            //  the maximum number of 128 bit blocks which can
                            //  get added in one OT. So, declare an array of
                            //  size AES_BATCH_SIZE + 130.
    block128 *dataToBeSent =
        new block128[AES_BATCH_SIZE + (senderMatmulDims / 2) + 10];
    uint64_t dataToBeSentByteAlignedPtr = 0;
    uint64_t corrPtr = 0;
    for (int i = 0; i < numOTs; i++) {
      uint64_t curNumChunks = numChunks[i];
      uint64_t curChunkSize = chunkSizes[i];
      uint64_t curNumHashes = ceil_val(curNumChunks * curChunkSize, 128);
      for (uint64_t j = 0; j < curNumHashes; j++) {
        hashArr[j] = xorBlocks(qT[i], toBlock(j));
        hashArr[j + curNumHashes] = xorBlocks(hashArr[j], block_s);
      }
      crh.Hn(hashArr, hashArr, 2 * curNumHashes);
      // Now calculate f(Hash), where f represents the correlation
      for (uint64_t j = 0; j < curNumChunks; j++) {
        // read from corrPtr onwards for curNumChunks uint64s
        uint64_t curCorrVal = corr[corrPtr + j];
        // Now extract the corresponding random value from hashArr
        uint64_t curRandVal =
            readFromPackedArr((uint8_t *)hashArr, 16 * curNumHashes,
                              j * curChunkSize, curChunkSize);
        uint64_t curFinalVal =
            (curRandVal + curCorrVal) & (all1Mask(curChunkSize));
        // Now pack this curFinalVal in data to be xored with H(q \xor s)
        writeToPackedArr((uint8_t *)corrHashArr, 16 * curNumHashes,
                         j * curChunkSize, curChunkSize, curFinalVal);
        rdata[corrPtr + j] = (intType)curRandVal;
      }
      // The packed correlated value is ready. Now xor with H(q_i \xor s) and
      // then its ready to be sent
      for (uint64_t j = 0; j < curNumHashes; j++) {
        scratchArr[j] = xorBlocks(corrHashArr[j], hashArr[curNumHashes + j]);
      }
      uint64_t bytesToBeSent = ceil_val(curNumChunks * curChunkSize,
                                        8); // Byte align data to be sent
      memcpy(((uint8_t *)dataToBeSent) + dataToBeSentByteAlignedPtr, scratchArr,
             bytesToBeSent);
      dataToBeSentByteAlignedPtr += bytesToBeSent;
      corrPtr += curNumChunks;

      // If either enough data is accumulated or this is the last OT, send the
      // data
      if ((dataToBeSentByteAlignedPtr >= (AES_BATCH_SIZE * 16)) ||
          (i == numOTs - 1)) { // Each 128 bit block has 16 bytes
        // Send this data to receiver for pipelining
        io->send_data(dataToBeSent, dataToBeSentByteAlignedPtr);
        io->flush();
        dataToBeSentByteAlignedPtr = 0;
      }
    }
    delete[] dataToBeSent;
    delete[] qT;
  }

  template <typename intType>
  void cot_recv_post_matmul(intType *data, const uint8_t *choices,
                            const uint32_t *chunkSizes,
                            const uint32_t *numChunks, const int numOTs,
                            int senderMatmulDims) {
    block128 dataToBeRecvd[AES_BATCH_SIZE + (senderMatmulDims / 2) +
                           10]; // For logic of this bound, refer to the
                                // function above.

    /*
            Logic for the bound used in the following line:
            In each OT, except for the last hash evluation, whatever is hashed
       is also accounted for in the data sent. It is only in the last hash
       evaluation for that OT that data to be sent < data to be stored (for
       example, if data to be sent is 1 byte misaligned). So, at worst, the
       maximum amount of extra blocks needed = #OTs which are processed before
       sending < #totalOTs. Hence, the bound. And it is not that bad either,
       assuming numOTs = 2048*64 (for row side of right side matrix of receiver
       of size 2048 in 64 bits) mem consumption of this array =
       ((2048+130+(2048*64))*16)/(1<<20) MiB = 2.03 MiB.

            Also, since this can be a large array, allocating on heap is better.
    */
    block128 *hashesStored =
        new block128[AES_BATCH_SIZE + (senderMatmulDims / 2) + 10 + numOTs];
    uint64_t hashesStoredPtr =
        0; // Indexes into hashesStored to keep track of which hash block to be
           // used to start storing hashes
           // for given OT. This is used in the outer loop to fill in hashes,
           // which gets consumed in the inner loop when enough data is present.
    uint64_t dataToBeRecvdCummulativeCtr =
        0; // Keeps track of how much data is expected to be received.
           // When budget is full, it receives the expected amount of data.
           // This should work because sender and receiver follow the same
           // deterministic logic of calculating the amount of data to be
           // sent/received.
    int otDataStartCtr = 0; // Represents the OT number from which data is being
                            // collected in dataToBeRecvd When time comes, data
                            // received is OT data of OT #[otDataStartCtr,i]
    uint64_t dataPtr =
        0; // Indexes into data to keep track that when OT data starts to be
           // unpacked, which index in data represents its start
    for (int i = 0; i < numOTs; i++) {
      uint64_t curNumChunks = numChunks[i];
      uint64_t curChunkSize = chunkSizes[i];
      uint64_t curNumHashes = ceil_val(curNumChunks * curChunkSize, 128);

      // For current OT, the hashes need to be stored in hashesStored array
      // starting at idx = hashesStoredPtr.
      for (uint64_t j = 0; j < curNumHashes; j++) {
        hashesStored[hashesStoredPtr + j] = xorBlocks(tT[i], toBlock(j));
      }
      crh.Hn(hashesStored + hashesStoredPtr, hashesStored + hashesStoredPtr,
             curNumHashes);
      uint64_t bytesToBeRecvd = ceil_val(curNumChunks * curChunkSize, 8);
      dataToBeRecvdCummulativeCtr += bytesToBeRecvd;
      hashesStoredPtr += curNumHashes;
      if ((dataToBeRecvdCummulativeCtr >= (AES_BATCH_SIZE * 16)) ||
          (i == numOTs - 1)) { // Each 128 bit block has 16 bytes
        // Expect these many bytes from the sender
        io->recv_data(dataToBeRecvd, dataToBeRecvdCummulativeCtr);
        // Now that data has been received for past OTs, process those OTs.
        //  The OTs to be processed are from [otDataStartCtr,i]
        assert(otDataStartCtr <= i);
        uint64_t hashBlockCtr =
            0; // The hashes of the OTs from #[otDataStartCtr,i] have already
               // been calculated.
               // We only need to consume them now.
               // This variable indexes into hashesStored.
        uint64_t dataRecvByteAlignedCtr =
            0; // Indexes into dataToBeRecvd to keep track of from where the
               // data needs to be read
               // for current OT. Note that this variable represents byte
               // aligned read.
        for (int j = otDataStartCtr; j <= i; j++) {
          uint64_t curOTChunkSize = chunkSizes[j];
          uint64_t curOTNumChunks = numChunks[j];
          uint64_t numHashBlocks =
              ceil_val(curOTChunkSize * curOTNumChunks, 128);
          block128 *curOTRecvDataStartBlock =
              (block128 *)(((uint8_t *)dataToBeRecvd) + dataRecvByteAlignedCtr);
          if (choices[j] == 1) {
            for (uint64_t k = 0; k < numHashBlocks; k++) {
              // Following is a simple trick.
              //  curOTRecvDataStartBlock is a block128* starting at the byte
              //  from which the data of current OT is expected. Rem that all OT
              //  data is packed with byte alignment. So, this casting to
              //  block128* should work. We read in chunks of block128 from this
              //  array. But also rem that this array wasn't designed for
              //  block128 read. However, I claim that this should still work.
              //  This is because: at worst for a middle OT, the next OT's data
              //  will get hashed. However, while reading, only required amount
              //  of data will get read. So, all good here. For the last OT
              //  also, nothing bad should happen because dataToBeRecvd is
              //  created with at least one more extra block128 than required.
              //  Hence, this should work and no seg fault should come.
              hashesStored[hashBlockCtr + k] =
                  xorBlocks(hashesStored[hashBlockCtr + k],
                            _mm_loadu_si128(curOTRecvDataStartBlock + k));
            }
          }
          // Now hashesStored[hashBlockCtr:hashBlockCtr+numHashBlocks] contains
          // the final data to be used. Unpack and fill in the output variable
          // data
          for (uint64_t k = 0; k < curOTNumChunks; k++) {
            data[dataPtr + k] = (intType)readFromPackedArr(
                (uint8_t *)(hashesStored + hashBlockCtr), 16 * numHashBlocks,
                k * curOTChunkSize, curOTChunkSize);
          }
          dataPtr += curOTNumChunks;
          dataRecvByteAlignedCtr +=
              ceil_val(curOTChunkSize * curOTNumChunks, 8);
          hashBlockCtr += numHashBlocks;
        }

        otDataStartCtr = i + 1;
        dataToBeRecvdCummulativeCtr = 0;
        assert(hashBlockCtr == hashesStoredPtr);
        hashesStoredPtr = 0;
      }
    }

    // Assertion for my understanding
    uint64_t totalChunks = 0;
    for (int i = 0; i < numOTs; i++)
      totalChunks += numChunks[i];
    assert(dataPtr == totalChunks);
    assert(otDataStartCtr == numOTs);
    delete[] hashesStored;
    delete[] tT;
  }

  /*
  COT function for packed multiplication correlation
  - Does numOTs COTs for the correlation which is used in matmul
  - corr is the array which contains the correlated values (i.e. a in the case
  of matmul). Each value is stored in a separate uint64_t.
  - For each OT, the chunkSizes[i] denotes the bitlen used for that OT (again
  referring to matmul)
  - For each OT, the numChunks[i] denotes the #values of that particular bitlen.
  Hence, size of corr = summation of numChunks
  - rdata is the r value which is returned to the sender
  */
  template <typename intType>
  void send_cot_matmul(intType *rdata, const intType *corr,
                       const uint32_t *chunkSizes, const uint32_t *numChunks,
                       const int numOTs, int senderMatmulDims) {
    send_pre(numOTs);
    cot_send_post_matmul<intType>(rdata, corr, chunkSizes, numChunks, numOTs,
                                  senderMatmulDims);
  }

  /*
  COT function for packed multiplication correlation
  - data is the data which will be read
  - Rest of parameters have same meaning as specified in sender's description
  */
  template <typename intType>
  void recv_cot_matmul(intType *data, const uint8_t *choices,
                       const uint32_t *chunkSizes, const uint32_t *numChunks,
                       const int numOTs, int senderMatmulDims) {
    recv_pre((bool *)choices, numOTs);
    cot_recv_post_matmul<intType>(data, choices, chunkSizes, numChunks, numOTs,
                                  senderMatmulDims);
  }

  template <typename intType>
  void cot_send_post_moduloAdd(intType *rdata, const intType *delta,
                               const int length) {
    const int bsize = AES_BATCH_SIZE / 2;
    block128 pad[2 * bsize];
    intType tmp[bsize];
    for (int i = 0; i < length; i += bsize) {
      for (int j = i; j < i + bsize and j < length; ++j) {
        pad[2 * (j - i)] = qT[j];
        pad[2 * (j - i) + 1] = xorBlocks(qT[j], block_s);
      }
      crh.H<2 * bsize>(pad, pad);
      for (int j = i; j < i + bsize and j < length; ++j) {
        rdata[j] =
            (intType)_mm_extract_epi64(pad[2 * (j - i)], 0); // Use lower bits
        intType corrVal = rdata[j] + delta[j];
        intType mask = (intType)_mm_extract_epi64(pad[2 * (j - i) + 1], 0);
        intType msgToBeSent = corrVal ^ mask;
        tmp[j - i] = msgToBeSent;
      }
      io->send_data(tmp, sizeof(intType) * std::min(bsize, length - i));
    }
    delete[] qT;
  }

  template <typename intType>
  void cot_recv_post_moduloAdd(intType *data, const uint8_t *choices,
                               const int length) {
    const int bsize = AES_BATCH_SIZE;
    intType res[bsize];
    block128 hashes[bsize];
    for (int i = 0; i < length; i += bsize) {
      io->recv_data(res, sizeof(intType) * std::min(bsize, length - i));
      if (bsize <= length - i)
        crh.H<bsize>(hashes, tT + i);
      else
        crh.Hn(hashes, tT + i, length - i);
      for (int j = 0; j < bsize and j < length - i; ++j) {
        data[i + j] = (intType)_mm_extract_epi64(
            hashes[j], 0); // Use lower bits convention
        if (choices[i + j]) {
          data[i + j] = data[i + j] ^ res[j];
        }
      }
    }
    delete[] tT;
  }

  /*
  - For the correlation f(r) = (r+delta[i])mod 2^l
  */
  template <typename intType>
  void send_cot_moduloAdd(intType *rdata, const intType *delta,
                          const int numOTs) {
    send_pre(numOTs);
    cot_send_post_moduloAdd<intType>(rdata, delta, numOTs);
  }

  /*
  - For the correlation f(r) = (r+delta[i])mod 2^l
  */
  template <typename intType>
  void recv_cot_moduloAdd(intType *data, const uint8_t *choices,
                          const int numOTs) {
    recv_pre((bool *)choices, numOTs);
    cot_recv_post_moduloAdd<intType>(data, choices, numOTs);
  }
};

} // namespace primihub::sci
#endif // IKNP_H__
