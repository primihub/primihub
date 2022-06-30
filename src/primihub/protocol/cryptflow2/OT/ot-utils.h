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

#ifndef OT_UTIL_H__
#define OT_UTIL_H__
#include "src/primihub/protocol/cryptflow2/OT/ot.h"

namespace primihub::sci {
template <typename basetype>
void pack_ot_messages(basetype *y, basetype **data, block128 *pad, int ysize,
                      int bsize, int bitsize, int N) {
  assert(y != nullptr && data != nullptr && pad != nullptr);
  uint64_t start_pos = 0;
  uint64_t end_pos = 0;
  uint64_t start_block = 0;
  uint64_t end_block = 0;
  basetype temp_bl = 0;
  basetype mask = (1 << bitsize) - 1;
  if (8 * sizeof(basetype) == 64) {
    mask = (basetype)((1ULL << bitsize) - 1ULL);
  }
  if (8 * sizeof(basetype) == bitsize) {
    if (bitsize == 64) {
      mask = (basetype)(-1ULL);
    } else {
      mask = (basetype)(-1);
    }
  }
  uint64_t carriersize = 8 * (sizeof(basetype));
  for (int i = 0; i < ysize; i++) {
    y[i] = 0;
  }
  for (int i = 0; i < bsize; i++) {
    for (int k = 0; k < N; k++) {
      // OT message k
      start_pos = i * N * bitsize + k * bitsize; // inclusive
      end_pos = start_pos + bitsize;
      end_pos -= 1; // inclusive
      start_block = start_pos / carriersize;
      end_block = end_pos / carriersize;
      if (carriersize == 64) {
        if (start_block == end_block) {
          y[start_block] ^=
              ((((basetype)_mm_extract_epi64(pad[(N * i) + k], 0)) ^
                data[i][k]) &
               mask)
              << (start_pos % carriersize);
        } else {
          temp_bl = ((((basetype)_mm_extract_epi64(pad[(N * i) + k], 0)) ^
                      data[i][k]) &
                     mask);
          y[start_block] ^= (temp_bl) << (start_pos % carriersize);
          y[end_block] ^=
              (temp_bl) >> (carriersize - (start_pos % carriersize));
        }
      } else if (carriersize == 8) {
        if (start_block == end_block) {
          y[start_block] ^=
              ((((basetype)_mm_extract_epi8(pad[(N * i) + k], 0)) ^
                data[i][k]) &
               mask)
              << (start_pos % carriersize);
        } else {
          temp_bl = ((((basetype)_mm_extract_epi8(pad[(N * i) + k], 0)) ^
                      data[i][k]) &
                     mask);
          y[start_block] ^= (temp_bl) << (start_pos % carriersize);
          y[end_block] ^=
              (temp_bl) >> (carriersize - (start_pos % carriersize));
        }
      } else {
        throw std::invalid_argument("Not implemented");
      }
    }
  }
}

template <typename basetype>
void unpack_ot_messages(basetype *data, const uint8_t *r, basetype *recvd,
                        block128 *pad, int bsize, int bitsize, int N) {
  assert(data != nullptr && recvd != nullptr && pad != nullptr);
  uint64_t start_pos = 0;
  uint64_t end_pos = 0;
  uint64_t start_block = 0;
  uint64_t end_block = 0;
  basetype mask = (1 << bitsize) - 1;
  if (8 * sizeof(basetype) == 64) {
    mask = (basetype)((1ULL << bitsize) - 1ULL);
  }
  if (8 * sizeof(basetype) == bitsize) {
    if (bitsize == 64) {
      mask = (basetype)(-1ULL);
    } else {
      mask = (basetype)(-1);
    }
  }
  uint64_t carriersize = 8 * (sizeof(basetype));

  for (int i = 0; i < bsize; i++) {
    start_pos = i * N * bitsize + r[i] * bitsize;
    end_pos = start_pos + bitsize - 1; // inclusive
    start_block = start_pos / carriersize;
    end_block = end_pos / carriersize;
    if (carriersize == 64) {
      if (start_block == end_block) {
        data[i] = ((recvd[start_block] >> (start_pos % carriersize)) ^
                   ((basetype)_mm_extract_epi64(pad[i], 0))) &
                  mask;
      } else {
        data[i] = 0;
        data[i] ^= (recvd[start_block] >> (start_pos % carriersize));
        data[i] ^=
            (recvd[end_block] << (carriersize - (start_pos % carriersize)));
        data[i] = (data[i] ^ ((basetype)_mm_extract_epi64(pad[i], 0))) & mask;
      }
    } else if (carriersize == 8) {
      if (start_block == end_block) {
        data[i] = ((recvd[start_block] >> (start_pos % carriersize)) ^
                   ((basetype)_mm_extract_epi8(pad[i], 0))) &
                  mask;
      } else {
        data[i] = 0;
        data[i] ^= (recvd[start_block] >> (start_pos % carriersize));
        data[i] ^=
            (recvd[end_block] << (carriersize - (start_pos % carriersize)));
        data[i] = (data[i] ^ ((basetype)_mm_extract_epi8(pad[i], 0))) & mask;
      }
    } else {
      throw std::invalid_argument("Not implemented");
    }
  }
}

inline void pack_cot_messages(uint64_t *y, uint64_t *corr_data, int ysize,
                              int bsize, int bitsize) {
  assert(y != nullptr && corr_data != nullptr);
  uint64_t start_pos = 0;
  uint64_t end_pos = 0;
  uint64_t start_block = 0;
  uint64_t end_block = 0;
  uint64_t temp_bl = 0;
  uint64_t mask = (1ULL << bitsize) - 1;
  if (bitsize == 64)
    mask = -1;

  uint64_t carriersize = 64;
  for (int i = 0; i < ysize; i++) {
    y[i] = 0;
  }
  for (int i = 0; i < bsize; i++) {
    start_pos = i * bitsize; // inclusive
    end_pos = start_pos + bitsize;
    end_pos -= 1; // inclusive
    start_block = start_pos / carriersize;
    end_block = end_pos / carriersize;
    if (carriersize == 64) {
      if (start_block == end_block) {
        y[start_block] ^= (corr_data[i] & mask) << (start_pos % carriersize);
      } else {
        temp_bl = (corr_data[i] & mask);
        y[start_block] ^= (temp_bl) << (start_pos % carriersize);
        y[end_block] ^= (temp_bl) >> (carriersize - (start_pos % carriersize));
      }
    }
  }
}

inline void unpack_cot_messages(uint64_t *corr_data, uint64_t *recvd, int bsize,
                                int bitsize) {
  assert(corr_data != nullptr && recvd != nullptr);
  uint64_t start_pos = 0;
  uint64_t end_pos = 0;
  uint64_t start_block = 0;
  uint64_t end_block = 0;
  uint64_t mask = (1ULL << bitsize) - 1;
  if (bitsize == 64)
    mask = -1;
  uint64_t carriersize = 64;

  for (int i = 0; i < bsize; i++) {
    start_pos = i * bitsize;
    end_pos = start_pos + bitsize - 1; // inclusive
    start_block = start_pos / carriersize;
    end_block = end_pos / carriersize;
    if (carriersize == 64) {
      if (start_block == end_block) {
        corr_data[i] = (recvd[start_block] >> (start_pos % carriersize)) & mask;
      } else {
        corr_data[i] = 0;
        corr_data[i] ^= (recvd[start_block] >> (start_pos % carriersize));
        corr_data[i] ^=
            (recvd[end_block] << (carriersize - (start_pos % carriersize)));
      }
    }
  }
}
} // namespace primihub::sci

#endif // OT_UTIL_H__
