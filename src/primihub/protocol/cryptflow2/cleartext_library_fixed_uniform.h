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

#ifndef CLEARTEXT_LIBRARY_FIXED_CSF_H__
#define CLEARTEXT_LIBRARY_FIXED_CSF_H__

#include <Eigen/Dense>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

extern uint64_t prime_mod;
extern uint64_t moduloMask;
extern uint64_t moduloMidPt;

typedef std::vector<uint64_t> uint64_1D;
typedef std::vector<std::vector<uint64_t>> uint64_2D;
typedef std::vector<std::vector<std::vector<uint64_t>>> uint64_3D;
typedef std::vector<std::vector<std::vector<std::vector<uint64_t>>>> uint64_4D;
typedef std::vector<
    std::vector<std::vector<std::vector<std::vector<uint64_t>>>>>
    uint64_5D;

#if defined(SCI_OT)

void div_floor(int64_t a, int64_t b, int64_t &quot, int64_t &rem) {
  assert(b > 0);
  int64_t q = a / b;
  int64_t r = a % b;
  int64_t corr = ((r != 0) && (r < 0));
  quot = q - corr;
  rem = (r + b) % b;
}

inline int64_t getSignedVal(uint64_t x) {
  assert(x < prime_mod);
  int64_t sx = x;
  if (x >= moduloMidPt)
    sx = x - prime_mod;
  return sx;
}

inline uint64_t getRingElt(int64_t x) { return ((uint64_t)x) & moduloMask; }

inline uint64_t PublicAdd(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x + y) & moduloMask;
}

inline uint64_t PublicSub(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x - y) & moduloMask;
}

inline uint64_t PublicMult(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x * y) & moduloMask; // This works because its a two-power ring
}

inline bool PublicGT(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx > sy);
}

inline bool PublicGTE(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx >= sy);
}

inline bool PublicLT(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx < sy);
}

inline bool PublicLTE(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx <= sy);
}

uint64_t PublicDiv(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  int64_t q, r;
  div_floor(sx, sy, q, r);
  return getRingElt(q);
}

uint64_t PublicMod(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  int64_t q, r;
  div_floor(sx, sy, q, r);
  return r;
}

inline uint64_t PublicRShiftA(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  int64_t sx = getSignedVal(x);
  int64_t ans = sx >> y;
  return getRingElt(ans);
}

inline uint64_t PublicRShiftL(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x >> y);
}

inline uint64_t PublicLShift(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x << y) & moduloMask;
}

#else

// Assumption at some places in the following code is that 2*mod < (1<<64)
//  which allows things like (x+y)%(1<<64).
uint64_t moduloMult(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t res = 0;
  a %= mod;
  while (b) {
    if (b & 1)
      res = (res + a) % mod;
    a = (2 * a) % mod;
    b >>= 1;
  }
  return res;
}

void div_floor(int64_t a, int64_t b, int64_t &quot, int64_t &rem) {
  assert(b > 0);
  int64_t q = a / b;
  int64_t r = a % b;
  int64_t corr = ((r != 0) && (r < 0));
  quot = q - corr;
  rem = (r + b) % b;
}

int64_t getSignedVal(uint64_t x) {
  assert(x < prime_mod);
  bool xPos;
  if (prime_mod & 1)
    xPos = (x <= (prime_mod / 2));
  else
    xPos = (x < (prime_mod / 2));
  int64_t sx = x;
  if (!xPos)
    sx = x - prime_mod;
  return sx;
}

uint64_t getRingElt(int64_t x) {
  if (x > 0)
    return x % prime_mod;
  else {
    int64_t y = -x;
    int64_t temp = prime_mod - y;
    int64_t temp1 = temp % ((int64_t)prime_mod);
    uint64_t ans = (temp1 + prime_mod) % prime_mod;
    return ans;
  }
}

uint64_t PublicAdd(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x + y) % prime_mod;
}

uint64_t PublicSub(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  uint64_t ans;
  if (x >= y)
    ans = (x - y) % prime_mod;
  else
    ans = ((x + prime_mod) - y) % prime_mod;
  return ans;
}

uint64_t PublicMult(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
#ifdef __SIZEOF_INT128__
  __int128 ix = x;
  __int128 iy = y;
  __int128 iz = ix * iy;

  return iz % prime_mod;
#else
  return moduloMult(x, y, prime_mod);
#endif
}

bool PublicGT(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx > sy);
}

bool PublicGTE(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx >= sy);
}

bool PublicLT(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx < sy);
}

bool PublicLTE(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  return (sx <= sy);
}

uint64_t PublicDiv(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  int64_t q, r;
  div_floor(sx, sy, q, r);
  return getRingElt(q);
}

uint64_t PublicMod(uint64_t x, uint64_t y) {
  int64_t sx = getSignedVal(x);
  int64_t sy = getSignedVal(y);
  int64_t q, r;
  div_floor(sx, sy, q, r);
  return r;
}

uint64_t PublicRShiftA(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  int64_t sx = getSignedVal(x);
  int64_t ans = sx >> y;
  return getRingElt(ans);
}

uint64_t PublicRShiftL(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x >> y);
}

uint64_t PublicLShift(uint64_t x, uint64_t y) {
  assert((x < prime_mod) && (y < prime_mod));
  return (x << y) % prime_mod;
}

#endif

using namespace std;

uint32_t public_lrshift(uint32_t x, uint32_t y) { return (x >> y); }

int32_t public_lrshift(int32_t x, uint32_t y) {
  return ((int32_t)(((uint32_t)x) >> y));
}

uint64_t public_lrshift(uint64_t x, uint64_t y) { return (x >> y); }

int64_t public_lrshift(int64_t x, uint64_t y) {
  return ((int64_t)(((uint64_t)x) >> y));
}

template <typename T> vector<T> make_vector(size_t size) {
  return std::vector<T>(size);
}

template <typename T, typename... Args>
auto make_vector(size_t first, Args... sizes) {
  auto inner = make_vector<T>(sizes...);
  return vector<decltype(inner)>(first, inner);
}

template <typename T> ostream &operator<<(ostream &os, const vector<T> &v) {
  for (auto it = v.begin(); it != v.end(); ++it) {
    os << *it << endl;
  }
  return os;
}

void MatMul2DEigen_pt(int64_t i, int64_t j, int64_t k, uint64_2D &A,
                      uint64_2D &B, uint64_2D &C, int64_t consSF) {
  Eigen::Matrix<__int128, Eigen::Dynamic, Eigen::Dynamic> eigen_a(i, j);
  Eigen::Matrix<__int128, Eigen::Dynamic, Eigen::Dynamic> eigen_b(j, k);
  Eigen::Matrix<__int128, Eigen::Dynamic, Eigen::Dynamic> eigen_c(i, k);

  for (int i0 = 0; i0 < i; ++i0) {
    for (int i1 = 0; i1 < j; ++i1) {
      eigen_a(i0, i1) = A[i0][i1];
    }
  }

  for (int i0 = 0; i0 < j; ++i0) {
    for (int i1 = 0; i1 < k; ++i1) {
      eigen_b(i0, i1) = B[i0][i1];
    }
  }

  eigen_c = eigen_a * eigen_b; // No overflows since running in __int128

  for (int i0 = 0; i0 < i; ++i0) {
    for (int i1 = 0; i1 < k; ++i1) {
      if (bitlength == 64) {
        C[i0][i1] = eigen_c(i0, i1);
      } else {
        C[i0][i1] = (eigen_c(i0, i1)) % prime_mod;
      }
    }
  }
}

void MatMul2D_pt(uint64_t i, uint64_t j, uint64_t k, uint64_2D &A, uint64_2D &B,
                 uint64_2D &C, uint64_t consSF) {
  MatMul2DEigen_pt(i, j, k, A, B, C, consSF);
}

void ArgMax_pt(uint64_t s1, uint64_t s2, uint64_2D &inArr, uint64_1D &outArr) {
  for (uint64_t od = (int32_t)0; od < s1; od++) {

    uint64_t maxi = inArr[od][(int32_t)0];

    uint64_t maxiIdx = (int64_t)0;
    for (uint64_t i = (int32_t)0; i < s2; i++) {

      uint64_t iL = i;
      maxiIdx = (PublicGT(inArr[od][i], maxi)) ? iL : maxiIdx;
      maxi = (PublicGT(inArr[od][i], maxi)) ? inArr[od][i] : maxi;
    }
    outArr[od] = maxiIdx;
  }
}

void Relu_pt(uint64_t s1, uint64_1D &inArr, uint64_1D &outArr, uint64_t sf,
             uint64_t doTruncation) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    outArr[i1] = (PublicGT(inArr[i1], (int64_t)0)) ? inArr[i1] : (int64_t)0;
  }
  if (doTruncation) {
    for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
      outArr[i1] = (PublicRShiftA(outArr[i1], sf));
    }
  }
}

void Floor_pt(uint64_t s1, uint64_1D &inArr, uint64_1D &outArr, uint64_t sf) {

  uint64_t mask = ~(PublicSub((PublicLShift((int32_t)1, sf)), (int32_t)1));
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    outArr[i1] = (inArr[i1] & mask);
  }
}

void MaxPool_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t C, uint64_t ksizeH,
                uint64_t ksizeW, uint64_t zPadHLeft, uint64_t zPadHRight,
                uint64_t zPadWLeft, uint64_t zPadWRight, uint64_t strideH,
                uint64_t strideW, uint64_t N1, uint64_t imgH, uint64_t imgW,
                uint64_t C1,
                std::vector<std::vector<std::vector<uint64_1D>>> &inArr,
                std::vector<std::vector<std::vector<uint64_1D>>> &outArr) {
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t c = (int32_t)0; c < C; c++) {

      uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadHLeft));

      uint64_t extremeRightBottomCornerH =
          (PublicAdd((PublicSub(imgH, (int32_t)1)), zPadHRight));

      uint64_t ctH = (int32_t)0;
      while ((PublicLTE(
          (PublicSub((PublicAdd(leftTopCornerH, ksizeH)), (int32_t)1)),
          extremeRightBottomCornerH))) {

        uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadWLeft));

        uint64_t extremeRightBottomCornerW =
            (PublicAdd((PublicSub(imgW, (int32_t)1)), zPadWRight));

        uint64_t ctW = (int32_t)0;
        while ((PublicLTE(
            (PublicSub((PublicAdd(leftTopCornerW, ksizeW)), (int32_t)1)),
            extremeRightBottomCornerW))) {

          uint64_t maxi = (int64_t)0;
          if ((((PublicLT(leftTopCornerH, (int32_t)0)) ||
                (PublicGTE(leftTopCornerH, imgH))) ||
               ((PublicLT(leftTopCornerW, (int32_t)0)) ||
                (PublicGTE(leftTopCornerW, imgW))))) {
            maxi = (int64_t)0;
          } else {
            maxi = inArr[n][leftTopCornerH][leftTopCornerW][c];
          }
          for (uint64_t fh = (int32_t)0; fh < ksizeH; fh++) {
            for (uint64_t fw = (int32_t)0; fw < ksizeW; fw++) {

              uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

              uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

              uint64_t temp = (int64_t)0;
              if ((((PublicLT(curPosH, (int32_t)0)) ||
                    (PublicGTE(curPosH, imgH))) ||
                   ((PublicLT(curPosW, (int32_t)0)) ||
                    (PublicGTE(curPosW, imgW))))) {
                temp = (int64_t)0;
              } else {
                temp = inArr[n][curPosH][curPosW][c];
              }
              // maxi = (PublicLT(maxi, temp)) ? temp : maxi;
              maxi = (getSignedVal(PublicSub(maxi, temp)) < 0) ? temp : maxi;
            }
          }
          outArr[n][ctH][ctW][c] = maxi;
          leftTopCornerW = (PublicAdd(leftTopCornerW, strideW));
          ctW = (PublicAdd(ctW, (int32_t)1));
        }

        leftTopCornerH = (PublicAdd(leftTopCornerH, strideH));
        ctH = (PublicAdd(ctH, (int32_t)1));
      }
    }
  }
}

