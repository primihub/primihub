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

#ifndef LINEAR_OT_H__
#define LINEAR_OT_H__

#include "src/primihub/protocol/cryptflow2/BuildingBlocks/aux-protocols.h"
#include "src/primihub/protocol/cryptflow2/BuildingBlocks/truncation.h"
#include "src/primihub/protocol/cryptflow2/BuildingBlocks/value-extension.h"
#include "src/primihub/protocol/cryptflow2/OT/emp-ot.h"

namespace primihub::cryptflow2
{
  enum class MultMode
  {
    None,        // Both A and B are secret shared
    Alice_has_A, // A is known to ALICE
    Alice_has_B, // B is known to ALICE
    Bob_has_A,   // A is known to BOB
    Bob_has_B,   // B is known to BOB
  };

  class LinearOT
  {
  public:
    int party;
    primihub::sci::IOPack *iopack;
    primihub::sci::OTPack *otpack;
    AuxProtocols *aux;
    Truncation *trunc;
    XTProtocol *xt;

    LinearOT(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack);

    ~LinearOT();

    void hadamard_cleartext(int dim, uint64_t *inA, uint64_t *inB,
                            uint64_t *outC);

    // Outputs the dim1*dim2*dim3 multiplications in inA*inB without accumulating
    void matmul_cleartext(int dim1, int dim2, int dim3, uint64_t *inA,
                          uint64_t *inB, uint64_t *outC, bool accumulate = true);

    // Hadamard cross terms A0B1 + A1B0
    void hadamard_cross_terms(int32_t dim, uint64_t *inA, uint64_t *inB,
                              uint64_t *outC, int32_t bwA, int32_t bwB,
                              int32_t bwC, MultMode mode = MultMode::None);

    // Hadamard product of two secret-shared vectors A and B
    void hadamard_product(int32_t dim,
                          // input share vector
                          uint64_t *inA, uint64_t *inB,
                          // output share vector
                          uint64_t *outC,
                          // bitwidths
                          int32_t bwA, int32_t bwB, int32_t bwC,
                          bool signed_arithmetic = true,
                          // take B as signed input?
                          bool signed_B = true, MultMode mode = MultMode::None,
                          uint8_t *msbA = nullptr, uint8_t *msbB = nullptr);

    // Matmul cross terms A0B1 + A1B0
    void matmul_cross_terms(
        // (dim1xdim2) X (dim2xdim3)
        int32_t dim1, int32_t dim2, int32_t dim3,
        // input share matrix
        uint64_t *inA, uint64_t *inB,
        // output share matrix
        uint64_t *outC,
        // bitwidths
        int32_t bwA, int32_t bwB, int32_t bwC, bool accumulate,
        MultMode mode = MultMode::None);

    // Matmul Multiplexer: bwA == 1 || bwB == 1
    void matmul_multiplexer(
        // (dim1xdim2) X (dim2xdim3)
        int32_t dim1, int32_t dim2, int32_t dim3,
        // either A or B will be a bit matrix
        uint64_t *inA, uint64_t *inB, uint64_t *outC,
        // bitwidths
        int32_t bwA, int32_t bwB, int32_t bwC, bool accumulate, MultMode mode);

    // Matrix Multiplication of two secret-shared matrices A and B
    void matrix_multiplication(
        // (dim1xdim2) X (dim2xdim3)
        int32_t dim1, int32_t dim2, int32_t dim3,
        // input share matrix
        uint64_t *inA, uint64_t *inB,
        // output share matrix
        uint64_t *outC,
        // bitwidths
        int32_t bwA, int32_t bwB, int32_t bwC, bool signed_arithmetic = true,
        // take B as signed input?
        bool signed_B = true, bool accumulate = true,
        MultMode mode = MultMode::None, uint8_t *msbA = nullptr,
        uint8_t *msbB = nullptr);
  };
}
#endif
