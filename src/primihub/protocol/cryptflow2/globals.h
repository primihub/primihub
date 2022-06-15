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

#ifndef GLOBALS_H___
#define GLOBALS_H___

#include "BuildingBlocks/aux-protocols.h"
#include "BuildingBlocks/truncation.h"
#include "Math/math-functions.h"
#include "NonLinear/argmax.h"
#include "NonLinear/maxpool.h"
#include "NonLinear/relu-interface.h"
#include "defines.h"
#include "defines_uniform.h"
#include <chrono>
#include <cstdint>
#include <thread>
#ifdef SCI_OT
#include "LinearOT/linear-ot.h"
#include "LinearOT/linear-uniform.h"
#endif
// Additional Headers for Athos
#ifdef SCI_HE
#include "LinearHE/conv-field.h"
#include "LinearHE/elemwise-prod-field.h"
#include "LinearHE/fc-field.h"
#endif

// #define MULTI_THREADING

#define MAX_THREADS 4

extern primihub::sci::NetIO *io;
extern primihub::sci::IOPack *iopack;
extern primihub::sci::OTPack *otpack;

extern primihub::cryptflow2::AuxProtocols *aux;
extern primihub::cryptflow2::Truncation *truncation;
extern primihub::cryptflow2::XTProtocol *xt;
#ifdef SCI_OT
extern primihub::cryptflow2::LinearOT *mult;
#endif
extern primihub::cryptflow2::MathFunctions *math;
extern primihub::cryptflow2::ArgMaxProtocol<intType> *argmax;
extern primihub::cryptflow2::ReLUProtocol<intType> *relu;
extern primihub::cryptflow2::MaxPoolProtocol<intType> *maxpool;
// Additional classes for Athos
#ifdef SCI_OT
extern primihub::cryptflow2::MatMulUniform<primihub::sci::NetIO, intType, primihub::sci::IKNP<primihub::sci::NetIO>> *multUniform;
#endif
#ifdef SCI_HE
extern primihub::cryptflow2::ConvField *he_conv;
extern primihub::cryptflow2::FCField *he_fc;
extern primihub::cryptflow2::ElemWiseProdField *he_prod;
#endif
extern primihub::sci::IKNP<primihub::sci::NetIO> *iknpOT;
extern primihub::sci::IKNP<primihub::sci::NetIO> *iknpOTRoleReversed;
extern primihub::sci::KKOT<primihub::sci::NetIO> *kkot;
extern primihub::sci::PRG128 *prg128Instance;

extern primihub::sci::NetIO *ioArr[MAX_THREADS];
extern primihub::sci::IOPack *iopackArr[MAX_THREADS];
extern primihub::sci::OTPack *otpackArr[MAX_THREADS];
extern primihub::cryptflow2::MathFunctions *mathArr[MAX_THREADS];
#ifdef SCI_OT
extern primihub::cryptflow2::LinearOT *multArr[MAX_THREADS];
#endif
extern primihub::cryptflow2::AuxProtocols *auxArr[MAX_THREADS];
extern primihub::cryptflow2::Truncation *truncationArr[MAX_THREADS];
extern primihub::cryptflow2::XTProtocol *xtArr[MAX_THREADS];
extern primihub::cryptflow2::ReLUProtocol<intType> *reluArr[MAX_THREADS];
extern primihub::cryptflow2::MaxPoolProtocol<intType> *maxpoolArr[MAX_THREADS];
// Additional classes for Athos
#ifdef SCI_OT
extern primihub::cryptflow2::MatMulUniform<primihub::sci::NetIO, intType, primihub::sci::IKNP<primihub::sci::NetIO>>
    *multUniformArr[MAX_THREADS];
#endif
extern primihub::sci::IKNP<primihub::sci::NetIO> *otInstanceArr[MAX_THREADS];
extern primihub::sci::KKOT<primihub::sci::NetIO> *kkotInstanceArr[MAX_THREADS];
extern primihub::sci::PRG128 *prgInstanceArr[MAX_THREADS];

extern std::chrono::time_point<std::chrono::high_resolution_clock> st_time;
extern uint64_t comm_threads[MAX_THREADS];
extern uint64_t num_rounds;

#ifdef LOG_LAYERWISE
extern uint64_t ConvTimeInMilliSec;
extern uint64_t MatAddTimeInMilliSec;
extern uint64_t BatchNormInMilliSec;
extern uint64_t TruncationTimeInMilliSec;
extern uint64_t ReluTimeInMilliSec;
extern uint64_t MaxpoolTimeInMilliSec;
extern uint64_t AvgpoolTimeInMilliSec;
extern uint64_t MatMulTimeInMilliSec;
extern uint64_t MatAddBroadCastTimeInMilliSec;
extern uint64_t MulCirTimeInMilliSec;
extern uint64_t ScalarMulTimeInMilliSec;
extern uint64_t SigmoidTimeInMilliSec;
extern uint64_t TanhTimeInMilliSec;
extern uint64_t SqrtTimeInMilliSec;
extern uint64_t NormaliseL2TimeInMilliSec;
extern uint64_t ArgMaxTimeInMilliSec;

extern uint64_t ConvCommSent;
extern uint64_t MatAddCommSent;
extern uint64_t BatchNormCommSent;
extern uint64_t TruncationCommSent;
extern uint64_t ReluCommSent;
extern uint64_t MaxpoolCommSent;
extern uint64_t AvgpoolCommSent;
extern uint64_t MatMulCommSent;
extern uint64_t MatAddBroadCastCommSent;
extern uint64_t MulCirCommSent;
extern uint64_t ScalarMulCommSent;
extern uint64_t SigmoidCommSent;
extern uint64_t TanhCommSent;
extern uint64_t SqrtCommSent;
extern uint64_t NormaliseL2CommSent;
extern uint64_t ArgMaxCommSent;
#endif

#endif // GLOBALS_H__