void AvgPool_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t C, uint64_t ksizeH,
                uint64_t ksizeW, uint64_t zPadHLeft, uint64_t zPadHRight,
                uint64_t zPadWLeft, uint64_t zPadWRight, uint64_t strideH,
                uint64_t strideW, uint64_t N1, uint64_t imgH, uint64_t imgW,
                uint64_t C1,
                std::vector<std::vector<std::vector<uint64_1D>>> &inArr,
                std::vector<std::vector<std::vector<uint64_1D>>> &outArr) {

  uint64_t rows = (PublicMult((PublicMult((PublicMult(N, C)), H)), W));

  auto filterAvg = make_vector<uint64_t>(rows);

  uint64_t rowIdx = (int32_t)0;
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t c = (int32_t)0; c < C; c++) {

      uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadHLeft));

      uint64_t extremeRightBottomCornerH =
          (PublicAdd((PublicSub(imgH, (int32_t)1)), zPadHRight));

      uint64_t ctH = (int32_t)0;
      while ((PublicLTE(
          (PublicSub((PublicAdd(leftTopCornerH, ksizeH)), (int32_t)1)),
          extremeRightBottomCornerH))) {

        uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadWLeft));

        uint64_t extremeRightBottomCornerW =
            (PublicAdd((PublicSub(imgW, (int32_t)1)), zPadWRight));

        uint64_t ctW = (int32_t)0;
        while ((PublicLTE(
            (PublicSub((PublicAdd(leftTopCornerW, ksizeW)), (int32_t)1)),
            extremeRightBottomCornerW))) {

          uint64_t curFilterSum = (int64_t)0;
          for (uint64_t fh = (int32_t)0; fh < ksizeH; fh++) {
            for (uint64_t fw = (int32_t)0; fw < ksizeW; fw++) {

              uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

              uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

              uint64_t temp = (int64_t)0;
              if ((((PublicLT(curPosH, (int32_t)0)) ||
                    (PublicGTE(curPosH, imgH))) ||
                   ((PublicLT(curPosW, (int32_t)0)) ||
                    (PublicGTE(curPosW, imgW))))) {
                temp = (int64_t)0;
              } else {
                temp = inArr[n][curPosH][curPosW][c];
              }
              curFilterSum = (PublicAdd(curFilterSum, temp));
            }
          }

          uint64_t ksizeH64 = ksizeH;

          uint64_t ksizeW64 = ksizeW;

          uint64_t filterSz64 = (PublicMult(ksizeH64, ksizeW64));

          uint64_t curFilterAvg = (PublicDiv(curFilterSum, filterSz64));
          filterAvg[rowIdx] = curFilterAvg;
          rowIdx = (PublicAdd(rowIdx, (int32_t)1));
          leftTopCornerW = (PublicAdd(leftTopCornerW, strideW));
          ctW = (PublicAdd(ctW, (int32_t)1));
        }

        leftTopCornerH = (PublicAdd(leftTopCornerH, strideH));
        ctH = (PublicAdd(ctH, (int32_t)1));
      }
    }
  }
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t c = (int32_t)0; c < C; c++) {
      for (uint64_t h = (int32_t)0; h < H; h++) {
        for (uint64_t w = (int32_t)0; w < W; w++) {
          outArr[n][h][w][c] = filterAvg[(PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(n, C)), H)), W)),
                      (PublicMult((PublicMult(c, H)), W)))),
                  (PublicMult(h, W)))),
              w))];
        }
      }
    }
  }
}

void ElemWiseSecretSharedVectorMult_pt(uint64_t s1, uint64_1D &arr1,
                                       uint64_1D &arr2, uint64_1D &outArr) {
  for (uint64_t ii = (int32_t)0; ii < s1; ii++) {
    outArr[ii] = (PublicMult(arr1[ii], arr2[ii]));
  }
}

void ElemWiseActModelVectorMult_pt(uint64_t s1, uint64_1D &arr1,
                                   uint64_1D &arr2, uint64_1D &outArr) {
  ElemWiseSecretSharedVectorMult_pt(s1, arr1, arr2, outArr);
}

void ElemWiseVectorPublicDiv_pt(uint64_t s1, uint64_1D &arr1, uint64_t divisor,
                                uint64_1D &outArr) {

  uint64_t divisor64 = divisor;
  for (uint64_t ii = (int32_t)0; ii < s1; ii++) {
    outArr[ii] = (PublicDiv(arr1[ii], divisor64));
  }
}

void ScaleUp_pt(uint64_t s1, uint64_1D &arr, uint64_t sf) {
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    arr[i] = (PublicLShift(arr[i], sf));
  }
}

void ScaleDown_pt(uint64_t s1, uint64_1D &arr, uint64_t sf) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    arr[i1] = (PublicRShiftA(arr[i1], sf));
  }
}

void ClearMemSecret1_pt(uint64_t s1, uint64_1D &arr) { return; }

void ClearMemSecret2_pt(uint64_t s1, uint64_t s2, uint64_2D &arr) { return; }

void ClearMemSecret3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_3D &arr) {
  return;
}

void ClearMemSecret4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                        uint64_4D &arr) {
  return;
}

void ClearMemSecret5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                        uint64_t s5, uint64_5D &arr) {
  return;
}

void ClearMemPublic_pt(uint64_t x) { return; }

void ClearMemPublic1_pt(uint64_t s, uint64_1D &x) { return; }

void ClearMemPublic2_pt(uint64_t s1, uint64_t s2, uint64_2D &arr) { return; }

void ClearMemPublic3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_3D &arr) {
  return;
}

void ClearMemPublic4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                        uint64_4D &arr) {
  return;
}

void ClearMemPublic5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                        uint64_t s5, uint64_5D &arr) {
  return;
}

void StartComputation_pt() { return; }

void EndComputation_pt() { return; }

void MatAddBroadCast2_pt(uint64_t s1, uint64_t s2, uint64_2D &A, uint64_1D &B,
                         uint64_2D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      outArr[i1][i2] = (PublicAdd(A[i1][i2], B[i2]));
    }
  }
}

void MatAdd2_pt(uint64_t s1, uint64_t s2, uint64_2D &A, uint64_2D &B,
                uint64_2D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      outArr[i1][i2] = (PublicAdd(A[i1][i2], B[i1][i2]));
    }
  }
}

void MatAddBroadCast4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                         uint64_4D &A, uint64_1D &B, uint64_4D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          outArr[i1][i2][i3][i4] = (PublicAdd(A[i1][i2][i3][i4], B[i4]));
        }
      }
    }
  }
}

void MatAdd4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                uint64_4D &A, uint64_4D &B, uint64_4D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          outArr[i1][i2][i3][i4] =
              (PublicAdd(A[i1][i2][i3][i4], B[i1][i2][i3][i4]));
        }
      }
    }
  }
}

void MatAddBroadCast5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                         uint64_t s5, uint64_5D &A, uint64_1D &B,
                         uint64_5D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {
            outArr[i1][i2][i3][i4][i5] =
                (PublicAdd(A[i1][i2][i3][i4][i5], B[i5]));
          }
        }
      }
    }
  }
}

void MatAdd5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4, uint64_t s5,
                uint64_5D &A, uint64_5D &B, uint64_5D &outArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {
            outArr[i1][i2][i3][i4][i5] =
                (PublicAdd(A[i1][i2][i3][i4][i5], B[i1][i2][i3][i4][i5]));
          }
        }
      }
    }
  }
}

void CreateTensor1_pt(uint64_t s1, uint64_t val, uint64_1D &arr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    arr[i1] = val;
  }
}

void CreateTensor2_pt(uint64_t s1, uint64_t s2, uint64_t val, uint64_2D &arr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      arr[i1][i2] = val;
    }
  }
}

void CreateTensor3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t val,
                      uint64_3D &arr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        arr[i1][i2][i3] = val;
      }
    }
  }
}

void CreateTensor4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                      uint64_t val, uint64_4D &arr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          arr[i1][i2][i3][i4] = val;
        }
      }
    }
  }
}

void CreateTensor5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                      uint64_t s5, uint64_t val, uint64_5D &arr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {
            arr[i1][i2][i3][i4][i5] = val;
          }
        }
      }
    }
  }
}

void CopyTensor1_pt(uint64_t s1, uint64_1D &targetArr, uint64_1D &fromArr,
                    uint64_1D &ignore) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    targetArr[i1] = fromArr[i1];
  }
}

void CopyTensor2_pt(uint64_t s1, uint64_t s2, uint64_2D &targetArr,
                    uint64_2D &fromArr, uint64_2D &ignore) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      targetArr[i1][i2] = fromArr[i1][i2];
    }
  }
}

void CopyTensor3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_3D &targetArr,
                    uint64_3D &fromArr, uint64_3D &ignore) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        targetArr[i1][i2][i3] = fromArr[i1][i2][i3];
      }
    }
  }
}

void CopyTensor4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                    uint64_4D &targetArr, uint64_4D &fromArr,
                    uint64_4D &ignore) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          targetArr[i1][i2][i3][i4] = fromArr[i1][i2][i3][i4];
        }
      }
    }
  }
}

void CreateIdentity11_pt(uint64_t s1, uint64_1D &fromArr, uint64_1D &newArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    newArr[i1] = fromArr[i1];
  }
}

void CreateIdentity22_pt(uint64_t s1, uint64_t s2, uint64_2D &fromArr,
                         uint64_2D &newArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      newArr[i1][i2] = fromArr[i1][i2];
    }
  }
}

void CreateIdentity33_pt(uint64_t s1, uint64_t s2, uint64_t s3,
                         uint64_3D &fromArr, uint64_3D &newArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        newArr[i1][i2][i3] = fromArr[i1][i2][i3];
      }
    }
  }
}

void CreateIdentity44_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                         uint64_4D &fromArr, uint64_4D &newArr) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          newArr[i1][i2][i3][i4] = fromArr[i1][i2][i3][i4];
        }
      }
    }
  }
}

void CreateCopy2211_pt(uint64_t s1, uint64_t s2, uint64_t inps1, uint64_t inps2,
                       uint64_2D &inArr, uint64_t perDimSize,
                       uint64_1D &beginIdx, uint64_1D &sizeIdx,
                       uint64_2D &outArr) {
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    for (uint64_t j = (int32_t)0; j < s2; j++) {
      outArr[i][j] = inArr[(PublicAdd(beginIdx[(int32_t)0], i))]
                          [(PublicAdd(beginIdx[(int32_t)1], j))];
    }
  }
}

void CreateCopy5511_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                       uint64_t s5, uint64_t inps1, uint64_t inps2,
                       uint64_t inps3, uint64_t inps4, uint64_t inps5,
                       uint64_5D &inArr, uint64_t perDimSize,
                       uint64_1D &beginIdx, uint64_1D &sizeIdx,
                       uint64_5D &outArr) {
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    for (uint64_t j = (int32_t)0; j < s2; j++) {
      for (uint64_t k = (int32_t)0; k < s3; k++) {
        for (uint64_t l = (int32_t)0; l < s4; l++) {
          for (uint64_t m = (int32_t)0; m < s5; m++) {
            outArr[i][j][k][l][m] = inArr[(PublicAdd(beginIdx[(int32_t)0], i))]
                                         [(PublicAdd(beginIdx[(int32_t)1], j))]
                                         [(PublicAdd(beginIdx[(int32_t)2], k))]
                                         [(PublicAdd(beginIdx[(int32_t)3], l))]
                                         [(PublicAdd(beginIdx[(int32_t)4], m))];
          }
        }
      }
    }
  }
}

void Concat2T222_pt(uint64_t s1, uint64_t s2, uint64_t inp1s1, uint64_t inp1s2,
                    uint64_2D &inp1, uint64_t inp2s1, uint64_t inp2s2,
                    uint64_2D &inp2, uint64_t axis, uint64_2D &outp) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      if (axis == (int32_t)0) {
        if ((PublicLT(i1, inp1s1))) {
          outp[i1][i2] = inp1[i1][i2];
        } else {
          outp[i1][i2] = inp2[(PublicSub(i1, inp1s1))][i2];
        }
      } else {
        if ((PublicLT(i2, inp1s2))) {
          outp[i1][i2] = inp1[i1][i2];
        } else {
          outp[i1][i2] = inp2[i1][(PublicSub(i2, inp1s2))];
        }
      }
    }
  }
}

void Concat2T444_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                    uint64_t inp1s1, uint64_t inp1s2, uint64_t inp1s3,
                    uint64_t inp1s4, uint64_4D &inp1, uint64_t inp2s1,
                    uint64_t inp2s2, uint64_t inp2s3, uint64_t inp2s4,
                    uint64_4D &inp2, uint64_t axis, uint64_4D &outp) {
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          if (axis == (int32_t)0) {
            if ((PublicLT(i1, inp1s1))) {
              outp[i1][i2][i3][i4] = inp1[i1][i2][i3][i4];
            } else {
              outp[i1][i2][i3][i4] = inp2[(PublicSub(i1, inp1s1))][i2][i3][i4];
            }
          } else {
            if (axis == (int32_t)1) {
              if ((PublicLT(i2, inp1s2))) {
                outp[i1][i2][i3][i4] = inp1[i1][i2][i3][i4];
              } else {
                outp[i1][i2][i3][i4] =
                    inp2[i1][(PublicSub(i2, inp1s2))][i3][i4];
              }
            } else {
              if (axis == (int32_t)2) {
                if ((PublicLT(i3, inp1s3))) {
                  outp[i1][i2][i3][i4] = inp1[i1][i2][i3][i4];
                } else {
                  outp[i1][i2][i3][i4] =
                      inp2[i1][i2][(PublicSub(i3, inp1s3))][i4];
                }
              } else {
                if ((PublicLT(i4, inp1s4))) {
                  outp[i1][i2][i3][i4] = inp1[i1][i2][i3][i4];
                } else {
                  outp[i1][i2][i3][i4] =
                      inp2[i1][i2][i3][(PublicSub(i4, inp1s4))];
                }
              }
            }
          }
        }
      }
    }
  }
}

