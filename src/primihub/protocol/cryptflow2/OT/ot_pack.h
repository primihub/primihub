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

#ifndef OT_PACK_H__
#define OT_PACK_H__
#include "src/primihub/protocol/cryptflow2/OT/emp-ot.h"
#include "src/primihub/protocol/cryptflow2/utils/emp-tool.h"

#define KKOT_TYPES 8

namespace primihub::sci {
class OTPack {
public:
  SplitKKOT<NetIO> *kkot[KKOT_TYPES];

  // iknp_straight and iknp_reversed: party
  // acts as sender in straight and receiver in reversed.
  // Needed for MUX calls.
  SplitIKNP<NetIO> *iknp_straight;
  SplitIKNP<NetIO> *iknp_reversed;
  IOPack *iopack;
  int party;
  bool do_setup = false;

  OTPack(IOPack *iopack, int party, bool do_setup = true) {
    this->party = party;
    this->do_setup = do_setup;
    this->iopack = iopack;

    for (int i = 0; i < KKOT_TYPES; i++) {
      kkot[i] = new SplitKKOT<NetIO>(party, iopack->io, 1 << (i + 1));
    }

    iknp_straight = new SplitIKNP<NetIO>(party, iopack->io);
    iknp_reversed = new SplitIKNP<NetIO>(3 - party, iopack->io_rev);

    if (do_setup) {
      SetupBaseOTs();
    }
  }

  ~OTPack() {
    for (int i = 0; i < KKOT_TYPES; i++)
      delete kkot[i];
    delete iknp_straight;
    delete iknp_reversed;
  }

  void SetupBaseOTs() {
    switch (party) {
    case 1:
      kkot[0]->setup_send();
      iknp_straight->setup_send();
      iknp_reversed->setup_recv();
      for (int i = 1; i < KKOT_TYPES; i++) {
        kkot[i]->setup_send();
      }
      break;
    case 2:
      kkot[0]->setup_recv();
      iknp_straight->setup_recv();
      iknp_reversed->setup_send();
      for (int i = 1; i < KKOT_TYPES; i++) {
        kkot[i]->setup_recv();
      }
      break;
    }
  }

  /*
   * DISCLAIMER:
   * OTPack copy method avoids computing setup keys for each OT instance by
   * reusing the keys generated (through base OTs) for another OT instance.
   * Ideally, the PRGs within OT instances, using the same keys, should use
   * mutually exclusive counters for security. However, the current
   * implementation does not support this.
   */

  void copy(OTPack *copy_from) {
    assert(this->do_setup == false && copy_from->do_setup == true);
    SplitKKOT<NetIO> *kkot_base = copy_from->kkot[0];
    SplitIKNP<NetIO> *iknp_s_base = copy_from->iknp_straight;
    SplitIKNP<NetIO> *iknp_r_base = copy_from->iknp_reversed;

    switch (this->party) {
    case 1:
      for (int i = 0; i < KKOT_TYPES; i++) {
        this->kkot[i]->setup_send(kkot_base->k0, kkot_base->s);
      }
      this->iknp_straight->setup_send(iknp_s_base->k0, iknp_s_base->s);
      this->iknp_reversed->setup_recv(iknp_r_base->k0, iknp_r_base->k1);
      break;
    case 2:
      for (int i = 0; i < KKOT_TYPES; i++) {
        this->kkot[i]->setup_recv(kkot_base->k0, kkot_base->k1);
      }
      this->iknp_straight->setup_recv(iknp_s_base->k0, iknp_s_base->k1);
      this->iknp_reversed->setup_send(iknp_r_base->k0, iknp_r_base->s);
      break;
    }
    this->do_setup = true;
    return;
  }
};
} // namespace primihub::sci
#endif // OT_PACK_H__
