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

#include "cleartext_library_fixed.h"

using namespace std;
using namespace sci;

inline int64_t Saturate(int32_t inp) { return (int64_t)inp; }

void cleartext_Sigmoid(int64_t *A, int I, int J, int scale_in, int scale_out,
                       int bwA, int bwB, int64_t *B) {
  int32_t sA = log2(scale_in);
  int32_t sB = log2(scale_out);

  int64_t *neg_A = new int64_t[I * J];
  for (int i = 0; i < I * J; i++) {
    if (A[i] < 0) {
      neg_A[i] = int64_t(A[i]);
    } else {
      neg_A[i] = (-1LL * int64_t(A[i]));
    }
    // std::cout << neg_A[i] << std::endl;
  }

  int64_t *exp_neg_A = new int64_t[I * J];
  cleartext_Exp_lookup(neg_A, I, J, bwA, scale_in, scale_out, exp_neg_A, 1);
  int64_t *sig_neg_A = new int64_t[I * J];
  int64_t *all1 = new int64_t[I * J];
  int64_t *den = new int64_t[I * J];
  for (int i = 0; i < I * J; i++) {
    if (exp_neg_A[i] == (1 << sB)) {
      den[i] = (exp_neg_A[i] + (1LL << sB) - 1);
    } else {
      den[i] = (exp_neg_A[i] + (1LL << sB));
    }
    all1[i] = 1;
  }
  cleartext_div(all1, den, I, J, 1, scale_out, scale_out, sig_neg_A, false);

  int64_t error = 0;
  for (int i = 0; i < I * J; i++) {
    if (A[i] >= 0) {
      B[i] = all1Mask(bwB) & sig_neg_A[i];
    } else {
      B[i] = all1Mask(bwB) & ((exp_neg_A[i] * sig_neg_A[i]) >> sB);
    }
    double flA = (A[i] / double(1LL << sA));
    double sig_A = 1.0 / (1.0 + exp(-1.0 * flA));
    int64_t actualB = sig_A * (1LL << sB);
    error += abs(actualB - B[i]);
  }
  if (party == 2) {
    std::cout << "Sigmoid Average Error: " << error / (I * J) << std::endl;
  }

  delete[] neg_A;
  delete[] exp_neg_A;
  delete[] all1;
  delete[] sig_neg_A;
  delete[] den;

  return;
}

void cleartext_TanH(int64_t *A, int32_t I, int32_t J, int64_t scale_in,
                    int64_t scale_out, int32_t bwA, int32_t bwB, int64_t *B) {
#ifdef APPROXIMATE_TANH
  for (int i = 0; i < I * J; i++) {
    float a = float(A[i]) / scale_in;
    float b = a;

    if (b < -1)
      b = -1;
    else if (b > 1)
      b = 1;
    B[i] = b * scale_out;
  }
#else
  int32_t sA = log2(scale_in);
  int32_t sB = log2(scale_out);

  int64_t *neg_A = new int64_t[I * J];
  for (int i = 0; i < I * J; i++) {
    if (A[i] < 0) {
      neg_A[i] = int64_t(A[i]);
    } else {
      neg_A[i] = (-1LL * int64_t(A[i]));
    }
  }
  int64_t *exp_neg_2A = new int64_t[I * J];
  // scale_in/2 because we need exp(-2x)
  cleartext_Exp_lookup(neg_A, I, J, bwA, scale_in / 2, scale_out, exp_neg_2A,
                       1);
  int64_t *tanh_neg_A = new int64_t[I * J];
  int64_t *num = new int64_t[I * J];
  int64_t *den = new int64_t[I * J];
  for (int i = 0; i < I * J; i++) {
    if (exp_neg_2A[i] == (1 << sB)) {
      den[i] = (exp_neg_2A[i] + (1LL << sB) - 1);
    } else {
      den[i] = (exp_neg_2A[i] + (1LL << sB));
    }
    num[i] = (1LL << sB) - exp_neg_2A[i];
  }
  cleartext_div(num, den, I, J, scale_out, scale_out, scale_out, tanh_neg_A,
                false);

  int64_t error = 0;
  for (int i = 0; i < I * J; i++) {
    if (A[i] >= 0) {
      B[i] = Saturate(tanh_neg_A[i]);
    } else {
      B[i] = Saturate(-1 * tanh_neg_A[i]);
    }
    double flA = (A[i] / double(1LL << sA));
    double tanh_A = tanh(flA);
    int64_t actualB = tanh_A * (1LL << sB);
    error += abs(actualB - B[i]);
  }
  if (party == 2) {
    std::cout << "TanH Average Error: " << error / (I * J) << std::endl;
  }

  delete[] neg_A;
  delete[] exp_neg_2A;
  delete[] num;
  delete[] den;
  delete[] tanh_neg_A;

  return;
#endif
}