void Split44_pt(uint64_t O1, uint64_t O2, uint64_t O3, uint64_t O4, uint64_t I1,
                uint64_t I2, uint64_t I3, uint64_t I4, uint64_4D &inp,
                uint64_t axis, uint64_t curCount, uint64_t total,
                uint64_4D &out) {
  for (uint64_t o1 = (int32_t)0; o1 < O1; o1++) {
    for (uint64_t o2 = (int32_t)0; o2 < O2; o2++) {
      for (uint64_t o3 = (int32_t)0; o3 < O3; o3++) {
        for (uint64_t o4 = (int32_t)0; o4 < O4; o4++) {

          uint64_t i1 = o1;

          uint64_t i2 = o2;

          uint64_t i3 = o3;

          uint64_t i4 = o4;
          if (axis == (int32_t)0) {
            i1 =
                (PublicAdd((PublicMult((PublicDiv(I1, total)), curCount)), o1));
          }
          if (axis == (int32_t)1) {
            i2 =
                (PublicAdd((PublicMult((PublicDiv(I2, total)), curCount)), o2));
          }
          if (axis == (int32_t)2) {
            i3 =
                (PublicAdd((PublicMult((PublicDiv(I3, total)), curCount)), o3));
          }
          if (axis == (int32_t)3) {
            i4 =
                (PublicAdd((PublicMult((PublicDiv(I4, total)), curCount)), o4));
          }
          out[o1][o2][o3][o4] = inp[i1][i2][i3][i4];
        }
      }
    }
  }
}

void Conv2DReshapeFilter_pt(uint64_t FH, uint64_t FW, uint64_t CI, uint64_t CO,
                            uint64_4D &inputArr, uint64_2D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
      for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
        for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

          uint64_t linIdx =
              (PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CI)),
                                    (PublicMult(fw, CI)))),
                         ci));
          outputArr[co][linIdx] = inputArr[fh][fw][ci][co];
        }
      }
    }
  }
}

void Conv2DReshapeMatMulOP_pt(uint64_t N, uint64_t finalH, uint64_t finalW,
                              uint64_t CO, uint64_2D &inputArr,
                              uint64_4D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t n = (int32_t)0; n < N; n++) {
      for (uint64_t h = (int32_t)0; h < finalH; h++) {
        for (uint64_t w = (int32_t)0; w < finalW; w++) {
          outputArr[n][h][w][co] = inputArr[co][(PublicAdd(
              (PublicAdd((PublicMult((PublicMult(n, finalH)), finalW)),
                         (PublicMult(h, finalW)))),
              w))];
        }
      }
    }
  }
}

void Conv2DReshapeInput_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                           uint64_t FH, uint64_t FW, uint64_t zPadHLeft,
                           uint64_t zPadHRight, uint64_t zPadWLeft,
                           uint64_t zPadWRight, uint64_t strideH,
                           uint64_t strideW, uint64_t RRows, uint64_t RCols,
                           uint64_4D &inputArr, uint64_2D &outputArr) {

  uint64_t linIdxFilterMult = (int32_t)0;
  for (uint64_t n = (int32_t)0; n < N; n++) {

    uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadHLeft));

    uint64_t extremeRightBottomCornerH =
        (PublicAdd((PublicSub(H, (int32_t)1)), zPadHRight));
    while ((PublicLTE((PublicSub((PublicAdd(leftTopCornerH, FH)), (int32_t)1)),
                      extremeRightBottomCornerH))) {

      uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadWLeft));

      uint64_t extremeRightBottomCornerW =
          (PublicAdd((PublicSub(W, (int32_t)1)), zPadWRight));
      while (
          (PublicLTE((PublicSub((PublicAdd(leftTopCornerW, FW)), (int32_t)1)),
                     extremeRightBottomCornerW))) {
        for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
          for (uint64_t fw = (int32_t)0; fw < FW; fw++) {

            uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

            uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

            uint64_t val = (int64_t)0;
            for (uint64_t ci = (int32_t)0; ci < CI; ci++) {
              if ((((PublicLT(curPosH, (int32_t)0)) ||
                    (PublicGTE(curPosH, H))) ||
                   ((PublicLT(curPosW, (int32_t)0)) ||
                    (PublicGTE(curPosW, W))))) {
                val = (int64_t)0;
              } else {
                val = inputArr[n][curPosH][curPosW][ci];
              }
              outputArr[(
                  PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CI)),
                                       (PublicMult(fw, CI)))),
                            ci))][linIdxFilterMult] = val;
            }
          }
        }
        linIdxFilterMult = (PublicAdd(linIdxFilterMult, (int32_t)1));
        leftTopCornerW = (PublicAdd(leftTopCornerW, strideW));
      }

      leftTopCornerH = (PublicAdd(leftTopCornerH, strideH));
    }
  }
}

void Conv2D_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI, uint64_t FH,
               uint64_t FW, uint64_t CO, uint64_t zPadHLeft,
               uint64_t zPadHRight, uint64_t zPadWLeft, uint64_t zPadWRight,
               uint64_t strideH, uint64_t strideW, uint64_4D &inputArr,
               uint64_4D &filterArr, uint64_4D &outArr) {

  uint64_t reshapedFilterRows = CO;

  uint64_t reshapedFilterCols = (PublicMult((PublicMult(FH, FW)), CI));

  uint64_t reshapedIPRows = (PublicMult((PublicMult(FH, FW)), CI));

  uint64_t newH = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(H, (PublicAdd(zPadHLeft, zPadHRight)))), FH)),
          strideH)),
      (int32_t)1));

  uint64_t newW = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(W, (PublicAdd(zPadWLeft, zPadWRight)))), FW)),
          strideW)),
      (int32_t)1));

  uint64_t reshapedIPCols = (PublicMult((PublicMult(N, newH)), newW));

  auto filterReshaped =
      make_vector<uint64_t>(reshapedFilterRows, reshapedFilterCols);

  auto inputReshaped = make_vector<uint64_t>(reshapedIPRows, reshapedIPCols);

  auto matmulOP = make_vector<uint64_t>(reshapedFilterRows, reshapedIPCols);
  Conv2DReshapeFilter_pt(FH, FW, CI, CO, filterArr, filterReshaped);
  Conv2DReshapeInput_pt(N, H, W, CI, FH, FW, zPadHLeft, zPadHRight, zPadWLeft,
                        zPadWRight, strideH, strideW, reshapedIPRows,
                        reshapedIPCols, inputArr, inputReshaped);
  MatMul2D_pt(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
              filterReshaped, inputReshaped, matmulOP, 1);
  Conv2DReshapeMatMulOP_pt(N, newH, newW, CO, matmulOP, outArr);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedFilterCols, filterReshaped);
  ClearMemSecret2_pt(reshapedIPRows, reshapedIPCols, inputReshaped);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedIPCols, matmulOP);
}

void Conv2DLoopInner_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                        uint64_t FH, uint64_t FW, uint64_t CO,
                        uint64_t zPadHLeft, uint64_t zPadHRight,
                        uint64_t zPadWLeft, uint64_t zPadWRight,
                        uint64_t strideH, uint64_t strideW, uint64_t outH,
                        uint64_t outW, uint64_t G, uint64_4D &inputArr,
                        uint64_4D &filterArr, uint64_4D &outArr) {

  uint64_t GIS = (PublicDiv(CI, G));

  uint64_t GOS = (PublicDiv(CO, G));
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t cog = (int32_t)0; cog < GOS; cog++) {
      for (uint64_t cig = (int32_t)0; cig < GIS; cig++) {
        for (uint64_t g = (int32_t)0; g < G; g++) {
          for (uint64_t h = (int32_t)0; h < outH; h++) {
            for (uint64_t w = (int32_t)0; w < outW; w++) {

              uint64_t val = (int64_t)0;

              uint64_t ci = (PublicAdd((PublicMult(GIS, g)), cig));

              uint64_t co = (PublicAdd((PublicMult(GOS, g)), cog));

              uint64_t curPosH =
                  (PublicSub((PublicMult(strideH, h)), zPadHLeft));
              for (uint64_t fh = (int32_t)0; fh < FH; fh++) {

                uint64_t curPosW =
                    (PublicSub((PublicMult(strideW, w)), zPadWLeft));
                for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
                  if (((((PublicGTE(curPosH, (int32_t)0)) &&
                         (PublicGTE(curPosW, (int32_t)0))) &&
                        (PublicLT(curPosH, H))) &&
                       (PublicLT(curPosW, W)))) {
                    val = (PublicAdd(
                        val, (PublicMult(
                                 inputArr[n][curPosH][curPosW][ci],
                                 filterArr[fh][fw][(PublicDiv(ci, G))][co]))));
                  }
                  curPosW = (PublicAdd(curPosW, (int32_t)1));
                }
                curPosH = (PublicAdd(curPosH, (int32_t)1));
              }
              outArr[n][h][w][co] = (PublicAdd(outArr[n][h][w][co], val));
            }
          }
        }
      }
    }
  }
}

void Conv2DLoop_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI, uint64_t FH,
                   uint64_t FW, uint64_t CO, uint64_t zPadHLeft,
                   uint64_t zPadHRight, uint64_t zPadWLeft, uint64_t zPadWRight,
                   uint64_t strideH, uint64_t strideW, uint64_t G,
                   uint64_4D &inputArr, uint64_4D &filterArr,
                   uint64_4D &outArr) {

  uint64_t outH =
      (PublicAdd((PublicDiv((PublicAdd((PublicSub(H, FH)),
                                       (PublicAdd(zPadHLeft, zPadHRight)))),
                            strideH)),
                 (int32_t)1));

  uint64_t outW =
      (PublicAdd((PublicDiv((PublicAdd((PublicSub(W, FW)),
                                       (PublicAdd(zPadWLeft, zPadWRight)))),
                            strideW)),
                 (int32_t)1));
  Conv2DLoopInner_pt(N, H, W, CI, FH, FW, CO, zPadHLeft, zPadHRight, zPadWLeft,
                     zPadWRight, strideH, strideW, outH, outW, G, inputArr,
                     filterArr, outArr);
}

void Conv2DReshapeFilterGroup_pt(uint64_t FH, uint64_t FW, uint64_t CI,
                                 uint64_t CO, uint64_t g, uint64_t G,
                                 uint64_4D &inputArr, uint64_2D &outputArr) {

  uint64_t CIG = (PublicDiv(CI, G));

  uint64_t COG = (PublicDiv(CO, G));

  uint64_t startCO = (PublicMult(g, COG));
  for (uint64_t co = (int32_t)0; co < COG; co++) {
    for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
      for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
        for (uint64_t ci = (int32_t)0; ci < CIG; ci++) {

          uint64_t linIdx =
              (PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CIG)),
                                    (PublicMult(fw, CIG)))),
                         ci));
          outputArr[co][linIdx] =
              inputArr[fh][fw][ci][(PublicAdd(co, startCO))];
        }
      }
    }
  }
}

void Conv2DReshapeMatMulOPGroup_pt(uint64_t N, uint64_t finalH, uint64_t finalW,
                                   uint64_t CO, uint64_t g, uint64_t G,
                                   uint64_2D &inputArr, uint64_4D &outputArr) {

  uint64_t COG = (PublicDiv(CO, G));

  uint64_t startCO = (PublicMult(g, COG));
  for (uint64_t co = (int32_t)0; co < COG; co++) {
    for (uint64_t n = (int32_t)0; n < N; n++) {
      for (uint64_t h = (int32_t)0; h < finalH; h++) {
        for (uint64_t w = (int32_t)0; w < finalW; w++) {
          outputArr[n][h][w][(PublicAdd(co, startCO))] =
              inputArr[co][(PublicAdd(
                  (PublicAdd((PublicMult((PublicMult(n, finalH)), finalW)),
                             (PublicMult(h, finalW)))),
                  w))];
        }
      }
    }
  }
}

void Conv2DReshapeInputGroup_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                                uint64_t FH, uint64_t FW, uint64_t zPadHLeft,
                                uint64_t zPadHRight, uint64_t zPadWLeft,
                                uint64_t zPadWRight, uint64_t strideH,
                                uint64_t strideW, uint64_t g, uint64_t G,
                                uint64_t RRows, uint64_t RCols,
                                uint64_4D &inputArr, uint64_2D &outputArr) {

  uint64_t linIdxFilterMult = (int32_t)0;

  uint64_t CIG = (PublicDiv(CI, G));
  for (uint64_t n = (int32_t)0; n < N; n++) {

    uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadHLeft));

    uint64_t extremeRightBottomCornerH =
        (PublicAdd((PublicSub(H, (int32_t)1)), zPadHRight));
    while ((PublicLTE((PublicSub((PublicAdd(leftTopCornerH, FH)), (int32_t)1)),
                      extremeRightBottomCornerH))) {

      uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadWLeft));

      uint64_t extremeRightBottomCornerW =
          (PublicAdd((PublicSub(W, (int32_t)1)), zPadWRight));
      while (
          (PublicLTE((PublicSub((PublicAdd(leftTopCornerW, FW)), (int32_t)1)),
                     extremeRightBottomCornerW))) {
        for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
          for (uint64_t fw = (int32_t)0; fw < FW; fw++) {

            uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

            uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

            uint64_t val = (int64_t)0;

            uint64_t startCI = (PublicMult(g, CIG));
            for (uint64_t ci = (int32_t)0; ci < CIG; ci++) {
              if ((((PublicLT(curPosH, (int32_t)0)) ||
                    (PublicGTE(curPosH, H))) ||
                   ((PublicLT(curPosW, (int32_t)0)) ||
                    (PublicGTE(curPosW, W))))) {
                val = (int64_t)0;
              } else {
                val = inputArr[n][curPosH][curPosW][(PublicAdd(ci, startCI))];
              }
              outputArr[(
                  PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CIG)),
                                       (PublicMult(fw, CIG)))),
                            ci))][linIdxFilterMult] = val;
            }
          }
        }
        linIdxFilterMult = (PublicAdd(linIdxFilterMult, (int32_t)1));
        leftTopCornerW = (PublicAdd(leftTopCornerW, strideW));
      }

      leftTopCornerH = (PublicAdd(leftTopCornerH, strideH));
    }
  }
}

