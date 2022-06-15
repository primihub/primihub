/*
Authors: Mayank Rathee, Deevashwer Rathee
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

#ifndef TRUNCATION_H__
#define TRUNCATION_H__

#include "src/primihub/protocol/cryptflow2/BuildingBlocks/aux-protocols.h"
#include "src/primihub/protocol/cryptflow2/Millionaire/equality.h"
#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire_with_equality.h"
namespace primihub::cryptflow2
{

    class Truncation
    {
    public:
        primihub::sci::IOPack *iopack;
        primihub::sci::OTPack *otpack;
        TripleGenerator *triple_gen;
        MillionaireProtocol *mill;
        MillionaireWithEquality *mill_eq;
        Equality *eq;
        AuxProtocols *aux;
        int party;

        // Constructor
        Truncation(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack);

        // Destructor
        ~Truncation();

        // Truncate (right-shift) by shift in the same ring (round towards -inf)
        void truncate(
            // Size of vector
            int32_t dim,
            // input vector
            uint64_t *inA,
            // output vector
            uint64_t *outB,
            // right shift amount
            int32_t shift,
            // Input and output bitwidth
            int32_t bw,
            // signed truncation?
            bool signed_arithmetic = true,
            // msb of input vector elements
            uint8_t *msb_x = nullptr);

        // Divide by 2^shift in the same ring (round towards 0)
        void div_pow2(
            // Size of vector
            int32_t dim,
            // input vector
            uint64_t *inA,
            // output vector
            uint64_t *outB,
            // right shift amount
            int32_t shift,
            // Input and output bitwidth
            int32_t bw,
            // signed truncation?
            bool signed_arithmetic = true,
            // msb of input vector elements
            uint8_t *msb_x = nullptr);

        // Truncate (right-shift) by shift in the same ring
        void truncate_red_then_ext(
            // Size of vector
            int32_t dim,
            // input vector
            uint64_t *inA,
            // output vector
            uint64_t *outB,
            // right shift amount
            int32_t shift,
            // Input and output bitwidth
            int32_t bw,
            // signed truncation?
            bool signed_arithmetic = true,
            // msb of input vector elements
            uint8_t *msb_x = nullptr);

        // Truncate (right-shift) by shift and go to a smaller ring
        void truncate_and_reduce(
            // Size of vector
            int32_t dim,
            // input vector
            uint64_t *inA,
            // output vector
            uint64_t *outB,
            // right shift amount
            int32_t shift,
            // Input bitwidth
            int32_t bw);
    };
}
#endif // TRUNCATION_H__