void cleartext_MatAdd(int64_t *A, int64_t *B, int64_t *C, int32_t I, int32_t J,
                      int32_t shrA, int32_t shrB, int32_t shrC,
                      int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t b = (int64_t)B[i * J + j];

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = b / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = b >> (shiftB + shiftC);
#endif

      int64_t c = a + b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_MatSub(int64_t *A, const int64_t *B, int64_t *C, int32_t I,
                      int32_t J, int32_t shrA, int32_t shrB, int32_t shrC,
                      int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t b = (int64_t)B[i * J + j];

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = b / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = b >> (shiftB + shiftC);
#endif

      int64_t c = a - b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_MatMul(int64_t *A, int64_t *B, int64_t *C, int64_t *tmp,
                      int32_t I, int32_t K, int32_t J, int32_t shrA,
                      int32_t shrB, int32_t H1, int32_t H2, int32_t demote) {
  // /*
  // int64_t* temp = new int64_t[K];
  int64_t *temp = new int64_t[K];
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);
  int depth = ceil(log2(K));
  int32_t shift = shiftA + shiftB + shift_demote + H1 - depth;

  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      for (int k = 0; k < K; k++) {
        int64_t a = (int64_t)A[i * K + k];
        int64_t b = (int64_t)B[k * J + j];

        // int64_t prod = a * b;
        int64_t prod = int64_t(a) * int64_t(b);

        temp[k] = prod;
      }
      // int64_t sum = 0;
      int64_t sum = 0;
      for (int k = 0; k < K; k++) {
        // sum += (temp[k] >> depth);
        sum += temp[k];
      }
      sum = (sum >> depth);
      // C[i * J + j] = Saturate(sum >> (shiftA + shiftB + shift_demote));
      if (shift >= 0) {
#ifdef DIV_RESCALING
        C[i * J + j] = Saturate(sum / (1LL << shift));
#else
        C[i * J + j] = Saturate(sum >> shift);
#endif
      } else {
        C[i * J + j] = Saturate(sum * (1LL << (-1 * shift)));
      }
    }
  }

  delete[] temp;
  return;
  // */
}

void cleartext_MatAddBroadCastA(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)*A;
      int64_t b = (int64_t)B[i * J + j];

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = b / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = b >> (shiftB + shiftC);
#endif

      int64_t c = a + b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_MatAddBroadCastB(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t b = (int64_t)*B;

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = b / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = b >> (shiftB + shiftC);
#endif

      int64_t c = a + b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_MatSubBroadCastA(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)*A;
      int64_t b = (int64_t)B[i * J + j];

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = (-1 * b) / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = (-1 * b) >> (shiftB + shiftC);
#endif

      int64_t c = a + b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_MatSubBroadCastB(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                                int32_t J, int32_t shrA, int32_t shrB,
                                int32_t shrC, int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t b = (int64_t)*B;

#ifdef DIV_RESCALING
      a = a / (shrA * shrC);
      b = (-1 * b) / (shrB * shrC);
#else
      a = a >> (shiftA + shiftC);
      b = (-1 * b) >> (shiftB + shiftC);
#endif

      int64_t c = a + b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(c / demote);
#else
      C[i * J + j] = Saturate(c >> shift_demote);
#endif
    }
  }
  return;
}

void cleartext_ScalarMul(int64_t *A, int64_t *B, int64_t *C, int32_t I,
                         int32_t J, int32_t shrA, int32_t shrB, int demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);
  int64_t a = (int64_t)*A;
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t b = (int64_t)B[i * J + j];

      int64_t prod = a * b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(prod / (shrA * shrB * demote));
#else
      C[i * J + j] = Saturate(prod >> (shiftA + shiftB + shift_demote));
#endif
    }
  }

  return;
}

void cleartext_MulCir(int64_t *A, int64_t *B, int64_t *C, int32_t I, int32_t J,
                      int32_t shrA, int32_t shrB, int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);

  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t b = (int64_t)B[i * J + j];

      int64_t prod = a * b;

#ifdef DIV_RESCALING
      C[i * J + j] = Saturate(prod / (shrA * shrB * demote));
#else
      C[i * J + j] = Saturate(prod >> (shiftA + shiftB + shift_demote));
#endif
    }
  }
  return;
}