void Conv2DGroup_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                    uint64_t FH, uint64_t FW, uint64_t CO, uint64_t zPadHLeft,
                    uint64_t zPadHRight, uint64_t zPadWLeft,
                    uint64_t zPadWRight, uint64_t strideH, uint64_t strideW,
                    uint64_t G, uint64_4D &inputArr, uint64_4D &filterArr,
                    uint64_4D &outArr) {

  uint64_t CIG = (PublicDiv(CI, G));

  uint64_t reshapedFilterRows = (PublicDiv(CO, G));

  uint64_t reshapedFilterCols = (PublicMult((PublicMult(FH, FW)), CIG));

  uint64_t reshapedIPRows = (PublicMult((PublicMult(FH, FW)), CIG));

  uint64_t outH = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(H, (PublicAdd(zPadHLeft, zPadHRight)))), FH)),
          strideH)),
      (int32_t)1));

  uint64_t outW = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(W, (PublicAdd(zPadWLeft, zPadWRight)))), FW)),
          strideW)),
      (int32_t)1));

  uint64_t reshapedIPCols = (PublicMult((PublicMult(N, outH)), outW));
  for (uint64_t g = (int32_t)0; g < G; g++) {

    auto inputReshaped = make_vector<uint64_t>(reshapedIPRows, reshapedIPCols);

    auto matmulOP = make_vector<uint64_t>(reshapedFilterRows, reshapedIPCols);

    auto filterReshaped =
        make_vector<uint64_t>(reshapedFilterRows, reshapedFilterCols);
    Conv2DReshapeFilterGroup_pt(FH, FW, CI, CO, g, G, filterArr,
                                filterReshaped);
    Conv2DReshapeInputGroup_pt(N, H, W, CI, FH, FW, zPadHLeft, zPadHRight,
                               zPadWLeft, zPadWRight, strideH, strideW, g, G,
                               reshapedIPRows, reshapedIPCols, inputArr,
                               inputReshaped);
    MatMul2D_pt(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
                filterReshaped, inputReshaped, matmulOP, 1);
    Conv2DReshapeMatMulOPGroup_pt(N, outH, outW, CO, g, G, matmulOP, outArr);
    ClearMemSecret2_pt(reshapedFilterRows, reshapedFilterCols, filterReshaped);
    ClearMemSecret2_pt(reshapedIPRows, reshapedIPCols, inputReshaped);
    ClearMemSecret2_pt(reshapedFilterRows, reshapedIPCols, matmulOP);
  }
}

void Conv3DReshapeFilter_pt(uint64_t FD, uint64_t FH, uint64_t FW, uint64_t CI,
                            uint64_t CO, uint64_5D &inputArr,
                            uint64_2D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t fd = (int32_t)0; fd < FD; fd++) {
      for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
        for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
          for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd((PublicMult(
                                   (PublicMult((PublicMult(fd, FH)), FW)), CI)),
                               (PublicMult((PublicMult(fh, FW)), CI)))),
                    (PublicMult(fw, CI)))),
                ci));
            outputArr[co][linIdx] = inputArr[fd][fh][fw][ci][co];
          }
        }
      }
    }
  }
}

void Conv3DReshapeMatMulOP_pt(uint64_t N, uint64_t finalD, uint64_t finalH,
                              uint64_t finalW, uint64_t CO, uint64_2D &inputArr,
                              uint64_5D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t n = (int32_t)0; n < N; n++) {
      for (uint64_t d = (int32_t)0; d < finalD; d++) {
        for (uint64_t h = (int32_t)0; h < finalH; h++) {
          for (uint64_t w = (int32_t)0; w < finalW; w++) {
            outputArr[n][d][h][w][co] = inputArr[co][(PublicAdd(
                (PublicAdd(
                    (PublicAdd((PublicMult((PublicMult((PublicMult(n, finalD)),
                                                       finalH)),
                                           finalW)),
                               (PublicMult((PublicMult(d, finalH)), finalW)))),
                    (PublicMult(h, finalW)))),
                w))];
          }
        }
      }
    }
  }
}

void Conv3DReshapeInput_pt(uint64_t N, uint64_t D, uint64_t H, uint64_t W,
                           uint64_t CI, uint64_t FD, uint64_t FH, uint64_t FW,
                           uint64_t zPadDLeft, uint64_t zPadDRight,
                           uint64_t zPadHLeft, uint64_t zPadHRight,
                           uint64_t zPadWLeft, uint64_t zPadWRight,
                           int64_t strideD, uint64_t strideH, uint64_t strideW,
                           uint64_t RRows, uint64_t RCols, uint64_5D &inputArr,
                           uint64_2D &outputArr) {

  uint64_t linIdxFilterMult = (int32_t)0;
  for (uint64_t n = (int32_t)0; n < N; n++) {

    uint64_t leftTopCornerD = (PublicSub((int32_t)0, zPadDLeft));

    uint64_t extremeRightBottomCornerD =
        (PublicAdd((PublicSub(D, (int32_t)1)), zPadDRight));
    while ((PublicLTE((PublicSub((PublicAdd(leftTopCornerD, FD)), (int32_t)1)),
                      extremeRightBottomCornerD))) {

      uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadHLeft));

      uint64_t extremeRightBottomCornerH =
          (PublicAdd((PublicSub(H, (int32_t)1)), zPadHRight));
      while (
          (PublicLTE((PublicSub((PublicAdd(leftTopCornerH, FH)), (int32_t)1)),
                     extremeRightBottomCornerH))) {

        uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadWLeft));

        uint64_t extremeRightBottomCornerW =
            (PublicAdd((PublicSub(W, (int32_t)1)), zPadWRight));
        while (
            (PublicLTE((PublicSub((PublicAdd(leftTopCornerW, FW)), (int32_t)1)),
                       extremeRightBottomCornerW))) {
          for (uint64_t fd = (int32_t)0; fd < FD; fd++) {
            for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
              for (uint64_t fw = (int32_t)0; fw < FW; fw++) {

                uint64_t curPosD = (PublicAdd(leftTopCornerD, fd));

                uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

                uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

                uint64_t val = (int64_t)0;
                for (uint64_t ci = (int32_t)0; ci < CI; ci++) {
                  if (((((PublicLT(curPosD, (int32_t)0)) ||
                         (PublicGTE(curPosD, D))) ||
                        ((PublicLT(curPosH, (int32_t)0)) ||
                         (PublicGTE(curPosH, H)))) ||
                       ((PublicLT(curPosW, (int32_t)0)) ||
                        (PublicGTE(curPosW, W))))) {
                    val = (int64_t)0;
                  } else {
                    val = inputArr[n][curPosD][curPosH][curPosW][ci];
                  }
                  outputArr[(PublicAdd(
                      (PublicAdd(
                          (PublicAdd(
                              (PublicMult(
                                  (PublicMult((PublicMult(fd, FH)), FW)), CI)),
                              (PublicMult((PublicMult(fh, FW)), CI)))),
                          (PublicMult(fw, CI)))),
                      ci))][linIdxFilterMult] = val;
                }
              }
            }
          }
          linIdxFilterMult = (PublicAdd(linIdxFilterMult, (int32_t)1));
          leftTopCornerW = (PublicAdd(leftTopCornerW, strideW));
        }

        leftTopCornerH = (PublicAdd(leftTopCornerH, strideH));
      }

      leftTopCornerD = (PublicAdd(leftTopCornerD, strideD));
    }
  }
}

void Conv3D_pt(uint64_t N, uint64_t D, uint64_t H, uint64_t W, uint64_t CI,
               uint64_t FD, uint64_t FH, uint64_t FW, uint64_t CO,
               uint64_t zPadDLeft, uint64_t zPadDRight, uint64_t zPadHLeft,
               uint64_t zPadHRight, uint64_t zPadWLeft, uint64_t zPadWRight,
               uint64_t strideD, uint64_t strideH, uint64_t strideW,
               uint64_5D &inputArr, uint64_5D &filterArr, uint64_5D &outArr) {

  uint64_t reshapedFilterRows = CO;

  uint64_t reshapedFilterCols =
      (PublicMult((PublicMult((PublicMult(FD, FH)), FW)), CI));

  uint64_t reshapedIPRows =
      (PublicMult((PublicMult((PublicMult(FD, FH)), FW)), CI));

  uint64_t newD = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(D, (PublicAdd(zPadDLeft, zPadDRight)))), FD)),
          strideD)),
      (int32_t)1));

  uint64_t newH = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(H, (PublicAdd(zPadHLeft, zPadHRight)))), FH)),
          strideH)),
      (int32_t)1));

  uint64_t newW = (PublicAdd(
      (PublicDiv(
          (PublicSub((PublicAdd(W, (PublicAdd(zPadWLeft, zPadWRight)))), FW)),
          strideW)),
      (int32_t)1));

  uint64_t reshapedIPCols =
      (PublicMult((PublicMult((PublicMult(N, newD)), newH)), newW));

  auto filterReshaped =
      make_vector<uint64_t>(reshapedFilterRows, reshapedFilterCols);

  auto inputReshaped = make_vector<uint64_t>(reshapedIPRows, reshapedIPCols);

  auto matmulOP = make_vector<uint64_t>(reshapedFilterRows, reshapedIPCols);
  Conv3DReshapeFilter_pt(FD, FH, FW, CI, CO, filterArr, filterReshaped);
  Conv3DReshapeInput_pt(N, D, H, W, CI, FD, FH, FW, zPadDLeft, zPadDRight,
                        zPadHLeft, zPadHRight, zPadWLeft, zPadWRight, strideD,
                        strideH, strideW, reshapedIPRows, reshapedIPCols,
                        inputArr, inputReshaped);
  MatMul2D_pt(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
              filterReshaped, inputReshaped, matmulOP, 1);
  Conv3DReshapeMatMulOP_pt(N, newD, newH, newW, CO, matmulOP, outArr);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedFilterCols, filterReshaped);
  ClearMemSecret2_pt(reshapedIPRows, reshapedIPCols, inputReshaped);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedIPCols, matmulOP);
}

void Conv3DLoopInner_pt(uint64_t N, uint64_t D, uint64_t H, uint64_t W,
                        uint64_t CI, uint64_t FD, uint64_t FH, uint64_t FW,
                        uint64_t CO, uint64_t zPadDLeft, uint64_t zPadDRight,
                        uint64_t zPadHLeft, uint64_t zPadHRight,
                        uint64_t zPadWLeft, uint64_t zPadWRight,
                        uint64_t strideD, uint64_t strideH, uint64_t strideW,
                        uint64_t outD, uint64_t outH, uint64_t outW,
                        uint64_5D &inputArr, uint64_5D &filterArr,
                        uint64_5D &outArr) {
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t co = (int32_t)0; co < CO; co++) {
      for (uint64_t d = (int32_t)0; d < outD; d++) {
        for (uint64_t h = (int32_t)0; h < outH; h++) {
          for (uint64_t w = (int32_t)0; w < outW; w++) {
            for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

              uint64_t val = (int64_t)0;
              for (uint64_t fd = (PublicMult(d, strideD));
                   fd < (PublicAdd((PublicMult(d, strideD)), FD)); fd++) {
                for (uint64_t fh = (PublicMult(h, strideH));
                     fh < (PublicAdd((PublicMult(h, strideH)), FH)); fh++) {
                  for (uint64_t fw = (PublicMult(w, strideW));
                       fw < (PublicAdd((PublicMult(w, strideW)), FW)); fw++) {

                    uint64_t curPosD = (PublicSub(fd, zPadDLeft));

                    uint64_t curPosH = (PublicSub(fh, zPadHLeft));

                    uint64_t curPosW = (PublicSub(fw, zPadWLeft));
                    if (((((((PublicGTE(curPosD, (int32_t)0)) &&
                             (PublicGTE(curPosH, (int32_t)0))) &&
                            (PublicGTE(curPosW, (int32_t)0))) &&
                           (PublicLT(curPosD, D))) &&
                          (PublicLT(curPosH, H))) &&
                         (PublicLT(curPosW, W)))) {

                      uint64_t curFilterPosD =
                          (PublicSub(fd, (PublicMult(d, strideD))));

                      uint64_t curFilterPosH =
                          (PublicSub(fh, (PublicMult(h, strideH))));

                      uint64_t curFilterPosW =
                          (PublicSub(fw, (PublicMult(w, strideW))));
                      val = (PublicAdd(
                          val, (PublicMult(
                                   inputArr[n][curPosD][curPosH][curPosW][ci],
                                   filterArr[curFilterPosD][curFilterPosH]
                                            [curFilterPosW][ci][co]))));
                    }
                  }
                }
              }
              outArr[n][d][h][w][co] = (PublicAdd(outArr[n][d][h][w][co], val));
            }
          }
        }
      }
    }
  }
}

