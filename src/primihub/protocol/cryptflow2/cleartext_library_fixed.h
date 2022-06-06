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

#ifndef CLEARTEXT_LIBRARY_FIXED_H__
#define CLEARTEXT_LIBRARY_FIXED_H__

#include "defines.h"
#include "globals.h"
#include <cassert>
#include <cmath>

#define KKOT_LIMIT 8
#define SQRT_LOOKUP_SCALE 2

static inline int64_t signed_val_cleartext(uint64_t x, int bw_x) {
  uint64_t pow_x = (bw_x == 64 ? 0ULL : (1ULL << bw_x));
  uint64_t mask_x = pow_x - 1;
  x = x & mask_x;
  return int64_t(x - ((x >= (pow_x / 2)) * pow_x));
}

static int64_t lookup_sqrt(int32_t index, int m, int exp_parity) {
  int32_t k = 1 << m;
  double u = (1.0 + (double(index) / double(k))) * (1 << exp_parity);
  double Y = 1.0 / sqrt(u);
  int32_t scale = m + SQRT_LOOKUP_SCALE;
  int64_t val = (Y * (1ULL << scale));
  return val;
}

// A0 \in (1/4, 1)
static int64_t lookup_div_A0(int64_t index, int m) {
  int32_t k = 1 << m;
  double p = 1 + (double(index) / double(k));
  double A1 = 1.0 / (p * (p + 1.0 / double(k)));
  int32_t scale = m + 3;
  int64_t val = (A1 * (1LL << scale));
  return val;
}

// A1 \in (1/2, 1)
static int64_t lookup_div_A1(int64_t index, int m) {
  int32_t k = 1 << m;
  double p = 1 + (double(index) / double(k));
  double z = (p * (p + (1.0 / double(k))));
  double A1 = ((1.0 / double(k * 2)) + sqrt(z)) / z;
  int32_t scale = 2 * m + 2;
  int64_t val = (A1 * (1LL << scale));
  return val;
}

static int64_t lookup_neg_exp(int64_t val_in, int32_t s_in, int32_t s_out) {
  if (s_in < 0) {
    s_in *= -1;
    val_in *= (1 << (s_in));
    s_in = 0;
  }
  int64_t res_val = exp(-1.0 * (val_in / double(1LL << s_in))) * (1LL << s_out);
  return res_val;
}

void cleartext_Sigmoid(int64_t *A, int I, int J, int scale_in, int scale_out,
                       int bwA, int bwB, int64_t *B);

void cleartext_TanH(int64_t *A, int I, int J, int64_t scale_in,
                    int64_t scale_out, int bwA, int bwB, int64_t *B);

void cleartext_MatAdd(int64_t *A, int64_t *B, int64_t *C, int32_t I, int32_t J,
                      int32_t shrA, int32_t shrB, int32_t shrC, int32_t demote);

void cleartext_MatSub(int64_t *A, const int64_t *B, int64_t *C, int32_t I,
                      int32_t J, int32_t shrA, int32_t shrB, int32_t shrC,
                      int32_t demote);

void cleartext_MatMul(int64_t *A, int64_t *B, int64_t *C, int64_t *tmp,
                      int32_t I, int32_t K, int32_t J, int32_t shrA,
                      int32_t shrB, int32_t H1, int32_t H2, int32_t demote);

void cleartext_MatAddBroadCastA(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote);

void cleartext_MatAddBroadCastB(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote);

void cleartext_MatSubBroadCastA(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote);

void cleartext_MatSubBroadCastB(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote);

void cleartext_ScalarMul(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                         int32_t J, int32_t shrA, int32_t shrB, int demote);

void cleartext_MulCir(int64_t *A, int64_t *B, int64_t *C, int32_t I, int32_t J,
                      int32_t shrA, int32_t shrB, int32_t demote);

void cleartext_sqrt(int64_t *A, int32_t I, int32_t J, int32_t shrA,
                    int32_t shrB, int32_t bwA, int32_t bwB, int64_t *B,
                    bool inverse = false);

void cleartext_div(int64_t *A, int64_t *B, int32_t I, int32_t J, int32_t shrA,
                   int32_t shrB, int32_t shrC, int64_t *C,
                   bool compute_msnzb = false);

void cleartext_Exp_lookup(int64_t *A, int32_t I, int32_t J, int bwA,
                          int32_t shrA, int32_t shrB, int64_t *B,
                          int32_t demote);

void cleartext_ArgMax(int64_t *A, int32_t I, int32_t J, int32_t *index);

void cleartext_Convolution(int64_t *A, int64_t *B, int64_t *C, int64_t *tmp,
                           int32_t N, int32_t H, int32_t W, int32_t CIN,
                           int32_t HF, int32_t WF, int32_t CINF, int32_t COUTF,
                           int32_t HOUT, int32_t WOUT, int32_t HPADL,
                           int32_t HPADR, int32_t WPADL, int32_t WPADR,
                           int32_t HSTR, int32_t WSTR, int32_t HDL, int32_t WDL,
                           int32_t G, int32_t shrA, int32_t shrB, int32_t H1,
                           int32_t H2, int32_t demote);

void cleartext_AddOrSubCir4D(int64_t *A, int64_t *B, int64_t *X, int32_t N,
                             int32_t H, int32_t W, int32_t C, int32_t shrA,
                             int32_t shrB, int32_t shrC, bool add,
                             int32_t demote);

void cleartext_Relu6(int64_t *A, int64_t *B, int32_t N, int32_t H, int32_t W,
                     int32_t C, int64_t six, int32_t div);

void cleartext_BNorm(int64_t *A, int64_t *BNW, int64_t *BNB, int64_t *B,
                     int32_t I, int32_t J, int32_t shA, int32_t shBNB,
                     int32_t shB);

void cleartext_MBConv(int64_t *A, int64_t *F1, int64_t *BN1W, int64_t *BN1B,
                      int64_t *F2, int64_t *BN2W, int64_t *BN2B, int64_t *F3,
                      int64_t *BN3W, int64_t *BN3B, int64_t *C, int64_t *X,
                      int64_t *T, int64_t *U, int32_t N, int32_t H, int32_t W,
                      int32_t Cin, int32_t Ct, int32_t HF, int32_t WF,
                      int32_t Cout, int32_t Hout, int32_t Wout, int32_t HPADL,
                      int32_t HPADR, int32_t WPADL, int32_t WPADR, int32_t HSTR,
                      int32_t WSTR, int32_t D1, int32_t D2, int32_t D3,
                      int64_t SIX_1, int64_t SIX_2, int64_t shr1, int64_t shr2,
                      int64_t shr3, int64_t shr4, int64_t shr5, int64_t shr6,
                      int64_t shr7, int64_t shr8, int64_t shr9, int64_t shl1,
                      int64_t shl2, int64_t shl3, int64_t shl4, int64_t shl5,
                      int64_t shl6, int64_t shl7, int64_t shl8, int64_t shl9);

void cleartext_NormaliseL2(int64_t *A, int64_t *B, int32_t N, int32_t H,
                           int32_t W, int32_t C, int32_t scaleA, int32_t shrA,
                           int32_t bwA, int32_t bwB);

void cleartext_NormaliseL2_seedot(int64_t *A, int64_t *B, int32_t N, int32_t H,
                                  int32_t W, int32_t C, int32_t scaleA,
                                  int32_t shrA);

void cleartext_AdjustScaleShr(int64_t *A, int32_t I, int32_t J, int32_t scale);

void cleartext_MaxPool2D(int64_t *A, int32_t I, int32_t J, int64_t *B);

#endif