void cleartext_sqrt(int64_t *A, int32_t I, int32_t J, int32_t shrA,
                    int32_t shrB, int32_t bwA, int32_t bwB, int64_t *B,
                    bool inverse) {
  int32_t sA = log2(shrA);
  int32_t sB = log2(shrB);
  int32_t m, iters;
  if (sB <= 14) {
    m = ceil(sB / 2.0);
    iters = 1;
  } else {
    m = ceil((ceil(sB / 2.0)) / 2.0);
    iters = 2;
  }
  assert(m <= KKOT_LIMIT);
  assert(sA >= m);

  int64_t m_mask = (1LL << m) - 1;
  int64_t error = 0;
  for (int i = 0; i < I * J; i++) {
    assert(A[i] >= 0);
    int32_t msnzb = floor(log2(int64_t(A[i])));
    int32_t adjust_amt = bwA - 2 - msnzb;
    int64_t adjust = (1LL << adjust_amt);
    int64_t sqrt_adjust_scale =
        (inverse ? floor((bwA - 1 - sA) / 2.0) : floor((sA + 1) / 2.0));
    int64_t sqrt_adjust;
    if (inverse) {
      sqrt_adjust =
          (1LL << int(floor((sA - msnzb + 1) / 2.0) + sqrt_adjust_scale));
    } else {
      sqrt_adjust = (1LL << int(floor((msnzb - sA) / 2.0) + sqrt_adjust_scale));
    }
    int64_t exp_parity = ((msnzb - sA) & 1);
    // shifted_A: bw = bwA - 1, scale = bwA - 2
    int64_t shifted_A = int64_t(A[i]) * adjust;

    // Top m bits of shifted_A excluding MSB, which is always 1
    // A_m: bw = m, scale = m
    int64_t A_m = (int64_t(shifted_A) >> (bwA - 2 - m)) & m_mask;
    // Y: bw = m + SQRT_LOOKUP_SCALE + 1, scale = m + SQRT_LOOKUP_SCALE
    int64_t Y = lookup_sqrt(A_m, m, exp_parity);

    // X = (exp_parity ? 2 * shifted_A : shifted_A); scale = bwA - 2
    // X: bw = bwA
    int64_t X = (exp_parity ? 2 * shifted_A : shifted_A);

    int64_t x_prev;
    if (inverse) {
      // x_prev : bw = m + SQRT_LOOKUP_SCALE + 1, scale = m + SQRT_LOOKUP_SCALE
      // x_prev \approx 0.5 < 1/sqrt(X) < 1
      x_prev = Y;
    } else {
      // x_prev : bw = sB + 1, scale = sB
      // x_prev \approx 1 < sqrt(X) < 2
      x_prev = (X * Y);
      if (bwA - 2 + m + SQRT_LOOKUP_SCALE > sB) {
        x_prev = x_prev >> (bwA - 2 + m + SQRT_LOOKUP_SCALE - sB);
      } else {
        x_prev = x_prev << (sB - bwA + 2 - m - SQRT_LOOKUP_SCALE);
      }
    }
    // b_prev_scale = sB
    int64_t b_prev = X;
    if (bwA - 2 > sB) {
      b_prev = b_prev >> (bwA - 2 - sB);
    } else {
      b_prev = b_prev << (sB + 2 - bwA);
    }
    // Y_prev_scale = m + 1
    int64_t Y_prev = Y;
    int64_t x_curr, b_curr, Y_curr;
    for (int j = 0; j < iters; j++) {
      int64_t Y_sq = Y_prev * Y_prev;
      if (j == 0) {
        // b_curr: bw = sB + 2, scale = sB
        b_curr = (Y_sq * b_prev) >> (2 * m + 2 * SQRT_LOOKUP_SCALE);
      } else {
        Y_sq = Y_sq >> (sB + 2);
        // b_curr: bw = sB + 2, scale = sB
        b_curr = (Y_sq * b_prev) >> sB;
      }
      // Y_curr: bw = sB + 2, scale = sB + 1
      // Y_curr = (3 - b_curr)/2
      Y_curr = (1.5 * (1LL << (sB + 1))) - b_curr;
      x_curr = x_prev * Y_curr;
      if (inverse && (j == 0)) {
        // x_curr: bw = sB + 1, scale = sB
        x_curr = x_curr >> (m + SQRT_LOOKUP_SCALE + 1);
      } else {
        // x_curr: bw = sB + 1, scale = sB
        x_curr = x_curr >> (sB + 1);
      }
      x_prev = x_curr;
      b_prev = b_curr;
      Y_prev = Y_curr;
    }
    int64_t res;
    if (iters > 0 || (!inverse)) {
      res = x_curr * sqrt_adjust;
      res = res >> sqrt_adjust_scale;
    } else {
      // Scale of x_curr is m + 1 if iters == 0 and sB = m
      res = x_curr * sqrt_adjust;
      res = res >> (sqrt_adjust_scale + SQRT_LOOKUP_SCALE);
    }
    B[i] = Saturate(res);

    double flA = (A[i] / double(1LL << sA));
    double sqrt_A = (inverse ? 1.0 / sqrt(flA) : sqrt(flA));
    int64_t actualB = sqrt_A * (1LL << sB);
    error += abs(actualB - B[i]);
  }
  if (party == 2) {
    std::cout << "Sqrt Average Error: " << error / (I * J) << std::endl;
  }

  return;
}

