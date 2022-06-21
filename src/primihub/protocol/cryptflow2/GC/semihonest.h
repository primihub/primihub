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

#ifndef EMP_SEMIHONEST_H__
#define EMP_SEMIHONEST_H__
#include "src/primihub/protocol/cryptflow2/GC/sh_eva.h"
#include "src/primihub/protocol/cryptflow2/GC/sh_gen.h"

namespace primihub::sci {

template <typename IO = NetIO>
inline SemiHonestParty<IO> *setup_semi_honest(IO *io, int party,
                                              int batch_size = 1024 * 16) {
  if (party == ALICE) {
    HalfGateGen<IO> *t = new HalfGateGen<IO>(io);
    circ_exec = t;
    prot_exec = new SemiHonestGen<IO>(io, t);
  } else {
    HalfGateEva<IO> *t = new HalfGateEva<IO>(io);
    circ_exec = t;
    prot_exec = new SemiHonestEva<IO>(io, t);
  }
  static_cast<SemiHonestParty<IO> *>(prot_exec)->set_batch_size(batch_size);
  return (SemiHonestParty<IO> *)prot_exec;
}
} // namespace primihub::sci
#endif
