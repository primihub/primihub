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

#ifndef GC_UTILS_H__
#define GC_UTILS_H__

#include "src/primihub/protocol/cryptflow2/utils/emp-tool.h"

namespace primihub::sci {

inline block128 bool_to_block(const bool *data) {
  return makeBlock128(bool_to_int<uint64_t>(data + 64),
                      bool_to_int<uint64_t>(data));
}

inline void block_to_bool(bool *data, block128 b) {
  uint64_t *ptr = (uint64_t *)(&b);
  int_to_bool<uint64_t>(data, ptr[0], 64);
  int_to_bool<uint64_t>(data + 64, ptr[1], 64);
}

} // namespace primihub::sci
#endif // GC_UTILS_H__