void cleartext_div(int64_t *A, int64_t *B, int32_t I, int32_t J, int32_t shrA,
                   int32_t shrB, int32_t shrC, int64_t *C, bool compute_msnzb) {
  int32_t bwA = sizeof(int64_t) * 8;
  int32_t bwB = sizeof(int64_t) * 8;
  int32_t bwC = sizeof(int64_t) * 8;
  int32_t sA = log2(shrA);
  int32_t sB = log2(shrB);
  int32_t sC = log2(shrC);
  int32_t m, iters;
  m = (sC <= 18 ? ceil((sC - 2) / 2.0) : ceil((ceil(sC / 2.0) - 2) / 2.0));
  iters = (sC <= 18 ? 0 : 1);
  assert(m <= KKOT_LIMIT);
  assert(sB >= m);

  int64_t m_mask = (1LL << m) - 1;
  uint64_t s_min_m_mask = (1ULL << (sB - m)) - 1;
  int64_t error = 0;
  for (int i = 0; i < I * J; i++) {
    int32_t tmp_sB;
    int32_t msnzb, s_adjust;
    int64_t adjust, tmpB;
    if (compute_msnzb) {
      msnzb = floor(log2(int64_t(A[i])));
      tmp_sB = bwB - 1;
      s_adjust = bwB - 1 - sB;
      int32_t adjust_amt = bwB - 1 - msnzb;
      adjust = (1LL << adjust_amt);
      tmpB = (adjust * int64_t(B[i]));
    } else {
      assert((B[i] >= (1LL << sB)) && (B[i] < (2LL << sB)));
      tmpB = int64_t(B[i]);
      tmp_sB = sB;
    }
    int64_t B_m = (tmpB >> (tmp_sB - m)) & m_mask;
    int64_t A0 = lookup_div_A0(B_m, m);
    int64_t A1 = lookup_div_A1(B_m, m);
    int64_t Q = tmpB & s_min_m_mask;
    int64_t A0_Q = A0 * Q;
    // reciprocal approximation of B with precision sC
    int64_t Y = ((A1 << (tmp_sB + 1 - m)) - A0_Q) >> (tmp_sB + m + 3 - sC);
    int64_t a0 = (int64_t(A[i]) * Y) >> sA;
    if (compute_msnzb) {
      a0 = (a0 * adjust) >> s_adjust;
    }
    if (iters > 0) {
      int64_t e = (tmpB * Y) >> sB;
      // e0 = 1 - e
      int64_t e0 = (1LL << sC) - e;
      int64_t a_prev = a0, e_prev = e0, a_curr, e_curr;
      for (int j = 0; j < iters - 1; j++) {
        e_curr = (e_prev * e_prev) >> sC;
        a_curr = (a_prev * ((1LL << sC) + e_prev)) >> sC;
        a_prev = a_curr;
        e_prev = e_curr;
      }
      a0 = (a_prev * ((1LL << sC) + e_prev)) >> sC;
    }
    C[i] = Saturate(a0);

    double flA = (A[i] / double(1LL << sA));
    double flB = (B[i] / double(1LL << sB));
    double A_by_B = flA / flB;
    int64_t actualC = A_by_B * (1LL << sC);
    error += abs(actualC - C[i]);
  }
  if (party == 2) {
    std::cout << "Div Average Error: " << error / (I * J) << std::endl;
  }

  return;
}

void cleartext_Exp_lookup(int64_t *A, int32_t I, int32_t J, int bwA,
                          int32_t shrA, int32_t shrB, int64_t *B,
                          int32_t demote) {
  int32_t sA = log2(shrA);
  int32_t sB = log2(shrB);
  int32_t s_demote = log2(demote);
  int32_t LUT_size = KKOT_LIMIT;

  int digit_size = LUT_size;
  int num_digits = ceil(double(bwA) / digit_size);
  int last_digit_size = bwA - (num_digits - 1) * digit_size;
  int64_t digit_mask = (digit_size == 64 ? -1 : (1LL << digit_size) - 1);
  int64_t A_digits[num_digits];
  int64_t error = 0;
  for (int i = 0; i < I * J; i++) {
    assert(A[i] <= 0);
    int64_t neg_A = -1LL * (A[i]);
    for (int j = 0; j < num_digits; j++) {
      A_digits[j] = (neg_A >> (j * digit_size)) & digit_mask;
      A_digits[j] = lookup_neg_exp(A_digits[j], sA - digit_size * j, sB);
    }
    for (int j = 1; j < num_digits; j *= 2) {
      for (int k = 0; k < num_digits and k + j < num_digits; k += 2 * j) {
        A_digits[k] = (A_digits[k + j] * A_digits[k]) >> sB;
      }
    }
    B[i] = Saturate(A_digits[0] >> s_demote);
    double flA = (A[i] / double(1LL << sA));
    double expA = exp(flA);
    int64_t actualB = expA * (1LL << sB);
    error += abs(actualB - B[i]);
  }
  if (party == 2) {
    std::cout << "Exp Average Error: " << error / (I * J) << std::endl;
  }

  return;
}

void cleartext_ArgMax(int64_t *A, int32_t I, int32_t J, int32_t *index) {
  int64_t max = A[0];
  int32_t maxIndex = 0, counter = 0;
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t x = A[i * J + j];

      if (max < x) {
        maxIndex = counter;
        max = x;
      }

      counter++;
    }
  }

  *index = maxIndex;

  return;
}

