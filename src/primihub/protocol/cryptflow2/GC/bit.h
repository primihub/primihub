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

#ifndef EMP_BIT_H__
#define EMP_BIT_H__
#include "src/primihub/protocol/cryptflow2/GC/circuit_execution.h"
#include "src/primihub/protocol/cryptflow2/GC/protocol_execution.h"
#include "src/primihub/protocol/cryptflow2/GC/swappable.h"
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/utils.h"

namespace primihub::sci {
class Bit : public Swappable<Bit> {
public:
  block128 bit;

  Bit(bool _b = false, int party = PUBLIC);
  Bit(const block128 &a) { memcpy(&bit, &a, sizeof(block128)); }

  template <typename O = bool> O reveal(int party = PUBLIC) const;

  Bit operator!=(const Bit &rhs) const;
  Bit operator==(const Bit &rhs) const;
  Bit operator&(const Bit &rhs) const;
  Bit operator|(const Bit &rhs) const;
  Bit operator!() const;

  // swappable
  Bit select(const Bit &select, const Bit &new_v) const;
  Bit operator^(const Bit &rhs) const;

  // batcher
  template <typename... Args> static size_t bool_size(Args &&...args) {
    return 1;
  }

  static void bool_data(bool *b, bool data) { b[0] = data; }

  Bit(size_t size, const block128 *a) { memcpy(&bit, a, sizeof(block128)); }
};

#include "src/primihub/protocol/cryptflow2/GC/bit.hpp"
} // namespace primihub::sci
#endif
