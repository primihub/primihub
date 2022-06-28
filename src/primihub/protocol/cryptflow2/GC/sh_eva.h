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

#ifndef EMP_SEMIHONEST_EVA_H__
#define EMP_SEMIHONEST_EVA_H__
#include "src/primihub/protocol/cryptflow2/GC/sh_party.h"

namespace primihub::sci {
template <typename IO> class SemiHonestEva : public SemiHonestParty<IO> {
public:
  HalfGateEva<IO> *gc;
  PRG128 prg;
  SemiHonestEva(IO *io, HalfGateEva<IO> *gc) : SemiHonestParty<IO>(io, BOB) {
    this->gc = gc;
    // this->ot->setup_recv();
    block128 seed;
    this->io->recv_block(&seed, 1);
    this->shared_prg.reseed(&seed);
    this->top = this->batch_size;
    // refill();
  }

  void setup_keys(block128 *in_k0, block128 *in_k1) {
    this->ot->setup_recv(in_k0, in_k1);
  }

  void refill() {
    prg.random_bool(this->buff, this->batch_size);
    this->ot->recv_cot(this->buf, this->buff, this->batch_size);
    this->top = 0;
  }

  void feed(block128 *label, int party, const bool *b, int length) {
    if (party == ALICE) {
      this->shared_prg.random_block(label, length);
    } else {
      if (length > this->batch_size) {
        this->ot->recv_cot(label, b, length);
      } else {
        bool *tmp = new bool[length];
        if (length > this->batch_size - this->top) {
          memcpy(label, this->buf + this->top,
                 (this->batch_size - this->top) * sizeof(block128));
          memcpy(tmp, this->buff + this->top, (this->batch_size - this->top));
          int filled = this->batch_size - this->top;
          refill();
          memcpy(label + filled, this->buf,
                 (length - filled) * sizeof(block128));
          memcpy(tmp + filled, this->buff, length - filled);
          this->top = length - filled;
        } else {
          memcpy(label, this->buf + this->top, length * sizeof(block128));
          memcpy(tmp, this->buff + this->top, length);
          this->top += length;
        }

        for (int i = 0; i < length; ++i)
          tmp[i] = (tmp[i] != b[i]);
        this->io->send_data(tmp, length);

        delete[] tmp;
      }
    }
  }

  void reveal(bool *b, int party, const block128 *label, int length) {
    if (party == XOR) {
      for (int i = 0; i < length; ++i)
        b[i] = getLSB(label[i]);
      return;
    }
    for (int i = 0; i < length; ++i) {
      bool lsb = getLSB(label[i]), tmp;
      if (party == BOB or party == PUBLIC) {
        this->io->recv_data(&tmp, 1);
        b[i] = (tmp != lsb);
      } else if (party == ALICE) {
        this->io->send_data(&lsb, 1);
        b[i] = false;
      }
    }
    if (party == PUBLIC)
      this->io->send_data(b, length);
  }
};
} // namespace primihub::sci

#endif // GARBLE_CIRCUIT_SEMIHONEST_H__