void cleartext_Convolution(int64_t *A, int64_t *B, int64_t *C, int64_t *tmp,
                           int32_t N, int32_t H, int32_t W, int32_t CIN,
                           int32_t HF, int32_t WF, int32_t CINF, int32_t COUTF,
                           int32_t HOUT, int32_t WOUT, int32_t HPADL,
                           int32_t HPADR, int32_t WPADL, int32_t WPADR,
                           int32_t HSTR, int32_t WSTR, int32_t HDL, int32_t WDL,
                           int32_t G, int32_t shrA, int32_t shrB, int32_t H1,
                           int32_t H2, int32_t demote) {
  int32_t HOffsetL = HDL * (HF / 2) - HPADL;
  int32_t WOffsetL = WDL * (WF / 2) - WPADL;
  int32_t HOffsetR = HDL * (HF / 2) - HPADR;
  int32_t WOffsetR = WDL * (WF / 2) - WPADR;

  int K = HF * WF * CINF;
  int64_t *temp = new int64_t[K];
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);
  int depth = ceil(log2(K));
  int32_t shift = shiftA + shiftB + shift_demote + H1 - depth;

  for (int32_t n = 0; n < N; n++) {
    for (int32_t h = HOffsetL, hout = 0; h < H - HOffsetR; h += HSTR, hout++) {
      for (int32_t w = WOffsetL, wout = 0; w < W - WOffsetR;
           w += WSTR, wout++) {
        for (int32_t g = 0; g < G; g++) {
          for (int32_t co = 0; co < COUTF; co++) {

            int32_t counter = 0;
            for (int32_t hf = -(HF / 2); hf <= HF / 2; hf++) {
              for (int32_t wf = -(WF / 2); wf <= WF / 2; wf++) {
                for (int32_t ci = 0; ci < CINF; ci++) {

                  int64_t a =
                      (int64_t)(((h + HDL * hf) < 0) || ((h + HDL * hf) >= H) ||
                                ((w + WDL * wf) < 0) || ((w + WDL * wf) >= W))
                          ? 0
                          : A[n * H * W * CIN + (h + HDL * hf) * W * CIN +
                              (w + WDL * wf) * CIN + (ci + g * CINF)];

                  int64_t b = (int64_t)
                      B[g * HF * WF * CINF * COUTF +
                        (hf + HF / 2) * WF * CINF * COUTF +
                        (wf + WF / 2) * CINF * COUTF + ci * COUTF + co];

                  // tmp[counter] = a * b;
                  int64_t prod = int64_t(a) * int64_t(b);
                  temp[counter] = prod;
                  counter++;
                }
              }
            }

            int64_t sum = 0;
            for (int k = 0; k < K; k++) {
              sum += temp[k];
            }
            sum = (sum >> depth);
            int C_idx = n * HOUT * WOUT * (COUTF * G) +
                        hout * WOUT * (COUTF * G) + wout * (COUTF * G) +
                        (co + g * COUTF);
            if (shift >= 0) {
#ifdef DIV_RESCALING
              C[C_idx] = Saturate(sum / (1LL << shift));
#else
              C[C_idx] = Saturate(sum >> shift);
#endif
            } else {
              C[C_idx] = Saturate(sum * (1LL << (-1 * shift)));
            }
            // C[n * HOUT * WOUT * (COUTF * G) + hout * WOUT * (COUTF * G) +
            // wout * (COUTF * G) + (co + g * COUTF)] = Saturate(((tmp[0] /
            // shrA) / shrB) / demote);
          }
        }
      }
    }
  }
  delete[] temp;
}

void cleartext_AddOrSubCir4D(int64_t *A, int64_t *B, int64_t *X, int32_t N,
                             int32_t H, int32_t W, int32_t C, int32_t shrA,
                             int32_t shrB, int32_t shrC, bool add,
                             int32_t demote) {
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);

  for (int n = 0; n < N; n++) {
    for (int h = 0; h < H; h++) {
      for (int w = 0; w < W; w++) {
        for (int c = 0; c < C; c++) {
          int64_t a = (int64_t)A[n * H * W * C + h * W * C + w * C + c];
          int64_t b = (int64_t)B[c];
#ifdef DIV_RESCALING
          a = a / (shrA * shrC);
          b = b / (shrB * shrC);
#else
          a = a >> (shiftA + shiftC);
          b = b >> (shiftB + shiftC);
#endif

          int64_t res;
          if (add)
            res = a + b;
          else
            res = a - b;

#ifdef DIV_RESCALING
          X[n * H * W * C + h * W * C + w * C + c] = Saturate(res / demote);
#else
          X[n * H * W * C + h * W * C + w * C + c] =
              Saturate(res >> shift_demote);
#endif
        }
      }
    }
  }
  return;
}

