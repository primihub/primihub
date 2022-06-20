/*
Authors: Nishant Kumar, Deevashwer Rathee
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

#include "globals.h"
using namespace primihub::cryptflow2;
primihub::sci::NetIO *io;
primihub::sci::IOPack *iopack;
primihub::sci::OTPack *otpack;

AuxProtocols *aux;
Truncation *truncation;
XTProtocol *xt;
#ifdef SCI_OT
LinearOT *mult;
#endif
MathFunctions *math;
ArgMaxProtocol<intType> *argmax;
ReLUProtocol<intType> *relu;
MaxPoolProtocol<intType> *maxpool;
// Additional classes for Athos
#ifdef SCI_OT
MatMulUniform<primihub::sci::NetIO, intType, primihub::sci::IKNP<primihub::sci::NetIO>> *multUniform;
#endif
#ifdef SCI_HE
ConvField *he_conv;
FCField *he_fc;
ElemWiseProdField *he_prod;
#endif
primihub::sci::IKNP<primihub::sci::NetIO> *iknpOT;
primihub::sci::IKNP<primihub::sci::NetIO> *iknpOTRoleReversed;
primihub::sci::KKOT<primihub::sci::NetIO> *kkot;
primihub::sci::PRG128 *prg128Instance;

primihub::sci::NetIO *ioArr[MAX_THREADS];
primihub::sci::IOPack *iopackArr[MAX_THREADS];
primihub::sci::OTPack *otpackArr[MAX_THREADS];
MathFunctions *mathArr[MAX_THREADS];
#ifdef SCI_OT
LinearOT *multArr[MAX_THREADS];
#endif
AuxProtocols *auxArr[MAX_THREADS];
Truncation *truncationArr[MAX_THREADS];
XTProtocol *xtArr[MAX_THREADS];
ReLUProtocol<intType> *reluArr[MAX_THREADS];
MaxPoolProtocol<intType> *maxpoolArr[MAX_THREADS];
// Additional classes for Athos
#ifdef SCI_OT
MatMulUniform<primihub::sci::NetIO, intType, primihub::sci::IKNP<primihub::sci::NetIO>>
    *multUniformArr[MAX_THREADS];
#endif
primihub::sci::IKNP<primihub::sci::NetIO> *otInstanceArr[MAX_THREADS];
primihub::sci::KKOT<primihub::sci::NetIO> *kkotInstanceArr[MAX_THREADS];
primihub::sci::PRG128 *prgInstanceArr[MAX_THREADS];

std::chrono::time_point<std::chrono::high_resolution_clock> st_time;
uint64_t comm_threads[MAX_THREADS];
uint64_t num_rounds;

#ifdef LOG_LAYERWISE
uint64_t ConvTimeInMilliSec = 0;
uint64_t MatAddTimeInMilliSec = 0;
uint64_t BatchNormInMilliSec = 0;
uint64_t TruncationTimeInMilliSec = 0;
uint64_t ReluTimeInMilliSec = 0;
uint64_t MaxpoolTimeInMilliSec = 0;
uint64_t AvgpoolTimeInMilliSec = 0;
uint64_t MatMulTimeInMilliSec = 0;
uint64_t MatAddBroadCastTimeInMilliSec = 0;
uint64_t MulCirTimeInMilliSec = 0;
uint64_t ScalarMulTimeInMilliSec = 0;
uint64_t SigmoidTimeInMilliSec = 0;
uint64_t TanhTimeInMilliSec = 0;
uint64_t SqrtTimeInMilliSec = 0;
uint64_t NormaliseL2TimeInMilliSec = 0;
uint64_t ArgMaxTimeInMilliSec = 0;

uint64_t ConvCommSent = 0;
uint64_t MatAddCommSent = 0;
uint64_t BatchNormCommSent = 0;
uint64_t TruncationCommSent = 0;
uint64_t ReluCommSent = 0;
uint64_t MaxpoolCommSent = 0;
uint64_t AvgpoolCommSent = 0;
uint64_t MatMulCommSent = 0;
uint64_t MatAddBroadCastCommSent = 0;
uint64_t MulCirCommSent = 0;
uint64_t ScalarMulCommSent = 0;
uint64_t SigmoidCommSent = 0;
uint64_t TanhCommSent = 0;
uint64_t SqrtCommSent = 0;
uint64_t NormaliseL2CommSent = 0;
uint64_t ArgMaxCommSent = 0;
#endif
