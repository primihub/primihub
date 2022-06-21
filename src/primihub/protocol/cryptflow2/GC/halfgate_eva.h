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

#ifndef EMP_HALFGATE_EVA_H__
#define EMP_HALFGATE_EVA_H__
#include "src/primihub/protocol/cryptflow2/GC/circuit_execution.h"
#include "src/primihub/protocol/cryptflow2/GC/mitccrh.h"
#include "src/primihub/protocol/cryptflow2/utils/utils.h"
#include <iostream>
namespace primihub::sci {

block128 halfgates_eval(block128 A, block128 B, const block128 *table,
                        MITCCRH<8> *mitccrh);

template <typename T> class HalfGateEva : public CircuitExecution {
public:
  T *io;
  block128 constant[2];
  MITCCRH<8> mitccrh;
  HalfGateEva(T *io) : io(io) {
    set_delta();
    block128 tmp;
    io->recv_block(&tmp, 1);
    mitccrh.setS(tmp);
  }
  void set_delta() { io->recv_block(constant, 2); }
  block128 public_label(bool b) override { return constant[b]; }
  block128 and_gate(const block128 &a, const block128 &b) override {
    block128 table[2];
    io->recv_block(table, 2);
    return halfgates_eval(a, b, table, &mitccrh);
  }
  block128 xor_gate(const block128 &a, const block128 &b) override {
    return a ^ b;
  }
  block128 not_gate(const block128 &a) override {
    return xor_gate(a, public_label(true));
  }
  size_t num_and() override { return mitccrh.gid; }
};
} // namespace primihub::sci
#endif // HALFGATE_EVA_H__
