/*
Authors: Deevashwer Rathee, Mayank Rathee
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

#ifndef AUX_PROTOCOLS_H__
#define AUX_PROTOCOLS_H__

#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire.h"
#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire_with_equality.h"
#include "src/primihub/protocol/cryptflow2/OT/emp-ot.h"
#include "src/primihub/protocol/cryptflow2/GC/emp-sh2pc.h"

namespace primihub::cryptflow2
{

    class AuxProtocols
    {
    public:
        int party;
        primihub::sci::IOPack *iopack;
        primihub::sci::OTPack *otpack;
        MillionaireProtocol *mill;
        MillionaireWithEquality *mill_and_eq;

        AuxProtocols(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack);

        ~AuxProtocols();

        void wrap_computation(
            // input vector
            uint64_t *x,
            // wrap-bit of shares of x
            uint8_t *y,
            // size of input vector
            int32_t size,
            // bitwidth of x
            int32_t bw_x);

        // y = sel * x
        void multiplexer(
            // selection bits
            uint8_t *sel,
            // input vector
            uint64_t *x,
            // output vector
            uint64_t *y,
            // size of vectors
            int32_t size,
            // bitwidth of x
            int32_t bw_x,
            // bitwidth of y
            int32_t bw_y);

        // Boolean to Arithmetic Shares
        void B2A(
            // input (boolean) vector
            uint8_t *x,
            // output vector
            uint64_t *y,
            // size of vector
            int32_t size,
            // bitwidth of y
            int32_t bw_y);

        template <typename T>
        void lookup_table(
            // table specification
            T **spec,
            // input vector
            T *x,
            // output vector
            T *y,
            // size of vector
            int32_t size,
            // bitwidth of input to LUT
            int32_t bw_x,
            // bitwidth of output of LUT
            int32_t bw_y);

        // MSB computation
        void MSB(
            // input vector
            uint64_t *x,
            // shares of MSB(x)
            uint8_t *msb_x,
            // size of input vector
            int32_t size,
            // bitwidth of x
            int32_t bw_x);

        // MSB to Wrap computation
        void MSB_to_Wrap(
            // input vector
            uint64_t *x,
            // shares of MSB(x)
            uint8_t *msb_x,
            // output shares of Wrap(x)
            uint8_t *wrap_x,
            // size of input vector
            int32_t size,
            // bitwidth of x
            int32_t bw_x);

        // Bitwise AND
        void AND(
            // input A (boolean) vector
            uint8_t *x,
            // input B (boolean) vector
            uint8_t *y,
            // output vector
            uint8_t *z,
            // size of vector
            int32_t size);

        void digit_decomposition(int32_t dim, uint64_t *x, uint64_t *x_digits,
                                 int32_t bw_x, int32_t digit_size);

        void digit_decomposition_sci(
            int32_t dim, uint64_t *x, uint64_t *x_digits, int32_t bw_x,
            int32_t digit_size,
            // set true if the bitlength of all output digits is digit_size
            // leave false, if the last digit is output over <= digit_size bits
            bool all_digit_size = false);

        void reduce(int32_t dim, uint64_t *x, uint64_t *y, int32_t bw_x,
                    int32_t bw_y);

        // Make x positive: pos_x = x * (1 - 2*MSB(x))
        void make_positive(
            // input vector
            uint64_t *x,
            // input (boolean) vector containing MSB(x)
            uint8_t *msb_x,
            // output vector
            uint64_t *pos_x,
            // size of vector
            int32_t size);

        // Outputs index and not one-hot vector
        void msnzb_sci(uint64_t *x, uint64_t *msnzb_index, int32_t bw_x, int32_t size,
                       int32_t digit_size = 8);

        // Wrapper over msnzb_sci. Outputs one-hot vector
        void msnzb_one_hot(uint64_t *x,
                           // size: bw_x * size
                           uint8_t *one_hot_vector, int32_t bw_x, int32_t size,
                           int32_t digit_size = 8);

        void msnzb_GC(uint64_t *x,
                      // size: bw_x * size
                      uint8_t *one_hot_vector, int32_t bw_x, int32_t size);
    };
}
#endif
