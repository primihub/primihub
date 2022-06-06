/*
Authors: Nishant Kumar
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

#ifndef DEFINES_UNIFORM_H___
#define DEFINES_UNIFORM_H___

#include "defines.h"
#include <cassert>
#include <chrono> //Keep the local repo based headers below, once constants are defined
#include <cstdint> //Only keep standard headers over here
#include <iostream>
#include <map>
#include <thread>

typedef uint64_t intType;
typedef int64_t signedIntType;

extern int32_t bitlength;
extern uint64_t prime_mod;

// #define NDEBUG //This must come first -- so that this marco is used
// throughout code
// Defining this will disable all asserts throughout code
// #define USE_LINEAR_UNIFORM
#define TRAINING
#ifdef TRAINING
#undef USE_LINEAR_UNIFORM // Linear Uniform only for inference
#endif
#define RUNOPTI
#ifdef RUNOPTI
#define MULTITHREADED_MATMUL
#define MULTITHREADED_NONLIN
#define MULTITHREADED_TRUNC
#define MULTITHREADED_DOTPROD
#endif

#if defined(SCI_HE)
const bool isNativeRing = false;
#elif defined(SCI_OT)
const bool isNativeRing = true;
#endif

/*
Bitlength 32 prime: 4293918721
Bitlength 33 prime: 8589475841
Bitlength 34 prime: 17179672577
Bitlength 35 prime: 34359410689
Bitlength 36 prime: 68718428161
Bitlength 37 prime: 137438822401
Bitlength 38 prime: 274876334081
Bitlength 39 prime: 549755486209
Bitlength 40 prime: 1099510054913
Bitlength 41 prime: 2199023190017
*/

static void checkIfUsingEigen() {
#ifdef USE_EIGEN
  std::cout << "Using Eigen for Matmul" << std::endl;
#else
  std::cout << "Using normal Matmul" << std::endl;
#endif
}

static void assertFieldRun() {
  assert(sizeof(intType) == sizeof(uint64_t));
  assert(sizeof(signedIntType) == sizeof(int64_t));
  assert(bitlength >= 32 && bitlength <= 41);
}

#endif // DEFINES_UNIFORM_H__