void cleartext_Relu6(int64_t *A, int64_t *B, int32_t N, int32_t H, int32_t W,
                     int32_t C, int64_t six, int32_t div) {
  int32_t shift_div = log2(div);

  for (int n = 0; n < N; n++) {
    for (int h = 0; h < H; h++) {
      for (int w = 0; w < W; w++) {
        for (int c = 0; c < C; c++) {
          int64_t a = A[n * H * W * C + h * W * C + w * C + c];
          if (a < 0)
            a = 0;
          if (a > six)
            a = six;
#ifdef DIV_RESCALING
          B[n * H * W * C + h * W * C + w * C + c] = (int64_t)(a / div);
#else
          B[n * H * W * C + h * W * C + w * C + c] = (int64_t)(a >> shift_div);
#endif
        }
      }
    }
  }
  return;
}

void cleartext_BNorm(int64_t *A, int64_t *BNW, int64_t *BNB, int64_t *B,
                     int32_t I, int32_t J, int32_t shA, int32_t shBNB,
                     int32_t shB) {
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      int64_t a = (int64_t)A[i * J + j];
      int64_t bn_b = (int64_t)BNB[j];
      int64_t bn_w = (int64_t)BNW[j];

      // sh = +ve : right shift
      // sh = -ve : left shift
      if (shA <= 0) {
        a = (a * (1LL << (-1 * shA)));
      } else {
#ifdef DIV_RESCALING
        a = a / (1LL << shA);
#else
        a = a >> shA;
#endif
      }
      if (shBNB <= 0) {
        bn_b = (bn_b * (1LL << (-1 * shBNB)));
      } else {
#ifdef DIV_RESCALING
        bn_b = bn_b / (1LL << shBNB);
#else
        bn_b = bn_b >> shBNB;
#endif
      }

      int64_t b = (a + bn_b) * bn_w;

      if (shB <= 0) {
        B[i * J + j] = Saturate(b * (1LL << (-1 * shB)));
      } else {
#ifdef DIV_RESCALING
        B[i * J + j] = Saturate(b / (1LL << shB));
#else
        B[i * J + j] = Saturate(b >> shB);
#endif
      }
    }
  }
  return;
}

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
                      int64_t shl6, int64_t shl7, int64_t shl8, int64_t shl9) {
  int32_t HOffsetL = (HF / 2) - HPADL;
  int32_t WOffsetL = (WF / 2) - WPADL;
  int32_t HOffsetR = (HF / 2) - HPADR;
  int32_t WOffsetR = (WF / 2) - WPADR;

  bool del_X = false;
  if (X == nullptr) {
    X = new int64_t[H * W * Ct];
    del_X = true;
  }

  bool del_T = false;
  if (T == nullptr) {
    T = new int64_t[Ct];
    del_T = true;
  }

  bool del_U = false;
  if (U == nullptr) {
    U = new int64_t[Cin + HF * WF + Ct];
    del_U = true;
  }

  int32_t shift1 = log2(shr1);
  int32_t shift2 = log2(shr2);
  int32_t shift3 = log2(shr3);
  int32_t shift4 = log2(shr4);
  int32_t shift5 = log2(shr5);
  int32_t shift6 = log2(shr6);
  int32_t shift7 = log2(shr7);
  int32_t shift8 = log2(shr8);
  int32_t shift9 = log2(shr9);

  for (int32_t n = 0; n < N; n++) {
    int32_t margin =
        HOffsetL + (HF / 2 + 1) - HSTR > 0 ? HOffsetL + (HF / 2 + 1) - HSTR : 0;
    int32_t nstart = HOffsetL - (HF / 2) < 0 ? 0 : HOffsetL - (HF / 2);
    for (int32_t i = nstart; i < margin; i++) {
      for (int32_t j = 0; j < W; j++) {
        for (int32_t k = 0; k < Ct; k++) {
          int64_t tmpU = 0;
          for (int32_t l = 0; l < Cin; l++) {
            tmpU += ((int64_t)A[n * H * W * Cin + i * W * Cin + j * Cin + l]) *
                    ((int64_t)F1[l * Ct + k]);
          }
          tmpU = tmpU >> D1;

          // std::cout << i << "\t" << tmpU << std::endl;
#ifdef DIV_RESCALING
          int64_t x = (((int64_t)((int64_t(tmpU) * shl1) / shr1 +
                                  (BN1B[k] * shl2) / shr2)) *
                       ((int64_t)BN1W[k]));
#else
          int64_t x = (((int64_t)(((int64_t(tmpU) * shl1) >> shift1) +
                                  ((BN1B[k] * shl2) >> shift2))) *
                       ((int64_t)BN1W[k]));
#endif
          x = x < 0 ? 0 : x;
          x = x > SIX_1 ? SIX_1 : x;
#ifdef DIV_RESCALING
          X[i * W * Ct + j * Ct + k] = Saturate((x * shl3) / shr3);
#else
          X[i * W * Ct + j * Ct + k] = Saturate((x * shl3) >> shift3);
#endif
          // std::cout << i << "\t" << X[i * W * Ct + j * Ct + k] << std::endl;
        }
      }
    }

    for (int32_t h = HOffsetL, hout = 0; h < H - HOffsetR; hout++, h += HSTR) {

      for (int32_t i = 0; i < HSTR; i++) {
        for (int32_t j = 0; j < W; j++) {
          for (int32_t k = 0; k < Ct; k++) {
            int32_t iRed = (i + margin + hout * HSTR) % HF,
                    iFull = i + margin + hout * HSTR;
            X[iRed * W * Ct + j * Ct + k] = 0;
            int64_t tmpU = 0;
            for (int32_t l = 0; l < Cin; l++) {
              int64_t a =
                  iFull < H ? A[n * H * W * Cin + iFull * W * Cin + j * Cin + l]
                            : 0;
              tmpU += ((int64_t)a) * ((int64_t)F1[l * Ct + k]);
            }
            tmpU = tmpU >> D1;

            // std::cout << iFull << "\t" << tmpU << std::endl;
#ifdef DIV_RESCALING
            int64_t x = (((int64_t)((int64_t(tmpU) * shl1) / shr1 +
                                    (BN1B[k] * shl2) / shr2)) *
                         ((int64_t)BN1W[k]));
#else
            int64_t x = (((int64_t)(((int64_t(tmpU) * shl1) >> shift1) +
                                    ((BN1B[k] * shl2) >> shift2))) *
                         ((int64_t)BN1W[k]));
#endif
            x = x < 0 ? 0 : x;
            x = x > SIX_1 ? SIX_1 : x;
#ifdef DIV_RESCALING
            X[iRed * W * Ct + j * Ct + k] = Saturate((x * shl3) / shr3);
#else
            X[iRed * W * Ct + j * Ct + k] = Saturate((x * shl3) >> shift3);
#endif
            // std::cout << iFull << "\t" << X[iRed * W * Ct + j * Ct + k] <<
            // std::endl;
          }
        }
      }

      for (int32_t w = WOffsetL, wout = 0; w < W - WOffsetR;
           w += WSTR, wout++) {
        for (int32_t g = 0; g < Ct; g++) {
          int32_t counter = 0;
          int64_t tmpU = 0;
          for (int32_t hf = -(HF / 2); hf <= (HF / 2); hf++) {
            for (int32_t wf = -(WF / 2); wf <= (WF / 2); wf++) {
              int64_t x = (((h + hf) < 0) || ((h + hf) >= H) ||
                           ((w + wf) < 0) || ((w + wf) >= W))
                              ? 0
                              : X[((h + hf) % HF) * W * Ct + (w + wf) * Ct + g];
              int64_t b = F2[g * HF * WF + (hf + HF / 2) * WF + (wf + WF / 2)];
              tmpU += ((int64_t)x) * ((int64_t)b);
              counter++;
            }
          }
          tmpU = tmpU >> D2;

          // std::cout << tmpU << std::endl;

#ifdef DIV_RESCALING
          int64_t x = (((int64_t)((int64_t(tmpU) * shl4) / shr4 +
                                  (BN2B[g] * shl5) / shr5)) *
                       ((int64_t)BN2W[g]));
#else
          int64_t x = (((int64_t)(((int64_t(tmpU) * shl4) >> shift4) +
                                  ((BN2B[g] * shl5) >> shift5))) *
                       ((int64_t)BN2W[g]));
#endif
          x = x < 0 ? 0 : x;
          x = x > SIX_2 ? SIX_2 : x;
#ifdef DIV_RESCALING
          T[g] = Saturate((x * shl6) / shr6);
#else
          T[g] = Saturate((x * shl6) >> shift6);
#endif
        }

        for (int32_t i = 0; i < Cout; i++) {
          int64_t tmpU = 0;
          for (int32_t g = 0; g < Ct; g++) {
            tmpU += T[g] * F3[g * Cout + i];
          }
          tmpU = tmpU >> D3;

#ifdef DIV_RESCALING
          C[n * Hout * Wout * Cout + hout * Wout * Cout + wout * Cout + i] =
              Saturate(((((int64_t)((tmpU * shl7) / shr7 +
                                    (BN3B[i] * shl8) / shr8)) *
                         ((int64_t)BN3W[i])) *
                        shl9) /
                       shr9);
#else
          C[n * Hout * Wout * Cout + hout * Wout * Cout + wout * Cout + i] =
              Saturate(((((int64_t)(((tmpU * shl7) >> shift7) +
                                    ((BN3B[i] * shl8) >> shift8))) *
                         ((int64_t)BN3W[i])) *
                        shl9) >>
                       shift9);
#endif
        }
      }
    }
  }

  if (del_X)
    delete[] X;
  if (del_T)
    delete[] T;
  if (del_U)
    delete[] U;
}

