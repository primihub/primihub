/*
Authors: Anwesh Bhattacharya
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

#ifndef GLOBALS_FLOAT_H___
#define GLOBALS_FLOAT_H___

#include "defines_float.h"
#include "FloatingPoint/floating-point.h"
#include "FloatingPoint/fp-math.h"

using namespace sci ;
using namespace std ;

extern IOPack *iopackArr[MAX_THREADS] ;
extern OTPack *otpackArr[MAX_THREADS] ;

extern BoolOp *boolopArr[MAX_THREADS] ;
extern FixOp *fixopArr[MAX_THREADS] ;
extern FPOp *fpopArr[MAX_THREADS] ;
extern FPMath *fpmathArr[MAX_THREADS] ;

#endif