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

#ifndef EMP_CIRCUIT_EXECUTION_H__
#define EMP_CIRCUIT_EXECUTION_H__
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/constants.h"

namespace primihub::sci {

/* Circuit Pipelining
 * [REF] Implementation of "Faster Secure Two-Party Computation Using Garbled
 * Circuit" https://www.usenix.org/legacy/event/sec11/tech/full_papers/Huang.pdf
 */
class CircuitExecution {
public:
  /*
  #ifndef THREADING
          // static CircuitExecution * circ_exec;
  #else
          static __thread CircuitExecution * circ_exec;
  #endif
  */
  virtual block128 and_gate(const block128 &in1, const block128 &in2) = 0;
  virtual block128 xor_gate(const block128 &in1, const block128 &in2) = 0;
  virtual block128 not_gate(const block128 &in1) = 0;
  virtual block128 public_label(bool b) = 0;
  virtual size_t num_and() { return -1; }
  virtual ~CircuitExecution() {}
};
enum RTCktOpt { on, off };
} // namespace primihub::sci

// extern primihub::sci::CircuitExecution* circ_exec;
thread_local extern primihub::sci::CircuitExecution *circ_exec;
#endif
