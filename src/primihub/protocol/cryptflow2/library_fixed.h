/*
Authors: Deevashwer Rathee, G Rahul Kranti Kiran
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

#ifndef LIBRARY_FIXED_H__
#define LIBRARY_FIXED_H__

#include "cleartext_library_fixed.h"
#include "defines.h"
#include "library_fixed_uniform.h"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

static int32_t ctr = 1;

inline std::vector<int> divide_instances(
    // Total number of threads
    int num_threads,
    // Number of instances
    int num_instances,
    // Minimum chunk size of each thread
    int min_chunk_size = THREADING_MIN_CHUNK_SIZE) {
  int lnum_threads =
      std::min(int(ceil(num_instances / double(min_chunk_size))), num_threads);
  std::vector<int> chunks_per_thread(lnum_threads);
  int chunk_size = num_instances / lnum_threads;
  for (int i = 0; i < lnum_threads; ++i) {
    int offset = i * chunk_size;
    if (i == (lnum_threads - 1)) {
      chunks_per_thread[i] = num_instances - offset;
    } else {
      chunks_per_thread[i] = chunk_size;
    }
  }
  return chunks_per_thread;
}

void initialize();

void finalize();

void reconstruct(int dim, uint64_t *x, uint64_t *y, int bw_x);

void AdjustScaleShr(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
                    int32_t scale);

void AdjustScaleShr(int32_t I, int32_t J, int32_t scale, int64_t bwA,
                    int64_t *A);

void MatAdd(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
            int32_t bwA, int32_t bwB, int32_t bwC, int32_t bwTemp, int32_t shrA,
            int32_t shrB, int32_t shrC, int32_t demote,
            bool subroutine = false);

void MatAddBroadCast(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I,
                     int32_t J, int32_t bwA, int32_t bwB, int32_t bwC,
                     int32_t bwTemp, int32_t shrA, int32_t shrB, int32_t shrC,
                     int32_t demote, bool scalar_A = true);

void AddOrSubCir(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
                 int32_t bwA, int32_t bwB, int32_t bwC, int32_t bwTemp,
                 int32_t shrA, int32_t shrB, int32_t shrC, bool add,
                 int32_t demote);

void AddOrSubCir4D(int32_t N, int32_t H, int32_t W, int32_t C, int32_t shrA,
                   int32_t shrB, int32_t shrC, bool add, int32_t demote,
                   int32_t bwA, int32_t bwB, int32_t bwTemp, int32_t bwC,
                   int64_t *A, int64_t *B, int64_t *X);

void Exp(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
         int32_t bwB, int32_t sA, int32_t sB);

void Div(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
         int32_t bwA, int32_t bwB, int32_t bwC, int32_t sA, int32_t sB,
         int32_t sC);

void ArgMax(uint64_t *A, int32_t I, int32_t J, int32_t bwA, int32_t bw_index,
            uint64_t *index);

void MaxPool2D(uint64_t *A, int32_t I, int32_t J, int32_t bwA, int32_t bwB,
               uint64_t *B);

void MaxPool2D(int I, int J, int bwA, int bwB, int64_t *A, int64_t *B);

void Convolution(int32_t N, int32_t H, int32_t W, int32_t CIN, int32_t HF,
                 int32_t WF, int32_t CINF, int32_t COUTF, int32_t HOUT,
                 int32_t WOUT, int32_t HPADL, int32_t HPADR, int32_t WPADL,
                 int32_t WPADR, int32_t HSTR, int32_t WSTR, int32_t HDL,
                 int32_t WDL, int32_t G, int32_t bwA, int32_t bwB, int32_t bwC,
                 int32_t bwTemp, int32_t shrA, int32_t shrB, int32_t H1,
                 int32_t H2, int32_t demote, uint64_t *A, uint64_t *B,
                 uint64_t *C);

void Convolution(int32_t N, int32_t H, int32_t W, int32_t CIN, int32_t HF,
                 int32_t WF, int32_t CINF, int32_t COUTF, int32_t HOUT,
                 int32_t WOUT, int32_t HPADL, int32_t HPADR, int32_t WPADL,
                 int32_t WPADR, int32_t HSTR, int32_t WSTR, int32_t HDL,
                 int32_t WDL, int32_t G, int32_t shrA, int32_t shrB, int32_t H1,
                 int32_t H2, int32_t demote, int32_t bwA, int32_t bwB,
                 int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B,
                 int64_t *C, int64_t *tmp, bool verbose = true);

void ReLU(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
          int32_t bwB, uint64_t six, int32_t div);
void Relu6(int32_t N, int32_t H, int32_t W, int32_t C, int64_t six, int32_t div,
           int32_t bwA, int32_t bwB, int64_t *A, int64_t *B);

void BNorm(uint64_t *A, uint64_t *BNW, uint64_t *BNB, uint64_t *B, int32_t I,
           int32_t J, int32_t bwA, int32_t bwBNW, int32_t bwBNB, int32_t bwTemp,
           int32_t bwB, int32_t shA, int32_t shBNB, int32_t shB);

void NormaliseL2(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
                 int32_t scaleA, int32_t shrA);
void BNorm(int32_t I, int32_t J, int32_t shA, int32_t shBNB, int32_t shB,
           int32_t bwA, int32_t bwBNW, int32_t bwBNB, int32_t bwTemp,
           int32_t bwB, int64_t *A, int64_t *BNW, int64_t *BNB, int64_t *B);

void NormaliseL2(int32_t N, int32_t H, int32_t W, int32_t C, int32_t scaleA,
                 int32_t shrA, int32_t bwA, int64_t *A, int64_t *B);

void MBConv(int32_t N, int32_t H, int32_t W, int32_t Cin, int32_t Ct,
            int32_t HF, int32_t WF, int32_t Cout, int32_t Hout, int32_t Wout,
            int32_t HPADL, int32_t HPADR, int32_t WPADL, int32_t WPADR,
            int32_t HSTR, int32_t WSTR, int32_t D1, int32_t D2, int32_t D3,
            int64_t SIX_1, int64_t SIX_2, int32_t shr1, int32_t shr2,
            int32_t shr3, int32_t shr4, int32_t shr5, int32_t shr6,
            int32_t shr7, int32_t shr8, int32_t shr9, int32_t shl1,
            int32_t shl2, int32_t shl3, int32_t shl4, int32_t shl5,
            int32_t shl6, int32_t shl7, int32_t shl8, int32_t shl9, int32_t bwA,
            int32_t bwF1, int32_t bwB1W, int32_t bwB1B, int32_t bwF2,
            int32_t bwB2W, int32_t bwB2B, int32_t bwF3, int32_t bwB3W,
            int32_t bwB3B, int32_t bwC, int32_t bwX, int32_t bwT, int32_t bwU,
            int32_t bwUB1W, int32_t bwUB2W, int32_t bwUB3W, int64_t *A,
            int64_t *F1, int64_t *BN1W, int64_t *BN1B, int64_t *F2,
            int64_t *BN2W, int64_t *BN2B, int64_t *F3, int64_t *BN3W,
            int64_t *BN3B, int64_t *C, int64_t *X, int64_t *T, int64_t *U);

void output_vector(int64_t *x, int32_t I, int32_t J, int32_t bwX);

/*
    MatAdd fucntion for EzPC compatibility followed by MatAdd
    function developed using SeeDot.
*/
void MatAdd(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t shrC,
            int64_t demote, int64_t bwA, int64_t bwB, int64_t bwTemp,
            int64_t bwC, int64_t *A, int64_t *B, int64_t *C,
            bool verbose = true);