void cleartext_NormaliseL2(int64_t *A, int64_t *B, int32_t N, int32_t H,
                           int32_t W, int32_t C, int32_t scaleA, int32_t shrA,
                           int32_t bwA, int32_t bwB) {
  int32_t scale_in = -1 * scaleA;
  int32_t scale_out = -1 * (scaleA + 1);
  int32_t shrAdiv = (1 << shrA);
  int64_t *sumSquare = new int64_t[N * H * W];
  int64_t *inverseNorm = new int64_t[N * H * W];
  for (int32_t i = 0; i < N * H * W; i++) {
    // calculate the sum square
    sumSquare[i] = 0;
    for (int32_t j = 0; j < C; j++) {
      int64_t tmp = A[i * C + j];
      sumSquare[i] += int64_t((tmp * tmp) >> (2 * shrA));
    }
  }
  cleartext_sqrt(sumSquare, N * H * W, 1, 1 << (2 * (scale_in - shrA)),
                 1 << (scale_out - scale_in + shrA), bwA, bwB, inverseNorm,
                 true);
  /*
  for(int i = 0; i < N*H*W; i++) {
      std::cout << double(sumSquare[i])/(1LL << (2*(scale_in-shrA))) << "\t" <<
  double(inverseNorm[i])/(1LL << (scale_out-scale_in+shrA)) << std::endl;
  }
  */

  for (int32_t i = 0; i < N * H * W; i++) {
    // multiply all elements by the 1/sqrt(sumSquare)
    for (int32_t j = 0; j < C; j++) {
#ifdef DIV_RESCALING
      B[i * C + j] =
          (int64_t(A[i * C + j]) * int64_t(inverseNorm[i])) / (shrAdiv);
#else
      B[i * C + j] = (int64_t(A[i * C + j]) * int64_t(inverseNorm[i])) >> shrA;
#endif
    }
  }
  delete[] sumSquare;
  delete[] inverseNorm;

  return;
}