void Conv3DLoop_pt(uint64_t N, uint64_t D, uint64_t H, uint64_t W, uint64_t CI,
                   uint64_t FD, uint64_t FH, uint64_t FW, uint64_t CO,
                   uint64_t zPadDLeft, uint64_t zPadDRight, uint64_t zPadHLeft,
                   uint64_t zPadHRight, uint64_t zPadWLeft, uint64_t zPadWRight,
                   uint64_t strideD, uint64_t strideH, uint64_t strideW,
                   uint64_5D &inputArr, uint64_5D &filterArr,
                   uint64_5D &outArr) {

  uint64_t outD =
      (PublicAdd((PublicDiv((PublicAdd((PublicSub(D, FD)),
                                       (PublicAdd(zPadDLeft, zPadDRight)))),
                            strideD)),
                 (int32_t)1));

  uint64_t outH =
      (PublicAdd((PublicDiv((PublicAdd((PublicSub(H, FH)),
                                       (PublicAdd(zPadHLeft, zPadHRight)))),
                            strideH)),
                 (int32_t)1));

  uint64_t outW =
      (PublicAdd((PublicDiv((PublicAdd((PublicSub(W, FW)),
                                       (PublicAdd(zPadWLeft, zPadWRight)))),
                            strideW)),
                 (int32_t)1));
  Conv3DLoopInner_pt(N, D, H, W, CI, FD, FH, FW, CO, zPadDLeft, zPadDRight,
                     zPadHLeft, zPadHRight, zPadWLeft, zPadWRight, strideD,
                     strideH, strideW, outD, outH, outW, inputArr, filterArr,
                     outArr);
}

void ConvTranspose2DReshapeMatMulOP_pt(uint64_t N, uint64_t finalH,
                                       uint64_t finalW, uint64_t CO,
                                       uint64_2D &inputArr,
                                       uint64_4D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t n = (int32_t)0; n < N; n++) {
      for (uint64_t h = (int32_t)0; h < finalH; h++) {
        for (uint64_t w = (int32_t)0; w < finalW; w++) {
          outputArr[n][h][w][co] = inputArr[co][(PublicAdd(
              (PublicAdd((PublicMult((PublicMult(n, finalH)), finalW)),
                         (PublicMult(h, finalW)))),
              w))];
        }
      }
    }
  }
}

void ConvTranspose2DReshapeFilter_pt(uint64_t FH, uint64_t FW, uint64_t CO,
                                     uint64_t CI, uint64_4D &inputArr,
                                     uint64_2D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
      for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
        for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

          uint64_t linIdx =
              (PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CI)),
                                    (PublicMult(fw, CI)))),
                         ci));
          outputArr[co][linIdx] =
              inputArr[(PublicSub((PublicSub(FH, (int32_t)1)), fh))]
                      [(PublicSub((PublicSub(FW, (int32_t)1)), fw))][co][ci];
        }
      }
    }
  }
}

void ConvTranspose2DReshapeInput_pt(uint64_t N, uint64_t HPrime,
                                    uint64_t WPrime, uint64_t CI, uint64_t FH,
                                    uint64_t FW, uint64_t zPadTrHLeft,
                                    uint64_t zPadTrHRight, uint64_t zPadTrWLeft,
                                    uint64_t zPadTrWRight, uint64_t strideH,
                                    uint64_t strideW, uint64_t RRows,
                                    uint64_t RCols, uint64_4D &inputArr,
                                    uint64_2D &outputArr) {

  uint64_t linIdxFilterMult = (int32_t)0;
  for (uint64_t n = (int32_t)0; n < N; n++) {

    uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadTrHLeft));

    uint64_t HPrimeTilde =
        (PublicAdd(HPrime, (PublicMult((PublicSub(HPrime, (int32_t)1)),
                                       (PublicSub(strideH, (int32_t)1))))));

    uint64_t extremeRightBottomCornerH =
        (PublicAdd((PublicSub(HPrimeTilde, (int32_t)1)), zPadTrHRight));
    while ((PublicLTE((PublicSub((PublicAdd(leftTopCornerH, FH)), (int32_t)1)),
                      extremeRightBottomCornerH))) {

      uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadTrWLeft));

      uint64_t WPrimeTilde =
          (PublicAdd(WPrime, (PublicMult((PublicSub(WPrime, (int32_t)1)),
                                         (PublicSub(strideW, (int32_t)1))))));

      uint64_t extremeRightBottomCornerW =
          (PublicAdd((PublicSub(WPrimeTilde, (int32_t)1)), zPadTrWRight));
      while (
          (PublicLTE((PublicSub((PublicAdd(leftTopCornerW, FW)), (int32_t)1)),
                     extremeRightBottomCornerW))) {
        for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
          for (uint64_t fw = (int32_t)0; fw < FW; fw++) {

            uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

            uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

            uint64_t val = (int64_t)0;
            for (uint64_t ci = (int32_t)0; ci < CI; ci++) {
              if ((((PublicLT(curPosH, (int32_t)0)) ||
                    (PublicGTE(curPosH, HPrimeTilde))) ||
                   ((PublicLT(curPosW, (int32_t)0)) ||
                    (PublicGTE(curPosW, WPrimeTilde))))) {
                val = (int64_t)0;
              } else {
                if (((PublicMod(curPosH, strideH)) == (int32_t)0) &&
                    ((PublicMod(curPosW, strideW)) == (int32_t)0)) {

                  uint64_t idxInputH = (PublicDiv(curPosH, strideH));

                  uint64_t idxInputW = (PublicDiv(curPosW, strideW));
                  val = inputArr[n][idxInputH][idxInputW][ci];
                } else {
                  val = (int64_t)0;
                }
              }
              outputArr[(
                  PublicAdd((PublicAdd((PublicMult((PublicMult(fh, FW)), CI)),
                                       (PublicMult(fw, CI)))),
                            ci))][linIdxFilterMult] = val;
            }
          }
        }
        linIdxFilterMult = (PublicAdd(linIdxFilterMult, (int32_t)1));
        leftTopCornerW = (PublicAdd(leftTopCornerW, (int32_t)1));
      }

      leftTopCornerH = (PublicAdd(leftTopCornerH, (int32_t)1));
    }
  }
}

void ConvTranspose2D_pt(uint64_t N, uint64_t HPrime, uint64_t WPrime,
                        uint64_t CI, uint64_t FH, uint64_t FW, uint64_t CO,
                        uint64_t H, uint64_t W, uint64_t zPadTrHLeft,
                        uint64_t zPadTrHRight, uint64_t zPadTrWLeft,
                        uint64_t zPadTrWRight, uint64_t strideH,
                        uint64_t strideW, uint64_4D &inputArr,
                        uint64_4D &filterArr, uint64_4D &outArr) {

  uint64_t reshapedFilterRows = CO;

  uint64_t reshapedFilterCols = (PublicMult((PublicMult(FH, FW)), CI));

  uint64_t reshapedIPRows = (PublicMult((PublicMult(FH, FW)), CI));

  uint64_t reshapedIPCols = (PublicMult((PublicMult(N, H)), W));

  auto filterReshaped =
      make_vector<uint64_t>(reshapedFilterRows, reshapedFilterCols);

  auto inputReshaped = make_vector<uint64_t>(reshapedIPRows, reshapedIPCols);

  auto matmulOP = make_vector<uint64_t>(reshapedFilterRows, reshapedIPCols);
  ConvTranspose2DReshapeFilter_pt(FH, FW, CO, CI, filterArr, filterReshaped);
  ConvTranspose2DReshapeInput_pt(N, HPrime, WPrime, CI, FH, FW, zPadTrHLeft,
                                 zPadTrHRight, zPadTrWLeft, zPadTrWRight,
                                 strideH, strideW, reshapedIPRows,
                                 reshapedIPCols, inputArr, inputReshaped);
  MatMul2D_pt(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
              filterReshaped, inputReshaped, matmulOP, 1);
  ConvTranspose2DReshapeMatMulOP_pt(N, H, W, CO, matmulOP, outArr);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedFilterCols, filterReshaped);
  ClearMemSecret2_pt(reshapedIPRows, reshapedIPCols, inputReshaped);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedIPCols, matmulOP);
}

void ConvTranspose3DReshapeFilter_pt(uint64_t FD, uint64_t FH, uint64_t FW,
                                     uint64_t CO, uint64_t CI,
                                     uint64_5D &inputArr,
                                     uint64_2D &outputArr) {
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t fd = (int32_t)0; fd < FD; fd++) {
      for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
        for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
          for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd((PublicMult(
                                   (PublicMult((PublicMult(fd, FH)), FW)), CI)),
                               (PublicMult((PublicMult(fh, FW)), CI)))),
                    (PublicMult(fw, CI)))),
                ci));
            outputArr[co][linIdx] =
                inputArr[(PublicSub((PublicSub(FD, (int32_t)1)), fd))]
                        [(PublicSub((PublicSub(FH, (int32_t)1)), fh))]
                        [(PublicSub((PublicSub(FW, (int32_t)1)), fw))][co][ci];
          }
        }
      }
    }
  }
}

void ConvTranspose3DReshapeInput_pt(
    uint64_t N, uint64_t DPrime, uint64_t HPrime, uint64_t WPrime, uint64_t CI,
    uint64_t FD, uint64_t FH, uint64_t FW, uint64_t zPadTrDLeft,
    uint64_t zPadTrDRight, uint64_t zPadTrHLeft, uint64_t zPadTrHRight,
    uint64_t zPadTrWLeft, uint64_t zPadTrWRight, uint64_t strideD,
    uint64_t strideH, uint64_t strideW, uint64_t RRows, uint64_t RCols,
    uint64_5D &inputArr, uint64_2D &outputArr) {

  uint64_t linIdxFilterMult = (int32_t)0;
  for (uint64_t n = (int32_t)0; n < N; n++) {

    uint64_t leftTopCornerD = (PublicSub((int32_t)0, zPadTrDLeft));

    uint64_t DPrimeTilde =
        (PublicAdd(DPrime, (PublicMult((PublicSub(DPrime, (int32_t)1)),
                                       (PublicSub(strideD, (int32_t)1))))));

    uint64_t extremeRightBottomCornerD =
        (PublicAdd((PublicSub(DPrimeTilde, (int32_t)1)), zPadTrDRight));
    while ((PublicLTE((PublicSub((PublicAdd(leftTopCornerD, FD)), (int32_t)1)),
                      extremeRightBottomCornerD))) {

      uint64_t leftTopCornerH = (PublicSub((int32_t)0, zPadTrHLeft));

      uint64_t HPrimeTilde =
          (PublicAdd(HPrime, (PublicMult((PublicSub(HPrime, (int32_t)1)),
                                         (PublicSub(strideH, (int32_t)1))))));

      uint64_t extremeRightBottomCornerH =
          (PublicAdd((PublicSub(HPrimeTilde, (int32_t)1)), zPadTrHRight));
      while (
          (PublicLTE((PublicSub((PublicAdd(leftTopCornerH, FH)), (int32_t)1)),
                     extremeRightBottomCornerH))) {

        uint64_t leftTopCornerW = (PublicSub((int32_t)0, zPadTrWLeft));

        uint64_t WPrimeTilde =
            (PublicAdd(WPrime, (PublicMult((PublicSub(WPrime, (int32_t)1)),
                                           (PublicSub(strideW, (int32_t)1))))));

        uint64_t extremeRightBottomCornerW =
            (PublicAdd((PublicSub(WPrimeTilde, (int32_t)1)), zPadTrWRight));
        while (
            (PublicLTE((PublicSub((PublicAdd(leftTopCornerW, FW)), (int32_t)1)),
                       extremeRightBottomCornerW))) {
          for (uint64_t fd = (int32_t)0; fd < FD; fd++) {
            for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
              for (uint64_t fw = (int32_t)0; fw < FW; fw++) {

                uint64_t curPosD = (PublicAdd(leftTopCornerD, fd));

                uint64_t curPosH = (PublicAdd(leftTopCornerH, fh));

                uint64_t curPosW = (PublicAdd(leftTopCornerW, fw));

                uint64_t val = (int64_t)0;
                for (uint64_t ci = (int32_t)0; ci < CI; ci++) {
                  if (((((PublicLT(curPosD, (int32_t)0)) ||
                         (PublicGTE(curPosD, DPrimeTilde))) ||
                        ((PublicLT(curPosH, (int32_t)0)) ||
                         (PublicGTE(curPosH, HPrimeTilde)))) ||
                       ((PublicLT(curPosW, (int32_t)0)) ||
                        (PublicGTE(curPosW, WPrimeTilde))))) {
                    val = (int64_t)0;
                  } else {
                    if ((((PublicMod(curPosD, strideD)) == (int32_t)0) &&
                         ((PublicMod(curPosH, strideH)) == (int32_t)0)) &&
                        ((PublicMod(curPosW, strideW)) == (int32_t)0)) {

                      uint64_t idxInputD = (PublicDiv(curPosD, strideD));

                      uint64_t idxInputH = (PublicDiv(curPosH, strideH));

                      uint64_t idxInputW = (PublicDiv(curPosW, strideW));
                      val = inputArr[n][idxInputD][idxInputH][idxInputW][ci];
                    } else {
                      val = (int64_t)0;
                    }
                  }
                  outputArr[(PublicAdd(
                      (PublicAdd(
                          (PublicAdd(
                              (PublicMult(
                                  (PublicMult((PublicMult(fd, FH)), FW)), CI)),
                              (PublicMult((PublicMult(fh, FW)), CI)))),
                          (PublicMult(fw, CI)))),
                      ci))][linIdxFilterMult] = val;
                }
              }
            }
          }
          linIdxFilterMult = (PublicAdd(linIdxFilterMult, (int32_t)1));
          leftTopCornerW = (PublicAdd(leftTopCornerW, (int32_t)1));
        }

        leftTopCornerH = (PublicAdd(leftTopCornerH, (int32_t)1));
      }

      leftTopCornerD = (PublicAdd(leftTopCornerD, (int32_t)1));
    }
  }
}