void MatAdd4(int32_t N, int32_t H, int32_t W, int32_t C, int32_t shrA,
             int32_t shrB, int32_t shrC, int32_t demote, int32_t bwA,
             int32_t bwB, int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B,
             int64_t *X);

/**/
void MatSub(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t shrC,
            int64_t demote, int64_t bwA, int64_t bwB, int64_t bwTemp,
            int64_t bwC, int64_t *A, int64_t *B, int64_t *C);

/**/
void MatAddBroadCastA(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t A, int64_t *B,
                      int64_t *C, bool verbose = true);

/**/
void MatAddBroadCastB(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t *A, int64_t B,
                      int64_t *C, bool verbose = true);

/**/
void MatSubBroadCastA(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t A, int64_t *B,
                      int64_t *C);

/**/
void MatSubBroadCastB(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t *A, int64_t B,
                      int64_t *C);

/**/
void MulCir(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
            int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, uint64_t *A,
            uint64_t *B, uint64_t *C);

void MulCir(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
            int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, int64_t *A,
            int64_t *B, int64_t *C);

void MatMul(int64_t I, int64_t K, int64_t J, int64_t shrA, int64_t shrB,
            int64_t H1, int64_t H2, int64_t demote, int32_t bwA, int32_t bwB,
            int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B, int64_t *C,
            int64_t *tmp, bool verbose = true);