void cleartext_NormaliseL2_seedot(int64_t *A, int64_t *B, int32_t N, int32_t H,
                                  int32_t W, int32_t C, int32_t scaleA,
                                  int32_t shrA) {
  for (int32_t n = 0; n < N; n++) {
    for (int32_t h = 0; h < H; h++) {
      for (int32_t w = 0; w < W; w++) {

        // calculate the sum square
        int32_t sumSquare = 0;
        int32_t shrAdiv = (1 << shrA);

        for (int32_t c = 0; c < C; c++) {
#ifdef FASTAPPROX
          int32_t tmp = (A[n * H * W * C + h * W * C + w * C + c] / shrAdiv);
          sumSquare += tmp * tmp;
#else
          int32_t tmp = A[n * H * W * C + h * W * C + w * C + c];
          sumSquare += (((tmp * tmp) / shrAdiv) / shrAdiv);
#endif
        }

        // calculate the inverse square root of sumSquare
        int32_t yLow = 1;

        // yHigh: A number of length shrA with all 1s in binary representation
        // e.g. for shrA=8 --> y_high = 0b11111111
        int32_t yHigh = ((1 << shrA) - 1);

        // one: value of 1 with same scale as y*y*sumSquare
        // scale of sumSquare = 2*scale_in + 2*shrA
        // since we assume scale of y = 1 - shrA
        // scale of y*y*sumSquare =  2*scale_in + 2*shrA + 2(1-shrA) =
        // 2*scale_in + 2
        int32_t one = (1 << (-(2 * scaleA + 2)));

        // binary search for the inverse square root
        while (yLow + 1 < yHigh) {

          // using int32_t sotherwise (y*y*sumSquare) will overflow
          int32_t yMid = ((yHigh + yLow) >> 1);

          int64_t cmpValue = (int64_t)sumSquare * yMid * yMid;

          if (cmpValue > one) {
            yHigh = yMid;
          } else {
            yLow = yMid;
          }
        }
        int32_t inverseNorm = yLow;

        // multiply all elements by the 1/sqrt(sumSquare)
        for (int32_t c = 0; c < C; c++) {
          B[n * H * W * C + h * W * C + w * C + c] =
              (A[n * H * W * C + h * W * C + w * C + c] / shrAdiv) *
              inverseNorm;
        }
      }
    }
  }
  return;
}

void cleartext_AdjustScaleShr(int64_t *A, int32_t I, int32_t J, int32_t scale) {
  int32_t shift_scale = ceil(log2(scale));
  for (int32_t i = 0; i < I; i++) {
    for (int32_t j = 0; j < J; j++) {
      int64_t a = A[i * J + j];
#ifdef DIV_RESCALING
      A[i * J + j] = a / scale;
#else
      A[i * J + j] = a >> shift_scale;
#endif
    }
  }
  return;
}

void cleartext_MaxPool2D(int64_t *A, int32_t I, int32_t J, int64_t *B) {
  int32_t counter = 0;
  bool verbose = false;
  for (int i = 0; i < I; i++) {
    int64_t max = A[i * J + 0];
    for (int j = 0; j < J; j++) {
      int64_t x = max - A[i * J + j];
      int64_t diff = signed_val_cleartext(x, sizeof(int64_t) * 8);

      if (verbose) {
        std::cout << "max: " << max << " | cur: " << A[i * J + j] << " | diff:\
                    "
                  << diff << std::endl;
      }

      if (diff <= 0) {
        max = A[i * J + j];
      }
    }
    verbose = false;
    B[i] = (int64_t)max;
  }

  return;
}
