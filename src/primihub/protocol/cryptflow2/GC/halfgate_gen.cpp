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

#include "src/primihub/protocol/cryptflow2/GC/halfgate_gen.h"

using namespace primihub::sci;

block128 primihub::sci::halfgates_garble(block128 LA0, block128 A1, block128 LB0,
                               block128 B1, block128 delta, block128 *table,
                               MITCCRH<8> *mitccrh) {
  bool pa = getLSB(LA0);
  bool pb = getLSB(LB0);
  block128 HLA0, HA1, HLB0, HB1;
  block128 tmp, W0;

  block128 H[4];
  H[0] = LA0;
  H[1] = A1;
  H[2] = LB0;
  H[3] = B1;
  mitccrh->hash<2, 2>(H);
  HLA0 = H[0];
  HA1 = H[1];
  HLB0 = H[2];
  HB1 = H[3];

  table[0] = HLA0 ^ HA1;
  table[0] = table[0] ^ (select_mask[pb] & delta);
  W0 = HLA0;
  W0 = W0 ^ (select_mask[pa] & table[0]);
  tmp = HLB0 ^ HB1;
  table[1] = tmp ^ LA0;
  W0 = W0 ^ HLB0;
  W0 = W0 ^ (select_mask[pb] & tmp);

  return W0;
}
