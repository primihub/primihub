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

Modified by Nishant Kumar, Deevashwer Rathee
*/

#ifndef OT_IDEAL_H__
#define OT_IDEAL_H__
#include "src/primihub/protocol/cryptflow2/OT/ot.h"
#include <cmath>
/** @addtogroup OT
    @{
  */
namespace primihub::sci {
template <typename IO> class OTIdeal : public OT<OTIdeal<IO>> {
public:
  int cnt = 0;
  IO *io = nullptr;
  OTIdeal(IO *io) { this->io = io; }

  void send_impl(const block128 *data0, const block128 *data1, int length) {
    cnt += length;
    io->send_block(data0, length);
    io->send_block(data1, length);
  }

  void send_impl(const block256 *data0, const block256 *data1, int length) {
    cnt += length;
    io->send_block(data0, length);
    io->send_block(data1, length);
  }

  void recv_impl(block128 *data, const bool *b, int length) {
    cnt += length;
    block128 *data1 = new block128[length];
    io->recv_block(data, length);
    io->recv_block(data1, length);
    for (int i = 0; i < length; ++i)
      if (b[i])
        data[i] = data1[i];
    delete[] data1;
  }

  void recv_impl(block256 *data, const bool *b, int length) {
    cnt += length;
    alignas(32) block256 data1[length];
    io->recv_block(data, length);
    io->recv_block(data1, length);
    for (int i = 0; i < length; ++i)
      if (b[i])
        data[i] = data1[i];
  }

  void send_impl(uint8_t **data, int length, int N, int l) {
    assert(N <= 256 && N >= 2);
    assert(l <= 8 && l >= 1 && (8 % l) == 0);
    assert((N * l % 8) == 0);
    uint8_t *b = new uint8_t[length];
    io->recv_data(b, length);
    for (int i = 0; i < length; i++)
      io->send_data(&data[i][b[i]], 1);
    delete[] b;
  }

  void recv_impl(uint8_t *data, const uint8_t *b, int length, int N, int l) {
    assert(N <= 256 && N >= 2);
    assert(l <= 8 && l >= 1 && (8 % l) == 0);
    assert((N * l % 8) == 0);
    io->send_data(b, length);
    for (int i = 0; i < length; i++)
      io->recv_data(&data[i], 1);
  }

  /*
      COT function for packed multiplication correlation
      - Does numOTs COTs for the correlation which is used in matmul
      - corr is the array which contains the correlated values (i.e. a in the
     case of matmul). Each value is stored in a separate uint64_t.
      - For each OT, the chunkSizes[i] denotes the bitlen used for that OT
     (again referring to matmul)
      - For each OT, the numChunks[i] denotes the #values of that particular
     bitlen. Hence, size of corr = summation of numChunks
      - rdata is the r value which is returned to the sender
  */
  void send_cot_matmul(uint64_t *rdata, const uint64_t *corr,
                       const uint64_t *chunkSizes, const uint64_t *numChunks,
                       const int numOTs) {
    // std::cout<<"Using Ideal OT"<<std::endl;
    uint8_t choices[numOTs];
    io->recv_data(choices, numOTs);
    uint64_t corrPtr = 0;
    for (int i = 0; i < numOTs; i++) {
      // std::cout<<" i = "<<i<<" choice bit =
      // "<<unsigned(choices[i])<<std::endl;
      uint64_t curNumChunks = numChunks[i];
      uint64_t curChunkSize = chunkSizes[i];
      uint64_t recv_arr[curNumChunks];
      for (uint64_t j = 0; j < curNumChunks; j++) {
        rdata[corrPtr + j] =
            (1 << curChunkSize) - 1; // Fill deterministic random values
        // rdata[corrPtr+j] = 0; //Fill deterministic random values
        if (choices[i] == 0) {
          recv_arr[j] = rdata[corrPtr + j];
        } else {
          assert((choices[i] == 1) && "unknown choice bit");
          recv_arr[j] = (rdata[corrPtr + j] + corr[corrPtr + j]) &
                        (all1Mask(curChunkSize));
          // std::cout<<" ** "<<rdata[corrPtr+j]<<" "<<corr[corrPtr+j]<<"
          // "<<recv_arr[j]<<" chunksize = "<<curChunkSize<<std::endl;
        }
        // std::cout<<"j = "<<j<<" recv_data = "<<recv_arr[j]<<std::endl;
      }
      io->send_data(recv_arr, sizeof(uint64_t) * curNumChunks);
      corrPtr += curNumChunks;
    }
  }

  /*
      COT function for packed multiplication correlation
      - data is the data which will be read
      - Rest of parameters have same meaning as specified in sender's
     description
  */
  void recv_cot_matmul(uint64_t *data, const uint8_t *choices,
                       const uint64_t *chunkSizes, const uint64_t *numChunks,
                       const int numOTs) {
    // std::cout<<"Using Ideal OT"<<std::endl;
    io->send_data(choices, numOTs);
    uint64_t dataPtr = 0;
    for (int i = 0; i < numOTs; i++) {
      uint64_t curNumChunks = numChunks[i];
      io->recv_data(&data[dataPtr], sizeof(uint64_t) * curNumChunks);
      dataPtr += curNumChunks;
    }
  }
};
} // namespace primihub::sci
#endif // OT_IDEAL_H__
