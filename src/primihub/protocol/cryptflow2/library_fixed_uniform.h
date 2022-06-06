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

#ifndef LIBRARY_FIXED_UNIFORM_H__
#define LIBRARY_FIXED_UNIFORM_H__

#include "defines_uniform.h"
#include "utils/ArgMapping/ArgMapping.h"

// Note of the bracket around each expression use -- if this is not there, not
// macro expansion
//  can result in hard in trace bugs when expanded around expressions like 1-2.
#define Arr1DIdxRowM(arr, s0, i) (*((arr) + (i)))
#define Arr2DIdxRowM(arr, s0, s1, i, j) (*((arr) + (i) * (s1) + (j)))
#define Arr3DIdxRowM(arr, s0, s1, s2, i, j, k)                                 \
  (*((arr) + (i) * (s1) * (s2) + (j) * (s2) + (k)))
#define Arr4DIdxRowM(arr, s0, s1, s2, s3, i, j, k, l)                          \
  (*((arr) + (i) * (s1) * (s2) * (s3) + (j) * (s2) * (s3) + (k) * (s3) + (l)))
#define Arr5DIdxRowM(arr, s0, s1, s2, s3, s4, i, j, k, l, m)                   \
  (*((arr) + (i) * (s1) * (s2) * (s3) * (s4) + (j) * (s2) * (s3) * (s4) +      \
     (k) * (s3) * (s4) + (l) * (s4) + (m)))

#define Arr2DIdxColM(arr, s0, s1, i, j) (*((arr) + (j) * (s0) + (i)))

intType funcSSCons(int64_t x);
void funcReconstruct2PCCons(signedIntType *y, const intType *x, int len);
signedIntType funcReconstruct2PCCons(intType x, int revealParty);

void MatMul2D(int32_t s1, int32_t s2, int32_t s3, const intType *A,
              const intType *B, intType *C, bool modelIsA);

void Conv2DWrapper(signedIntType N, signedIntType H, signedIntType W,
                   signedIntType CI, signedIntType FH, signedIntType FW,
                   signedIntType CO, signedIntType zPadHLeft,
                   signedIntType zPadHRight, signedIntType zPadWLeft,
                   signedIntType zPadWRight, signedIntType strideH,
                   signedIntType strideW, intType *inputArr, intType *filterArr,
                   intType *outArr);

void Conv2DGroupWrapper(signedIntType N, signedIntType H, signedIntType W,
                        signedIntType CI, signedIntType FH, signedIntType FW,
                        signedIntType CO, signedIntType zPadHLeft,
                        signedIntType zPadHRight, signedIntType zPadWLeft,
                        signedIntType zPadWRight, signedIntType strideH,
                        signedIntType strideW, signedIntType G,
                        intType *inputArr, intType *filterArr, intType *outArr);

void ConvTranspose2DWrapper(int32_t N, int32_t HPrime, int32_t WPrime,
                               int32_t CI, int32_t FH, int32_t FW,
                               int32_t CO, int32_t H, int32_t W,
                               int32_t zPadTrHLeft, int32_t zPadTrHRight,
                               int32_t zPadTrWLeft, int32_t zPadTrWRight,
                               int32_t strideH, int32_t strideW,
                               uint64_t* inputArr, uint64_t* filterArr,
                               uint64_t* outArr);

void ElemWiseActModelVectorMult(int32_t size, intType *inArr,
                                intType *multArrVec, intType *outputArr);

void ArgMax(int32_t s1, int32_t s2, intType *inArr, intType *outArr);

void Min(int32_t size, intType *inArr, int32_t alpha, intType *outArr, int sf, bool doTruncation) ;

void Max(int32_t size, intType *inArr, int32_t alpha, intType *outArr, int sf, bool doTruncation) ;

void Relu(int32_t size, intType *inArr, intType *outArr, int sf,
          bool doTruncation);

// void Clip(int32_t size, int64_t alpha, int64_t beta, intType *inArr, intType *outArr, int sf, bool doTruncation) ;

void HardSigmoid(int32_t size, intType *inArr, intType *outArr, int sf, bool doTruncation);

void MaxPool(int32_t N, int32_t H, int32_t W, int32_t C, int32_t ksizeH,
             int32_t ksizeW, int32_t zPadHLeft, int32_t zPadHRight,
             int32_t zPadWLeft, int32_t zPadWRight, int32_t strideH,
             int32_t strideW, int32_t N1, int32_t imgH, int32_t imgW,
             int32_t C1, intType *inArr, intType *outArr);

void AvgPool(int32_t N, int32_t H, int32_t W, int32_t C, int32_t ksizeH,
             int32_t ksizeW, int32_t zPadHLeft, int32_t zPadHRight,
             int32_t zPadWLeft, int32_t zPadWRight, int32_t strideH,
             int32_t strideW, int32_t N1, int32_t imgH, int32_t imgW,
             int32_t C1, intType *inArr, intType *outArr);

void ScaleDown(int32_t size, intType *inArr, int32_t sf);

void ScaleUp(int32_t size, intType *arr, int32_t sf);

void StartComputation();

void EndComputation();

intType SecretAdd(intType x, intType y);

intType SecretSub(intType x, intType y);

intType SecretMult(intType x, intType y);

void ElemWiseVectorPublicDiv(int32_t s1, intType *arr1, int32_t divisor,
                             intType *outArr);

void ElemWiseSecretSharedVectorMult(int32_t size, intType *inArr,
                                    intType *multArrVec, intType *outputArr);

void Floor(int32_t s1, intType *inArr, intType *outArr, int32_t sf);

inline void ClearMemSecret1(int32_t s1, intType *arr) { delete[] arr; }

inline void ClearMemSecret2(int32_t s1, int32_t s2, intType *arr) {
  delete[] arr; // At the end of the day, everything is done using 1D array
}

inline void ClearMemSecret3(int32_t s1, int32_t s2, int32_t s3, intType *arr) {
  delete[] arr;
}

inline void ClearMemSecret4(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            intType *arr) {
  delete[] arr;
}

inline void ClearMemSecret5(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            int32_t s5, intType *arr) {
  delete[] arr;
}

inline void ClearMemPublic(int32_t x) { return; }

inline void ClearMemPublic1(int32_t s1, int32_t *arr) { delete[] arr; }

inline void ClearMemPublic2(int32_t s1, int32_t s2, int32_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic3(int32_t s1, int32_t s2, int32_t s3, int32_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic4(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            int32_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic5(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            int32_t s5, int32_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic(int64_t x) { return; }

inline void ClearMemPublic1(int32_t s1, int64_t *arr) { delete[] arr; }

inline void ClearMemPublic2(int32_t s1, int32_t s2, int64_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic3(int32_t s1, int32_t s2, int32_t s3, int64_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic4(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            int64_t *arr) {
  delete[] arr;
}

inline void ClearMemPublic5(int32_t s1, int32_t s2, int32_t s3, int32_t s4,
                            int32_t s5, int64_t *arr) {
  delete[] arr;
}

template <typename T> T *make_array(size_t s1) { return new T[s1]; }

template <typename T> T *make_array(size_t s1, size_t s2) {
  return new T[s1 * s2];
}

template <typename T> T *make_array(size_t s1, size_t s2, size_t s3) {
  return new T[s1 * s2 * s3];
}

template <typename T>
T *make_array(size_t s1, size_t s2, size_t s3, size_t s4) {
  return new T[s1 * s2 * s3 * s4];
}

template <typename T>
T *make_array(size_t s1, size_t s2, size_t s3, size_t s4, size_t s5) {
  return new T[s1 * s2 * s3 * s4 * s5];
}

#endif