void ConvTranspose3D_pt(uint64_t N, uint64_t DPrime, uint64_t HPrime,
                        uint64_t WPrime, uint64_t CI, uint64_t FD, uint64_t FH,
                        uint64_t FW, uint64_t CO, uint64_t D, uint64_t H,
                        uint64_t W, uint64_t zPadTrDLeft, uint64_t zPadTrDRight,
                        uint64_t zPadTrHLeft, uint64_t zPadTrHRight,
                        uint64_t zPadTrWLeft, uint64_t zPadTrWRight,
                        uint64_t strideD, uint64_t strideH, uint64_t strideW,
                        uint64_5D &inputArr, uint64_5D &filterArr,
                        uint64_5D &outArr) {

  uint64_t reshapedFilterRows = CO;

  uint64_t reshapedFilterCols =
      (PublicMult((PublicMult((PublicMult(FD, FH)), FW)), CI));

  uint64_t reshapedIPRows =
      (PublicMult((PublicMult((PublicMult(FD, FH)), FW)), CI));

  uint64_t reshapedIPCols =
      (PublicMult((PublicMult((PublicMult(N, D)), H)), W));

  auto filterReshaped =
      make_vector<uint64_t>(reshapedFilterRows, reshapedFilterCols);

  auto inputReshaped = make_vector<uint64_t>(reshapedIPRows, reshapedIPCols);

  auto matmulOP = make_vector<uint64_t>(reshapedFilterRows, reshapedIPCols);
  ConvTranspose3DReshapeFilter_pt(FD, FH, FW, CO, CI, filterArr,
                                  filterReshaped);
  ConvTranspose3DReshapeInput_pt(
      N, DPrime, HPrime, WPrime, CI, FD, FH, FW, zPadTrDLeft, zPadTrDRight,
      zPadTrHLeft, zPadTrHRight, zPadTrWLeft, zPadTrWRight, strideD, strideH,
      strideW, reshapedIPRows, reshapedIPCols, inputArr, inputReshaped);
  MatMul2D_pt(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
              filterReshaped, inputReshaped, matmulOP, 1);
  Conv3DReshapeMatMulOP_pt(N, D, H, W, CO, matmulOP, outArr);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedFilterCols, filterReshaped);
  ClearMemSecret2_pt(reshapedIPRows, reshapedIPCols, inputReshaped);
  ClearMemSecret2_pt(reshapedFilterRows, reshapedIPCols, matmulOP);
}

void ConvTranspose3DLoopInner_pt(
    uint64_t N, uint64_t D, uint64_t H, uint64_t W, uint64_t CI, uint64_t FD,
    uint64_t FH, uint64_t FW, uint64_t CO, uint64_t zPadDLeft,
    uint64_t zPadDRight, uint64_t zPadHLeft, uint64_t zPadHRight,
    uint64_t zPadWLeft, uint64_t zPadWRight, uint64_t strideD, uint64_t strideH,
    uint64_t strideW, uint64_t outD, uint64_t outH, uint64_t outW,
    uint64_5D &inputArr, uint64_5D &filterArr, uint64_5D &outArr) {
  for (uint64_t n = (int32_t)0; n < N; n++) {
    for (uint64_t co = (int32_t)0; co < CO; co++) {
      for (uint64_t d = (int32_t)0; d < outD; d++) {
        for (uint64_t h = (int32_t)0; h < outH; h++) {
          for (uint64_t w = (int32_t)0; w < outW; w++) {
            for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

              uint64_t val = (int64_t)0;
              for (uint64_t fd = d; fd < (PublicAdd(d, FD)); fd++) {
                for (uint64_t fh = h; fh < (PublicAdd(h, FH)); fh++) {
                  for (uint64_t fw = w; fw < (PublicAdd(w, FW)); fw++) {

                    uint64_t curPosD =
                        (PublicDiv((PublicSub(fd, zPadDLeft)), strideD));

                    uint64_t curPosH =
                        (PublicDiv((PublicSub(fh, zPadHLeft)), strideD));

                    uint64_t curPosW =
                        (PublicDiv((PublicSub(fw, zPadWLeft)), strideD));
                    if (((((((((PublicGTE(curPosD, (int32_t)0)) &&
                               (PublicGTE(curPosH, (int32_t)0))) &&
                              (PublicGTE(curPosW, (int32_t)0))) &&
                             (PublicLT(curPosD, D))) &&
                            (PublicLT(curPosH, H))) &&
                           (PublicLT(curPosW, W))) &&
                          ((PublicMod((PublicSub(fd, zPadDLeft)), strideD)) ==
                           (int32_t)0)) &&
                         ((PublicMod((PublicSub(fh, zPadHLeft)), strideH)) ==
                          (int32_t)0)) &&
                        ((PublicMod((PublicSub(fw, zPadWLeft)), strideW)) ==
                         (int32_t)0)) {

                      uint64_t curFilterPosD = (PublicSub(
                          (PublicSub((PublicAdd(FD, d)), fd)), (int32_t)1));

                      uint64_t curFilterPosH = (PublicSub(
                          (PublicSub((PublicAdd(FH, h)), fh)), (int32_t)1));

                      uint64_t curFilterPosW = (PublicSub(
                          (PublicSub((PublicAdd(FW, w)), fw)), (int32_t)1));
                      val = (PublicAdd(
                          val, (PublicMult(
                                   inputArr[n][curPosD][curPosH][curPosW][ci],
                                   filterArr[curFilterPosD][curFilterPosH]
                                            [curFilterPosW][co][ci]))));
                    }
                  }
                }
              }
              outArr[n][d][h][w][co] = (PublicAdd(outArr[n][d][h][w][co], val));
            }
          }
        }
      }
    }
  }
}

void ConvTranspose3DLoop_pt(
    uint64_t N, uint64_t DPrime, uint64_t HPrime, uint64_t WPrime, uint64_t CI,
    uint64_t FD, uint64_t FH, uint64_t FW, uint64_t CO, uint64_t D, uint64_t H,
    uint64_t W, uint64_t zPadTrDLeft, uint64_t zPadTrDRight,
    uint64_t zPadTrHLeft, uint64_t zPadTrHRight, uint64_t zPadTrWLeft,
    uint64_t zPadTrWRight, uint64_t strideD, uint64_t strideH, uint64_t strideW,
    uint64_5D &inputArr, uint64_5D &filterArr, uint64_5D &outArr) {
  ConvTranspose3DLoopInner_pt(
      N, DPrime, HPrime, WPrime, CI, FD, FH, FW, CO, zPadTrDLeft, zPadTrDRight,
      zPadTrHLeft, zPadTrHRight, zPadTrWLeft, zPadTrWRight, strideD, strideH,
      strideW, D, H, W, inputArr, filterArr, outArr);
}

void Transpose2_pt(uint64_t s1, uint64_t s2, uint64_2D &inArr,
                   uint64_2D &outArr) {
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    for (uint64_t j = (int32_t)0; j < s2; j++) {
      outArr[i][j] = inArr[j][i];
    }
  }
}

void Pad442_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
               uint64_t inps1, uint64_t inps2, uint64_t inps3, uint64_t inps4,
               uint64_4D &inpArr, uint64_t pads1, uint64_t pads2,
               uint64_2D &paddings, uint64_4D &outArr) {

  uint64_t lbounds1 = paddings[(int32_t)0][(int32_t)0];

  uint64_t rbounds1excl = (PublicSub(s1, paddings[(int32_t)0][(int32_t)1]));

  uint64_t lbounds2 = paddings[(int32_t)1][(int32_t)0];

  uint64_t rbounds2excl = (PublicSub(s2, paddings[(int32_t)1][(int32_t)1]));

  uint64_t lbounds3 = paddings[(int32_t)2][(int32_t)0];

  uint64_t rbounds3excl = (PublicSub(s3, paddings[(int32_t)2][(int32_t)1]));

  uint64_t lbounds4 = paddings[(int32_t)3][(int32_t)0];

  uint64_t rbounds4excl = (PublicSub(s4, paddings[(int32_t)3][(int32_t)1]));
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    for (uint64_t j = (int32_t)0; j < s2; j++) {
      for (uint64_t k = (int32_t)0; k < s3; k++) {
        for (uint64_t l = (int32_t)0; l < s4; l++) {
          if (((((((((PublicGTE(i, lbounds1)) && (PublicLT(i, rbounds1excl))) &&
                    (PublicGTE(j, lbounds2))) &&
                   (PublicLT(j, rbounds2excl))) &&
                  (PublicGTE(k, lbounds3))) &&
                 (PublicLT(k, rbounds3excl))) &&
                (PublicGTE(l, lbounds4))) &&
               (PublicLT(l, rbounds4excl)))) {
            outArr[i][j][k][l] =
                inpArr[(PublicSub(i, paddings[(int32_t)0][(int32_t)0]))]
                      [(PublicSub(j, paddings[(int32_t)1][(int32_t)0]))]
                      [(PublicSub(k, paddings[(int32_t)2][(int32_t)0]))]
                      [(PublicSub(l, paddings[(int32_t)3][(int32_t)0]))];
          } else {
            outArr[i][j][k][l] = (int64_t)0;
          }
        }
      }
    }
  }
}

void Pad552_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4, uint64_t s5,
               uint64_t inps1, uint64_t inps2, uint64_t inps3, uint64_t inps4,
               uint64_t inps5, uint64_5D &inpArr, uint64_t pads1,
               uint64_t pads2, uint64_2D &paddings, uint64_5D &outArr) {

  uint64_t lbounds1 = paddings[(int32_t)0][(int32_t)0];

  uint64_t rbounds1excl = (PublicSub(s1, paddings[(int32_t)0][(int32_t)1]));

  uint64_t lbounds2 = paddings[(int32_t)1][(int32_t)0];

  uint64_t rbounds2excl = (PublicSub(s2, paddings[(int32_t)1][(int32_t)1]));

  uint64_t lbounds3 = paddings[(int32_t)2][(int32_t)0];

  uint64_t rbounds3excl = (PublicSub(s3, paddings[(int32_t)2][(int32_t)1]));

  uint64_t lbounds4 = paddings[(int32_t)3][(int32_t)0];

  uint64_t rbounds4excl = (PublicSub(s4, paddings[(int32_t)3][(int32_t)1]));

  uint64_t lbounds5 = paddings[(int32_t)4][(int32_t)0];

  uint64_t rbounds5excl = (PublicSub(s5, paddings[(int32_t)4][(int32_t)1]));
  for (uint64_t i = (int32_t)0; i < s1; i++) {
    for (uint64_t j = (int32_t)0; j < s2; j++) {
      for (uint64_t k = (int32_t)0; k < s3; k++) {
        for (uint64_t l = (int32_t)0; l < s4; l++) {
          for (uint64_t m = (int32_t)0; m < s5; m++) {
            if (((((((((((PublicGTE(i, lbounds1)) &&
                         (PublicLT(i, rbounds1excl))) &&
                        (PublicGTE(j, lbounds2))) &&
                       (PublicLT(j, rbounds2excl))) &&
                      (PublicGTE(k, lbounds3))) &&
                     (PublicLT(k, rbounds3excl))) &&
                    (PublicGTE(l, lbounds4))) &&
                   (PublicLT(l, rbounds4excl))) &&
                  (PublicGTE(m, lbounds5))) &&
                 (PublicLT(m, rbounds5excl)))) {
              outArr[i][j][k][l][m] =
                  inpArr[(PublicSub(i, paddings[(int32_t)0][(int32_t)0]))]
                        [(PublicSub(j, paddings[(int32_t)1][(int32_t)0]))]
                        [(PublicSub(k, paddings[(int32_t)2][(int32_t)0]))]
                        [(PublicSub(l, paddings[(int32_t)3][(int32_t)0]))]
                        [(PublicSub(m, paddings[(int32_t)4][(int32_t)0]))];
            } else {
              outArr[i][j][k][l][m] = (int64_t)0;
            }
          }
        }
      }
    }
  }
}