void MatMul(int64_t I, int64_t K, int64_t J, int64_t shrA, int64_t shrB,
            int64_t H1, int64_t H2, int64_t demote, int32_t bwA, int32_t bwB,
            int32_t bwTemp, int32_t bwC, uint64_t *A, uint64_t *B, uint64_t *C,
            uint64_t *tmp, bool verbose = true);

/**/
void ScalarMul(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
               int32_t bwA, int32_t bwB, int32_t bwTemp, int32_t bwC,
               int32_t shrA, int32_t shrB, int32_t demote);

void ScalarMul(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
               int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, int64_t A,
               int64_t *B, int64_t *C);

/**/

void Sigmoid(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
             int64_t bwA, int64_t bwB, uint64_t *A, uint64_t *B);

void Sigmoid(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
             int64_t bwA, int64_t bwB, int64_t *A, int64_t *B);

/**/
void TanH(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, int64_t *A, int64_t *B);

void TanH(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, uint64_t *A, uint64_t *B);

void Sqrt(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, bool inverse, uint64_t *A, uint64_t *B);

void reconstruct(int64_t *A, int64_t *B, int32_t I, int32_t J, int bwA);

// template<class int64_t>
void AdjustScaleShl(int64_t I, int64_t J, int64_t scale, int64_t *A);

// template<class int64_t>
void ArgMax(int64_t I, int64_t J, int32_t bwA, int32_t bw_index, int64_t *A,
            int64_t *index);

void typecast_to_uint64(int64_t *A, uint64_t *A64, int32_t I, int32_t J,
                        int32_t bwA);

void typecast_from_uint64(uint64_t *A64, int64_t *A, int32_t I, int32_t J,
                          int bwA);

void Exp(int32_t I, int32_t J, int32_t shrA, int32_t shrB, int32_t bwA,
         int64_t *A, int64_t *B);

void Div(int32_t I, int32_t J, int32_t shrA, int32_t shrB, int32_t shrC,
         int32_t bwA, int64_t *A, int64_t *B, int64_t *C);

// Athos Wrappers
#ifdef SCI_OT
void Sigmoid(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
             int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B);

void TanH(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B);

void Sqrt(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, bool inverse, uint64_t *A, uint64_t *B);
#endif

#ifdef SCI_HE
void Sigmoid(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
             int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B) {
  assert(false && "Sigmoid not supported in SCI_HE.");
}

void TanH(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B) {
  assert(false && "TanH not supported in SCI_HE.");
}

void Sqrt(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, bool inverse, uint64_t *A, uint64_t *B) {
  assert(false && "Sqrt not supported in SCI_HE.");
}
#endif

#endif
