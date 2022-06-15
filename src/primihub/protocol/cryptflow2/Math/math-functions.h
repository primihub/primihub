/*
Authors: Deevashwer Rathee
Copyright:
Copyright (c) 2021 Microsoft Research
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

#ifndef MATH_FUNCTIONS_H__
#define MATH_FUNCTIONS_H__

#include "src/primihub/protocol/cryptflow2/BuildingBlocks/aux-protocols.h"
#include "src/primihub/protocol/cryptflow2/BuildingBlocks/truncation.h"
#include "src/primihub/protocol/cryptflow2/BuildingBlocks/value-extension.h"
#include "src/primihub/protocol/cryptflow2/LinearOT/linear-ot.h"

namespace primihub::cryptflow2
{
  class MathFunctions
  {
  public:
    int party;
    primihub::sci::IOPack *iopack;
    primihub::sci::OTPack *otpack;
    AuxProtocols *aux;
    XTProtocol *xt;
    Truncation *trunc;
    LinearOT *mult;

    MathFunctions(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack);

    ~MathFunctions();

    // Current implementation assumes that dn is always of the form 1.y1y2y3..yn
    void reciprocal_approximation(int32_t dim, int32_t m, uint64_t *dn,
                                  uint64_t *out, int32_t bw_dn, int32_t bw_out,
                                  int32_t s_dn, int32_t s_out);

    // If compute_msnzb = false, dn = 1.y1y2y3....
    // Else if compute_msnzb = true, dn is always positive
    void div(int32_t dim,
             // numerator
             uint64_t *nm,
             // denominator
             uint64_t *dn,
             // output
             uint64_t *out,
             // bitwidths
             int32_t bw_nm, int32_t bw_dn, int32_t bw_out,
             // scales
             int32_t s_nm, int32_t s_dn, int32_t s_out, bool signed_nm = true,
             bool compute_msnzb = false);

    // Assumes x is always negative
    void lookup_table_exp(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x,
                          int32_t bw_y, int32_t s_x, int32_t s_y);

    void sigmoid(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x,
                 int32_t bw_y, int32_t s_x, int32_t s_y);

    void tanh(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x, int32_t bw_y,
              int32_t s_x, int32_t s_y);

    void sqrt(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x, int32_t bw_y,
              int32_t s_x, int32_t s_y, bool inverse = false);

    // bw_y = bw_x
    void ReLU(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x,
              uint64_t six = 0);
  };
}
#endif