void PadONNX441_pt(uint64_t o1, uint64_t o2, uint64_t o3, uint64_t o4,
                   uint64_t i1, uint64_t i2, uint64_t i3, uint64_t i4,
                   uint64_4D &inpArr, uint64_t pads, uint64_1D &paddings,
                   uint64_4D &outArr) {

  uint64_t lbounds1 = paddings[(int32_t)0];

  uint64_t rbounds1excl = (PublicSub(o1, paddings[(int32_t)4]));

  uint64_t lbounds2 = paddings[(int32_t)1];

  uint64_t rbounds2excl = (PublicSub(o2, paddings[(int32_t)5]));

  uint64_t lbounds3 = paddings[(int32_t)2];

  uint64_t rbounds3excl = (PublicSub(o3, paddings[(int32_t)6]));

  uint64_t lbounds4 = paddings[(int32_t)3];

  uint64_t rbounds4excl = (PublicSub(o4, paddings[(int32_t)7]));
  for (uint64_t i = (int32_t)0; i < o1; i++) {
    for (uint64_t j = (int32_t)0; j < o2; j++) {
      for (uint64_t k = (int32_t)0; k < o3; k++) {
        for (uint64_t l = (int32_t)0; l < o4; l++) {
          if (((((((((PublicGTE(i, lbounds1)) && (PublicLT(i, rbounds1excl))) &&
                    (PublicGTE(j, lbounds2))) &&
                   (PublicLT(j, rbounds2excl))) &&
                  (PublicGTE(k, lbounds3))) &&
                 (PublicLT(k, rbounds3excl))) &&
                (PublicGTE(l, lbounds4))) &&
               (PublicLT(l, rbounds4excl)))) {
            outArr[i][j][k][l] = inpArr[(PublicSub(i, paddings[(int32_t)0]))]
                                       [(PublicSub(j, paddings[(int32_t)1]))]
                                       [(PublicSub(k, paddings[(int32_t)2]))]
                                       [(PublicSub(l, paddings[(int32_t)3]))];
          } else {
            outArr[i][j][k][l] = (int64_t)0;
          }
        }
      }
    }
  }
}

void Squeeze24_pt(uint64_t s1, uint64_t s2, uint64_t dim1, uint64_t dim2,
                  uint64_t ins1, uint64_t ins2, uint64_t ins3, uint64_t ins4,
                  uint64_4D &inArr, uint64_2D &outArr) {
  for (uint64_t i = (int32_t)0; i < ins1; i++) {
    for (uint64_t j = (int32_t)0; j < ins2; j++) {
      for (uint64_t k = (int32_t)0; k < ins3; k++) {
        for (uint64_t l = (int32_t)0; l < ins4; l++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i, ins2)), ins3)),
                                  ins4)),
                      (PublicMult((PublicMult(j, ins3)), ins4)))),
                  (PublicMult(k, ins4)))),
              l));

          uint64_t outIdx1 = (PublicDiv(linIdx, s2));

          uint64_t outIdx2 = (PublicMod(linIdx, s2));
          outArr[outIdx1][outIdx2] = inArr[i][j][k][l];
        }
      }
    }
  }
}

void FusedBatchNorm4411_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                           uint64_4D &inArr, uint64_1D &multArr,
                           uint64_1D &biasArr, uint64_t multExprScaleDownSf,
                           uint64_t biasExprScaleUpSf, uint64_4D &outputArr) {

  uint64_t inpSize = (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4));

  auto inArrReshaped = make_vector<uint64_t>(inpSize);

  auto multArrReshaped = make_vector<uint64_t>(inpSize);

  auto multExprAns = make_vector<uint64_t>(inpSize);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          inArrReshaped[linIdx] = inArr[i1][i2][i3][i4];
          multArrReshaped[linIdx] = multArr[i4];
        }
      }
    }
  }
  ElemWiseActModelVectorMult_pt(inpSize, inArrReshaped, multArrReshaped,
                                multExprAns);
  if ((PublicGT(multExprScaleDownSf, (int32_t)0))) {
    ScaleDown_pt(inpSize, multExprAns, multExprScaleDownSf);
  }

  auto biasArrScaledUp = make_vector<uint64_t>(s4);
  for (uint64_t ii = (int32_t)0; ii < s4; ii++) {
    biasArrScaledUp[ii] = biasArr[ii];
  }
  if ((PublicGT(biasExprScaleUpSf, (int32_t)0))) {
    ScaleUp_pt(s4, biasArrScaledUp, biasExprScaleUpSf);
  }
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          outputArr[i1][i2][i3][i4] =
              (PublicAdd(multExprAns[linIdx], biasArrScaledUp[i4]));
        }
      }
    }
  }
  ClearMemSecret1_pt(inpSize, inArrReshaped);
  ClearMemSecret1_pt(inpSize, multArrReshaped);
  ClearMemSecret1_pt(inpSize, multExprAns);
  ClearMemSecret1_pt(s4, biasArrScaledUp);
}

void FusedBatchNorm5511_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                           uint64_t s5, uint64_5D &inArr, uint64_1D &multArr,
                           uint64_1D &biasArr, uint64_t multExprScaleDownSf,
                           uint64_t biasExprScaleUpSf, uint64_5D &outputArr) {

  uint64_t inpSize = (PublicMult(
      (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4)), s5));

  auto inArrReshaped = make_vector<uint64_t>(inpSize);

  auto multArrReshaped = make_vector<uint64_t>(inpSize);

  auto multExprAns = make_vector<uint64_t>(inpSize);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            inArrReshaped[linIdx] = inArr[i1][i2][i3][i4][i5];
            multArrReshaped[linIdx] = multArr[i5];
          }
        }
      }
    }
  }
  ElemWiseActModelVectorMult_pt(inpSize, inArrReshaped, multArrReshaped,
                                multExprAns);
  if ((PublicGT(multExprScaleDownSf, (int32_t)0))) {
    ScaleDown_pt(inpSize, multExprAns, multExprScaleDownSf);
  }

  auto biasArrScaledUp = make_vector<uint64_t>(s5);
  for (uint64_t ii = (int32_t)0; ii < s5; ii++) {
    biasArrScaledUp[ii] = biasArr[ii];
  }
  if ((PublicGT(biasExprScaleUpSf, (int32_t)0))) {
    ScaleUp_pt(s5, biasArrScaledUp, biasExprScaleUpSf);
  }
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            outputArr[i1][i2][i3][i4][i5] =
                (PublicAdd(multExprAns[linIdx], biasArrScaledUp[i5]));
          }
        }
      }
    }
  }
  ClearMemSecret1_pt(inpSize, inArrReshaped);
  ClearMemSecret1_pt(inpSize, multArrReshaped);
  ClearMemSecret1_pt(inpSize, multExprAns);
  ClearMemSecret1_pt(s5, biasArrScaledUp);
}

void ElemWiseMul2_pt(uint64_t s1, uint64_t s2, uint64_2D &arr1, uint64_2D &arr2,
                     uint64_2D &outArr) {

  uint64_t inpSize = (PublicMult(s1, s2));

  auto arr1Reshaped = make_vector<uint64_t>(inpSize);

  auto arr2Reshaped = make_vector<uint64_t>(inpSize);

  auto outArrReshaped = make_vector<uint64_t>(inpSize);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      arr1Reshaped[linIdx] = arr1[i1][i2];
      arr2Reshaped[linIdx] = arr2[i1][i2];
    }
  }
  ElemWiseSecretSharedVectorMult_pt(inpSize, arr1Reshaped, arr2Reshaped,
                                    outArrReshaped);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      outArr[i1][i2] = outArrReshaped[linIdx];
    }
  }
  ClearMemSecret1_pt(inpSize, arr1Reshaped);
  ClearMemSecret1_pt(inpSize, arr2Reshaped);
  ClearMemSecret1_pt(inpSize, outArrReshaped);
}

void ElemWiseMul4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                     uint64_4D &arr1, uint64_4D &arr2, uint64_4D &outArr) {

  uint64_t inpSize = (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4));

  auto arr1Reshaped = make_vector<uint64_t>(inpSize);

  auto arr2Reshaped = make_vector<uint64_t>(inpSize);

  auto outArrReshaped = make_vector<uint64_t>(inpSize);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          arr1Reshaped[linIdx] = arr1[i1][i2][i3][i4];
          arr2Reshaped[linIdx] = arr2[i1][i2][i3][i4];
        }
      }
    }
  }
  ElemWiseSecretSharedVectorMult_pt(inpSize, arr1Reshaped, arr2Reshaped,
                                    outArrReshaped);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          outArr[i1][i2][i3][i4] = outArrReshaped[linIdx];
        }
      }
    }
  }
  ClearMemSecret1_pt(inpSize, arr1Reshaped);
  ClearMemSecret1_pt(inpSize, arr2Reshaped);
  ClearMemSecret1_pt(inpSize, outArrReshaped);
}

void ElemWiseMul5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                     uint64_t s5, uint64_5D &arr1, uint64_5D &arr2,
                     uint64_5D &outArr) {

  uint64_t inpSize = (PublicMult(
      (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4)), s5));

  auto arr1Reshaped = make_vector<uint64_t>(inpSize);

  auto arr2Reshaped = make_vector<uint64_t>(inpSize);

  auto outArrReshaped = make_vector<uint64_t>(inpSize);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            arr1Reshaped[linIdx] = arr1[i1][i2][i3][i4][i5];
            arr2Reshaped[linIdx] = arr2[i1][i2][i3][i4][i5];
          }
        }
      }
    }
  }
  ElemWiseSecretSharedVectorMult_pt(inpSize, arr1Reshaped, arr2Reshaped,
                                    outArrReshaped);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            outArr[i1][i2][i3][i4][i5] = outArrReshaped[linIdx];
          }
        }
      }
    }
  }
  ClearMemSecret1_pt(inpSize, arr1Reshaped);
  ClearMemSecret1_pt(inpSize, arr2Reshaped);
  ClearMemSecret1_pt(inpSize, outArrReshaped);
}

void ReduceMean24_pt(uint64_t outS1, uint64_t outS2, uint64_t inS1,
                     uint64_t inS2, uint64_t inS3, uint64_t inS4,
                     uint64_4D &inputArr, uint64_1D &axes,
                     uint64_2D &outputArr) {

  uint64_t divisor = (PublicMult(inS2, inS3));

  uint64_t outputSize = (PublicMult(outS1, outS2));

  auto sumArr = make_vector<uint64_t>(outputSize);

  auto outputArrReshaped = make_vector<uint64_t>(outputSize);
  for (uint64_t i1 = (int32_t)0; i1 < outS1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < outS2; i2++) {

      uint64_t summ = (int64_t)0;
      for (uint64_t i = (int32_t)0; i < inS2; i++) {
        for (uint64_t j = (int32_t)0; j < inS3; j++) {
          summ = (PublicAdd(summ, inputArr[i1][i][j][i2]));
        }
      }
      sumArr[(PublicAdd((PublicMult(i1, outS2)), i2))] = summ;
    }
  }
  ElemWiseVectorPublicDiv_pt(outputSize, sumArr, divisor, outputArrReshaped);
  for (uint64_t i1 = (int32_t)0; i1 < outS1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < outS2; i2++) {
      outputArr[i1][i2] =
          outputArrReshaped[(PublicAdd((PublicMult(i1, outS2)), i2))];
    }
  }
  ClearMemSecret1_pt(outputSize, sumArr);
  ClearMemSecret1_pt(outputSize, outputArrReshaped);
}

void ReduceMeanONNX24_pt(uint64_t outS1, uint64_t outS2, uint64_t inS1,
                         uint64_t inS2, uint64_t inS3, uint64_t inS4,
                         uint64_4D &inputArr, uint64_t axis1, uint64_t axis2,
                         uint64_2D &outputArr) {

  uint64_t divisor = (PublicMult(inS3, inS4));

  uint64_t outputSize = (PublicMult(outS1, outS2));

  auto sumArr = make_vector<uint64_t>(outputSize);

  auto outputArrReshaped = make_vector<uint64_t>(outputSize);
  for (uint64_t i1 = (int32_t)0; i1 < outS1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < outS2; i2++) {

      uint64_t summ = (int64_t)0;
      for (uint64_t i = (int32_t)0; i < inS3; i++) {
        for (uint64_t j = (int32_t)0; j < inS4; j++) {
          summ = (PublicAdd(summ, inputArr[i1][i2][i][j]));
        }
      }
      sumArr[(PublicAdd((PublicMult(i1, outS2)), i2))] = summ;
    }
  }
  ElemWiseVectorPublicDiv_pt(outputSize, sumArr, divisor, outputArrReshaped);
  for (uint64_t i1 = (int32_t)0; i1 < outS1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < outS2; i2++) {
      outputArr[i1][i2] =
          outputArrReshaped[(PublicAdd((PublicMult(i1, outS2)), i2))];
    }
  }
  ClearMemSecret1_pt(outputSize, sumArr);
  ClearMemSecret1_pt(outputSize, outputArrReshaped);
}

