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

#ifndef FLOATING_POINT_MATH_H__
#define FLOATING_POINT_MATH_H__

#include "FloatingPoint/floating-point.h"
#include "Math/math-functions.h"

class FPMath {
public:
  int party;
  sci::IOPack *iopack;
  sci::OTPack *otpack;
  BoolOp *bool_op;
  FixOp *fix;
  FPOp *fp_op;

  FPMath(int party, sci::IOPack *iopack, sci::OTPack *otpack) {
    this->party = party;
    this->iopack = iopack;
    this->otpack = otpack;
    this->bool_op = new BoolOp(party, iopack, otpack);
    this->fix = new FixOp(party, iopack, otpack);
    this->fp_op = new FPOp(party, iopack, otpack);
  }

  ~FPMath() {
    delete bool_op;
    delete fix;
    delete fp_op;
  }

  // Floating-Point Math Functions: returns OP(x[i]), OP = {sinpi, cospi, tanpi, exp2, log2, exp, ln, erf}
  // x must be secret-shared
  FPArray sinpi(const FPArray &x);
  FPArray cospi(const FPArray &x);
  FPArray tanpi(const FPArray &x);
  FPArray exp2(const FPArray &x);
  FPArray log2(const FPArray &x);
  FPArray exp(const FPArray &x);
  FPArray ln(const FPArray &x);
  FPArray erf(const FPArray &x);
  vector<FPArray> softmax(const vector<FPArray>& x);
};

#endif // FLOATING_POINT_MATH_H__
