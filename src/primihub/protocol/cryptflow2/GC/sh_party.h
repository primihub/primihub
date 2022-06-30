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

#ifndef EMP_SH_PARTY_H__
#define EMP_SH_PARTY_H__
#include "src/primihub/protocol/cryptflow2/GC/emp-tool.h"
#include "src/primihub/protocol/cryptflow2/OT/iknp.h"

namespace primihub::sci {

template <typename IO> class SemiHonestParty : public ProtocolExecution {
public:
  IO *io = nullptr;
  IKNP<IO> *ot = nullptr;
  PRG128 shared_prg;

  block128 *buf = nullptr;
  bool *buff = nullptr;
  int top = 0;
  int batch_size = 1024 * 16;

  SemiHonestParty(IO *io, int party) : ProtocolExecution(party) {
    this->io = io;
    ot = new IKNP<IO>(io);
    buf = new block128[batch_size];
    buff = new bool[batch_size];
  }
  void set_batch_size(int size) {
    delete[] buf;
    delete[] buff;
    batch_size = size;
    top = batch_size;
    buf = new block128[batch_size];
    buff = new bool[batch_size];
  }

  ~SemiHonestParty() {
    delete[] buf;
    delete[] buff;
    delete ot;
  }
};
} // namespace primihub::sci
#endif