void ArgMax1_pt(uint64_t outArrS1, uint64_t inArrS1, uint64_t inArrS2,
                uint64_2D &inArr, uint64_t dim, uint64_1D &outArr) {
  ArgMax_pt(inArrS1, inArrS2, inArr, outArr);
}

void ArgMax3_pt(uint64_t outs1, uint64_t outs2, uint64_t outs3, uint64_t ins1,
                uint64_t ins2, uint64_t ins3, uint64_t ins4, uint64_4D &inArr,
                uint64_t dim, uint64_3D &outArr) {

  uint64_t size = (PublicMult((PublicMult(ins1, ins2)), ins3));

  auto reshapedInArr = make_vector<uint64_t>(size, ins4);

  auto reshapedOutArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < ins1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < ins2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < ins3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < ins4; i4++) {

          uint64_t linIdx =
              (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, ins2)), ins3)),
                                    (PublicMult(i2, ins3)))),
                         i3));
          reshapedInArr[linIdx][i4] = inArr[i1][i2][i3][i4];
        }
      }
    }
  }
  ArgMax_pt(size, ins4, reshapedInArr, reshapedOutArr);
  for (uint64_t i1 = (int32_t)0; i1 < ins1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < ins2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < ins3; i3++) {

        uint64_t linIdx =
            (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, ins2)), ins3)),
                                  (PublicMult(i2, ins3)))),
                       i3));
        outArr[i1][i2][i3] = reshapedOutArr[linIdx];
      }
    }
  }
  ClearMemSecret2_pt(size, ins4, reshapedInArr);
  ClearMemSecret1_pt(size, reshapedOutArr);
}

void Relu2_pt(uint64_t s1, uint64_t s2, uint64_2D &inArr, uint64_2D &outArr,
              uint64_t sf, uint64_t doTruncation) {

  uint64_t size = (PublicMult(s1, s2));

  auto reshapedInArr = make_vector<uint64_t>(size);

  auto reshapedOutArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      reshapedInArr[linIdx] = inArr[i1][i2];
    }
  }
  Relu_pt(size, reshapedInArr, reshapedOutArr, sf, doTruncation);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      outArr[i1][i2] = reshapedOutArr[linIdx];
    }
  }
  ClearMemSecret1_pt(size, reshapedInArr);
  ClearMemSecret1_pt(size, reshapedOutArr);
}

void Relu4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
              uint64_4D &inArr, uint64_4D &outArr, uint64_t sf,
              uint64_t doTruncation) {

  uint64_t size = (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4));

  auto reshapedInArr = make_vector<uint64_t>(size);

  auto reshapedOutArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          reshapedInArr[linIdx] = inArr[i1][i2][i3][i4];
        }
      }
    }
  }
  Relu_pt(size, reshapedInArr, reshapedOutArr, sf, doTruncation);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          outArr[i1][i2][i3][i4] = reshapedOutArr[linIdx];
        }
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedInArr);
  ClearMemSecret1_pt(size, reshapedOutArr);
}

void Relu5_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4, uint64_t s5,
              uint64_5D &inArr, uint64_5D &outArr, uint64_t sf,
              uint64_t doTruncation) {

  uint64_t size = (PublicMult(
      (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4)), s5));

  auto reshapedInArr = make_vector<uint64_t>(size);

  auto reshapedOutArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            reshapedInArr[linIdx] = inArr[i1][i2][i3][i4][i5];
          }
        }
      }
    }
  }
  Relu_pt(size, reshapedInArr, reshapedOutArr, sf, doTruncation);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {
          for (uint64_t i5 = (int32_t)0; i5 < s5; i5++) {

            uint64_t linIdx = (PublicAdd(
                (PublicAdd(
                    (PublicAdd(
                        (PublicAdd(
                            (PublicMult(
                                (PublicMult(
                                    (PublicMult((PublicMult(i1, s2)), s3)),
                                    s4)),
                                s5)),
                            (PublicMult((PublicMult((PublicMult(i2, s3)), s4)),
                                        s5)))),
                        (PublicMult((PublicMult(i3, s4)), s5)))),
                    (PublicMult(i4, s5)))),
                i5));
            outArr[i1][i2][i3][i4][i5] = reshapedOutArr[linIdx];
          }
        }
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedInArr);
  ClearMemSecret1_pt(size, reshapedOutArr);
}

void Floor2_pt(uint64_t s1, uint64_t s2, uint64_2D &inArr, uint64_2D &outArr,
               uint64_t sf) {

  uint64_t size = (PublicMult(s1, s2));

  auto reshapedInArr = make_vector<uint64_t>(size);

  auto reshapedOutArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      reshapedInArr[linIdx] = inArr[i1][i2];
    }
  }
  Floor_pt(size, reshapedInArr, reshapedOutArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      outArr[i1][i2] = reshapedOutArr[linIdx];
    }
  }
  ClearMemSecret1_pt(size, reshapedInArr);
  ClearMemSecret1_pt(size, reshapedOutArr);
}

void ScaleUp1_pt(uint64_t s1, uint64_1D &arr, uint64_t sf) {
  ScaleUp_pt(s1, arr, sf);
}

void ScaleUp2_pt(uint64_t s1, uint64_t s2, uint64_2D &arr, uint64_t sf) {

  uint64_t size = (PublicMult(s1, s2));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      reshapedArr[linIdx] = arr[i1][i2];
    }
  }
  ScaleUp_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      arr[i1][i2] = reshapedArr[linIdx];
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void ScaleUp3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_3D &arr,
                 uint64_t sf) {

  uint64_t size = (PublicMult((PublicMult(s1, s2)), s3));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {

        uint64_t linIdx =
            (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, s2)), s3)),
                                  (PublicMult(i2, s3)))),
                       i3));
        reshapedArr[linIdx] = arr[i1][i2][i3];
      }
    }
  }
  ScaleUp_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {

        uint64_t linIdx =
            (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, s2)), s3)),
                                  (PublicMult(i2, s3)))),
                       i3));
        arr[i1][i2][i3] = reshapedArr[linIdx];
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void ScaleUp4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                 uint64_4D &arr, uint64_t sf) {

  uint64_t size = (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          reshapedArr[linIdx] = arr[i1][i2][i3][i4];
        }
      }
    }
  }
  ScaleUp_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          arr[i1][i2][i3][i4] = reshapedArr[linIdx];
        }
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void ScaleDown1_pt(uint64_t s1, uint64_1D &arr, uint64_t sf) {
  ScaleDown_pt(s1, arr, sf);
}

void ScaleDown2_pt(uint64_t s1, uint64_t s2, uint64_2D &arr, uint64_t sf) {

  uint64_t size = (PublicMult(s1, s2));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      reshapedArr[linIdx] = arr[i1][i2];
    }
  }
  ScaleDown_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {

      uint64_t linIdx = (PublicAdd((PublicMult(i1, s2)), i2));
      arr[i1][i2] = reshapedArr[linIdx];
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void ScaleDown3_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_3D &arr,
                   uint64_t sf) {

  uint64_t size = (PublicMult((PublicMult(s1, s2)), s3));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {

        uint64_t linIdx =
            (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, s2)), s3)),
                                  (PublicMult(i2, s3)))),
                       i3));
        reshapedArr[linIdx] = arr[i1][i2][i3];
      }
    }
  }
  ScaleDown_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {

        uint64_t linIdx =
            (PublicAdd((PublicAdd((PublicMult((PublicMult(i1, s2)), s3)),
                                  (PublicMult(i2, s3)))),
                       i3));
        arr[i1][i2][i3] = reshapedArr[linIdx];
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void ScaleDown4_pt(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4,
                   uint64_4D &arr, uint64_t sf) {

  uint64_t size = (PublicMult((PublicMult((PublicMult(s1, s2)), s3)), s4));

  auto reshapedArr = make_vector<uint64_t>(size);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          reshapedArr[linIdx] = arr[i1][i2][i3][i4];
        }
      }
    }
  }
  ScaleDown_pt(size, reshapedArr, sf);
  for (uint64_t i1 = (int32_t)0; i1 < s1; i1++) {
    for (uint64_t i2 = (int32_t)0; i2 < s2; i2++) {
      for (uint64_t i3 = (int32_t)0; i3 < s3; i3++) {
        for (uint64_t i4 = (int32_t)0; i4 < s4; i4++) {

          uint64_t linIdx = (PublicAdd(
              (PublicAdd(
                  (PublicAdd(
                      (PublicMult((PublicMult((PublicMult(i1, s2)), s3)), s4)),
                      (PublicMult((PublicMult(i2, s3)), s4)))),
                  (PublicMult(i3, s4)))),
              i4));
          arr[i1][i2][i3][i4] = reshapedArr[linIdx];
        }
      }
    }
  }
  ClearMemSecret1_pt(size, reshapedArr);
}

void Conv2DWrapper_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                      uint64_t FH, uint64_t FW, uint64_t CO, uint64_t zPadHLeft,
                      uint64_t zPadHRight, uint64_t zPadWLeft,
                      uint64_t zPadWRight, uint64_t strideH, uint64_t strideW,
                      uint64_4D &inputArr, uint64_4D &filterArr,
                      uint64_4D &outArr) {
  Conv2D_pt(N, H, W, CI, FH, FW, CO, zPadHLeft, zPadHRight, zPadWLeft,
            zPadWRight, strideH, strideW, inputArr, filterArr, outArr);
}

void Conv3DWrapper_pt(uint64_t N, uint64_t D, uint64_t H, uint64_t W,
                      uint64_t CI, uint64_t FD, uint64_t FH, uint64_t FW,
                      uint64_t CO, uint64_t zPadDLeft, uint64_t zPadDRight,
                      uint64_t zPadHLeft, uint64_t zPadHRight,
                      uint64_t zPadWLeft, uint64_t zPadWRight, uint64_t strideD,
                      uint64_t strideH, uint64_t strideW, uint64_5D &inputArr,
                      uint64_5D &filterArr, uint64_5D &outArr) {
  Conv3D_pt(N, D, H, W, CI, FD, FH, FW, CO, zPadDLeft, zPadDRight, zPadHLeft,
            zPadHRight, zPadWLeft, zPadWRight, strideD, strideH, strideW,
            inputArr, filterArr, outArr);
}

void Conv2DGroupWrapper_pt(uint64_t N, uint64_t H, uint64_t W, uint64_t CI,
                           uint64_t FH, uint64_t FW, uint64_t CO,
                           uint64_t zPadHLeft, uint64_t zPadHRight,
                           uint64_t zPadWLeft, uint64_t zPadWRight,
                           uint64_t strideH, uint64_t strideW, uint64_t G,
                           uint64_4D &inputArr, uint64_4D &filterArr,
                           uint64_4D &outArr) {
  Conv2DGroup_pt(N, H, W, CI, FH, FW, CO, zPadHLeft, zPadHRight, zPadWLeft,
                 zPadWRight, strideH, strideW, G, inputArr, filterArr, outArr);
}

void ConvTranspose2DWrapper_pt(uint64_t N, uint64_t HPrime, uint64_t WPrime,
                               uint64_t CI, uint64_t FH, uint64_t FW,
                               uint64_t CO, uint64_t H, uint64_t W,
                               uint64_t zPadTrHLeft, uint64_t zPadTrHRight,
                               uint64_t zPadTrWLeft, uint64_t zPadTrWRight,
                               uint64_t strideH, uint64_t strideW,
                               uint64_4D &inputArr, uint64_4D &filterArr,
                               uint64_4D &outArr) {
  ConvTranspose2D_pt(N, HPrime, WPrime, CI, FH, FW, CO, H, W, zPadTrHLeft,
                     zPadTrHRight, zPadTrWLeft, zPadTrWRight, strideH, strideW,
                     inputArr, filterArr, outArr);
}

void ConvTranspose3DWrapper_pt(
    uint64_t N, uint64_t DPrime, uint64_t HPrime, uint64_t WPrime, uint64_t CI,
    uint64_t FD, uint64_t FH, uint64_t FW, uint64_t CO, uint64_t D, uint64_t H,
    uint64_t W, uint64_t zPadTrDLeft, uint64_t zPadTrDRight,
    uint64_t zPadTrHLeft, uint64_t zPadTrHRight, uint64_t zPadTrWLeft,
    uint64_t zPadTrWRight, uint64_t strideD, uint64_t strideH, uint64_t strideW,
    uint64_5D &inputArr, uint64_5D &filterArr, uint64_5D &outArr) {
  ConvTranspose3D_pt(N, DPrime, HPrime, WPrime, CI, FD, FH, FW, CO, D, H, W,
                     zPadTrDLeft, zPadTrDRight, zPadTrHLeft, zPadTrHRight,
                     zPadTrWLeft, zPadTrWRight, strideD, strideH, strideW,
                     inputArr, filterArr, outArr);
}

#endif
