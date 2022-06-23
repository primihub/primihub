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

#include "src/primihub/protocol/cryptflow2/LinearOT/linear-ot.h"
#include <cmath>
#include <omp.h>
using namespace primihub::cryptflow2;
#define MAX_NUM_OT (1 << 24)

#define USE_EIGEN
#ifdef USE_EIGEN
#include <Eigen/Dense>
#endif

using namespace std;
using namespace primihub::sci;

#define START_IDX 0
#define print_vec(ot, vec, bw, N)                     \
  {                                                   \
    uint64_t *rec_vec = new uint64_t[N];              \
    reconstruct(ot, N, vec + START_IDX, rec_vec, bw); \
    std::cout << #vec << "_pub:" << std::endl;        \
    for (int i = 0; i < N; i++)                       \
    {                                                 \
      std::cout << rec_vec[i] << " ";                 \
    }                                                 \
    std::cout << std::endl;                           \
    delete[] rec_vec;                                 \
  }

#define print_share(vec, bw, N)                  \
  {                                              \
    std::cout << #vec << "_share:" << std::endl; \
    for (int i = 0; i < N; i++)                  \
    {                                            \
      std::cout << vec[i] << " ";                \
    }                                            \
    std::cout << std::endl;                      \
  }

void reconstruct(LinearOT *ot, int dim, uint64_t *x, uint64_t *y, int bw_x)
{
  uint64_t mask = (bw_x == 64 ? -1 : ((1ULL << bw_x) - 1));
  if (ot->party == primihub::sci::ALICE)
  {
    ot->iopack->io->send_data(x, dim * sizeof(uint64_t));
    for (int i = 0; i < dim; i++)
    {
      y[i] = 0;
    }
  }
  else
  {
    ot->iopack->io->recv_data(y, dim * sizeof(uint64_t));
    for (int i = 0; i < dim; i++)
    {
      y[i] = (y[i] + x[i]) & mask;
    }
  }
}

void matrix_transpose(uint64_t *A, int32_t m, int32_t n, int d = 1)
{
  uint64_t *tmpA = new uint64_t[m * n * d];
  memcpy(tmpA, A, m * n * d * sizeof(uint64_t));
  for (int i = 0; i < m; i++)
  {
    for (int j = 0; j < n; j++)
    {
      if (d == 1)
      {
        A[j * m + i] = tmpA[i * n + j];
      }
      else
      {
        memcpy(A + (j * m + i) * d, tmpA + (i * n + j) * d,
               d * sizeof(uint64_t));
      }
    }
  }
  delete[] tmpA;
}

LinearOT::LinearOT(int party, IOPack *iopack, OTPack *otpack)
{
  this->party = party;
  this->iopack = iopack;
  this->otpack = otpack;
  this->aux = new AuxProtocols(party, iopack, otpack);
  this->trunc = new Truncation(party, iopack, otpack);
  this->xt = new XTProtocol(party, iopack, otpack);
}

LinearOT::~LinearOT()
{
  delete aux;
  delete trunc;
  delete xt;
}

void LinearOT::hadamard_cleartext(int dim, uint64_t *inA, uint64_t *inB,
                                  uint64_t *outC)
{
  matmul_cleartext(1, dim, 1, inA, inB, outC, false);
}

#ifdef USE_EIGEN
void matmul_cleartext_eigen(int dim1, int dim2, int dim3, uint64_t *inA,
                            uint64_t *inB, uint64_t *outC)
{
  Eigen::Matrix<uint64_t, Eigen::Dynamic, Eigen::Dynamic> eigen_A(dim1, dim2);
  Eigen::Matrix<uint64_t, Eigen::Dynamic, Eigen::Dynamic> eigen_B(dim2, dim3);
  Eigen::Matrix<uint64_t, Eigen::Dynamic, Eigen::Dynamic> eigen_C(dim1, dim3);

  for (int i = 0; i < dim1; i++)
  {
    for (int j = 0; j < dim2; j++)
    {
      eigen_A(i, j) = Arr2DIdxRowM(inA, dim1, dim2, i, j);
    }
  }
  for (int i = 0; i < dim2; i++)
  {
    for (int j = 0; j < dim3; j++)
    {
      eigen_B(i, j) = Arr2DIdxRowM(inB, dim2, dim3, i, j);
    }
  }
  eigen_C = eigen_A * eigen_B;
  for (int i = 0; i < dim1; i++)
  {
    for (int j = 0; j < dim3; j++)
    {
      Arr2DIdxRowM(outC, dim1, dim3, i, j) = eigen_C(i, j);
    }
  }
}
#endif

void LinearOT::matmul_cleartext(int dim1, int dim2, int dim3, uint64_t *inA,
                                uint64_t *inB, uint64_t *outC,
                                bool accumulate)
{
  if (!accumulate)
  {
    for (int i = 0; i < dim1; i++)
    {
      for (int j = 0; j < dim3; j++)
      {
        for (int k = 0; k < dim2; k++)
        {
          outC[dim2 * dim3 * i + dim2 * j + k] =
              inA[dim2 * i + k] * inB[dim3 * k + j];
        }
      }
    }
    return;
  }
#ifndef USE_EIGEN
  for (int i = 0; i < dim1; i++)
  {
    for (int j = 0; j < dim3; j++)
    {
      outC[i * dim3 + j] = 0;
      for (int k = 0; k < dim2; k++)
      {
        outC[i * dim3 + j] += (inA[dim2 * i + k] * inB[dim3 * k + j]);
      }
    }
  }
#else
  assert(accumulate == true && "Eigen not configured for accumulate = false");
  matmul_cleartext_eigen(dim1, dim2, dim3, inA, inB, outC);
#endif
}

void LinearOT::hadamard_cross_terms(int32_t dim, uint64_t *inA, uint64_t *inB,
                                    uint64_t *outC, int32_t bwA, int32_t bwB,
                                    int32_t bwC, MultMode mode)
{
  matmul_cross_terms(1, dim, 1, inA, inB, outC, bwA, bwB, bwC, false, mode);
}

void LinearOT::hadamard_product(int32_t dim, uint64_t *inA, uint64_t *inB,
                                uint64_t *outC, int32_t bwA, int32_t bwB,
                                int32_t bwC, bool signed_arithmetic,
                                bool signed_B, MultMode mode, uint8_t *msbA,
                                uint8_t *msbB)
{
  matrix_multiplication(1, dim, 1, inA, inB, outC, bwA, bwB, bwC,
                        signed_arithmetic, signed_B, false, mode, msbA, msbB);
}

// Cost with matrix of dimension dim1xdim2 and bitwidth m used as OT receiver
double cross_term_cost(int dim1, int dim2, int dim3, int m, int n, int l)
{
  return dim1 * dim2 * (m * 128.0 + dim3 * (m * (l - m) + (m * m + m) / 2.0));
}

void LinearOT::matmul_cross_terms(int32_t dim1, int32_t dim2, int32_t dim3,
                                  uint64_t *inA, uint64_t *inB, uint64_t *outC,
                                  int32_t bwA, int32_t bwB, int32_t bwC,
                                  bool accumulate, MultMode mode)
{
  bool use_straight_ot = false, use_reversed_ot = false;
  uint64_t maskC = (bwC == 64 ? -1 : ((1ULL << bwC) - 1));
  uint64_t *inS, *inR;
  int32_t bwR, dimS1, dimS2, dimR1, dimR2;
  // A whole row of values is multiplied to a element using only bwR OTs
  bool row_batching;
  if (mode == MultMode::Alice_has_A)
  {
    use_straight_ot = true;
    row_batching = false;
  }
  else if (mode == MultMode::Bob_has_A)
  {
    use_straight_ot = true;
    row_batching = true;
  }
  else if (mode == MultMode::Alice_has_B)
  {
    use_reversed_ot = true;
    row_batching = false;
  }
  else if (mode == MultMode::Bob_has_B)
  {
    use_reversed_ot = true;
    row_batching = true;
  }
  else
  {
    use_straight_ot = true;
    use_reversed_ot = true;
    // if (bwA*dim1*dim2 > bwB*dim2*dim3) {
    // if (bwA > bwB) {
    if (cross_term_cost(dim1, dim2, dim3, bwA, bwB, bwC) > cross_term_cost(dim3, dim2, dim1, bwB, bwA, bwC))
    {
      row_batching = false;
    }
    else
    {
      row_batching = true;
    }
  }
  if (row_batching)
  {
    inS = inB;
    inR = inA;
    bwR = bwA;
    dimS1 = dim2;
    dimS2 = dim3;
    dimR1 = dim1;
    dimR2 = dim2;
  }
  else
  {
    // inS = inA;
    inS = new uint64_t[dim1 * dim2];
    memcpy(inS, inA, dim1 * dim2 * sizeof(uint64_t));
    inR = inB;
    bwR = bwB;
    dimS1 = dim1;
    dimS2 = dim2;
    dimR1 = dim2;
    dimR2 = dim3;
  }

  int32_t num_ot = dimR1 * dimR2 * bwR;
  int32_t msgs_per_ot = (row_batching ? dimS2 : dimS1);
  int32_t dim = dimR1 * dimR2;
  // max_num_ot is multiple of dim
  int32_t max_num_ot = ceil(float(MAX_NUM_OT) / (dim * msgs_per_ot)) * dim;
  int32_t batch_size;
  if (num_ot < max_num_ot)
    batch_size = num_ot;
  else
    batch_size = max_num_ot;

  uint64_t *corr = new uint64_t[batch_size * msgs_per_ot];
  uint64_t *ABs = new uint64_t[batch_size * msgs_per_ot];
  uint64_t *ABr = new uint64_t[batch_size * msgs_per_ot];
  bool *choice = new bool[batch_size];
  uint64_t *tmpR = new uint64_t[dim];
  memcpy(tmpR, inR, dim * sizeof(uint64_t));

  memset(ABs, 0, batch_size * msgs_per_ot * sizeof(uint64_t));
  memset(ABr, 0, batch_size * msgs_per_ot * sizeof(uint64_t));
  if (accumulate)
  {
    memset(outC, 0, dim1 * dim3 * sizeof(uint64_t));
  }
  else
  {
    memset(outC, 0, dim1 * dim2 * dim3 * sizeof(uint64_t));
  }
  if (!row_batching)
  {
    // inplace transposing is fine because inS is a copy if row_batching = false
    matrix_transpose(inS, dimS1, dimS2);
  }

  for (int i = 0; i < num_ot; i += batch_size)
  {
    vector<int> msg_len;
    if (batch_size <= num_ot - i)
      msg_len.resize(batch_size / dim);
    else
      msg_len.resize((num_ot - i) / dim);
    for (int j = i; j < i + batch_size and j < num_ot; j += dim)
    {
      int bit_offset = i / dim;
      int bit_idx = j / dim;
      msg_len[bit_idx - bit_offset] = bwC - bit_idx;
      for (int k = j; k < j + dim and k < i + batch_size; k++)
      {
        int inp_idx = k - j;
        int row_idx_R = inp_idx / dimR2;
        int col_idx_R = inp_idx % dimR2;
        for (int h = 0; h < msgs_per_ot; h++)
        {
          uint64_t elemS;
          if (row_batching)
            elemS = inS[col_idx_R * dimS2 + h];
          // To preserve locality, inS is tranposed if row_batching = false
          else
            elemS = inS[row_idx_R * dimS1 + h];
          // else elemS = inS[h*dimS2 + row_idx_R];
          corr[(k - i) * msgs_per_ot + h] =
              ((elemS << bit_idx) & maskC) >> bit_idx;
        }
        choice[k - i] = (bool)(tmpR[inp_idx] & 1);
        tmpR[inp_idx] >>= 1;
      }
    }
#pragma omp parallel num_threads(2)
    {
      if (omp_get_thread_num() == 1 && use_reversed_ot)
      {
        if (party == primihub::sci::ALICE)
        {
          if (batch_size <= num_ot - i)
          {
            otpack->iknp_reversed->recv_batched_cot(ABr, choice, msg_len,
                                                    batch_size, msgs_per_ot);
          }
          else
          {
            otpack->iknp_reversed->recv_batched_cot(ABr, choice, msg_len,
                                                    num_ot - i, msgs_per_ot);
          }
        }
        else
        { // party == primihub::sci::BOB
          if (batch_size <= num_ot - i)
          {
            otpack->iknp_reversed->send_batched_cot(ABs, corr, msg_len,
                                                    batch_size, msgs_per_ot);
          }
          else
          {
            otpack->iknp_reversed->send_batched_cot(ABs, corr, msg_len,
                                                    num_ot - i, msgs_per_ot);
          }
        }
      }
      else if (omp_get_thread_num() == 0 && use_straight_ot)
      {
        if (party == primihub::sci::ALICE)
        {
          if (batch_size <= num_ot - i)
          {
            otpack->iknp_straight->send_batched_cot(ABs, corr, msg_len,
                                                    batch_size, msgs_per_ot);
          }
          else
          {
            otpack->iknp_straight->send_batched_cot(ABs, corr, msg_len,
                                                    num_ot - i, msgs_per_ot);
          }
        }
        else
        { // party == primihub::sci::BOB
          if (batch_size <= num_ot - i)
          {
            otpack->iknp_straight->recv_batched_cot(ABr, choice, msg_len,
                                                    batch_size, msgs_per_ot);
          }
          else
          {
            otpack->iknp_straight->recv_batched_cot(ABr, choice, msg_len,
                                                    num_ot - i, msgs_per_ot);
          }
        }
      }
    }
    for (int h = 0; h < dim2; h++)
    {
      for (int j = i; j < i + batch_size and j < num_ot; j += dim)
      {
        int bit_idx = j / dim;
        for (int k = 0; k < dim1 * dim3; k++)
        {
          int row_idx = (row_batching ? k / dim3 : k / dim1);
          int col_idx = (row_batching ? k % dim3 : k % dim1);
          int idx;
          if (row_batching)
            idx = (j - i + row_idx * dimR2 + h) * msgs_per_ot + col_idx;
          // To preserve locality, outC-transpose is computed if row_batching =
          // false
          else
            idx = (j - i + h * dimR2 + row_idx) * msgs_per_ot + col_idx;
          if (use_straight_ot)
          {
            uint64_t temp = (party == ALICE ? -ABs[idx] : ABr[idx]) << bit_idx;
            if (accumulate)
            {
              outC[k] += temp;
            }
            else
            {
              outC[k * dim2 + h] += temp;
            }
          }
          if (use_reversed_ot)
          {
            uint64_t temp = (party == BOB ? -ABs[idx] : ABr[idx]) << bit_idx;
            if (accumulate)
            {
              outC[k] += temp;
            }
            else
            {
              outC[k * dim2 + h] += temp;
            }
          }
        }
      }
    }
  }
  if (!row_batching)
  {
    if (accumulate)
    {
      matrix_transpose(outC, dim3, dim1);
    }
    else
    {
      matrix_transpose(outC, dim3, dim1, dim2);
    }
  }

  if (accumulate)
  {
    for (int k = 0; k < dim1 * dim3; k++)
    {
      outC[k] &= maskC;
    }
  }
  else
  {
    for (int k = 0; k < dim1 * dim2 * dim3; k++)
    {
      outC[k] &= maskC;
    }
  }

  delete[] corr;
  delete[] ABs;
  delete[] ABr;
  delete[] choice;
  delete[] tmpR;
  if (!row_batching)
    delete[] inS;
}

void LinearOT::matmul_multiplexer(int32_t dim1, int32_t dim2, int32_t dim3,
                                  uint64_t *inA, uint64_t *inB, uint64_t *outC,
                                  int32_t bwA, int32_t bwB, int32_t bwC,
                                  bool accumulate, MultMode mode)
{
  assert(bwA == 1 || bwB == 1);
  assert(bwC < (bwA + bwB));

  bool use_straight_ot = false, use_reversed_ot = false;
  uint64_t maskC = (bwC == 64 ? -1 : ((1ULL << bwC) - 1));
  uint64_t *inS, *inR;
  int32_t dimS1, dimS2, dimR1, dimR2;
  // A whole row of values is multiplied to a bit
  bool row_batching;
  if (bwB == 1)
  {
    inS = new uint64_t[dim1 * dim2];
    memcpy(inS, inA, dim1 * dim2 * sizeof(uint64_t));
    inR = inB;
    dimS1 = dim1;
    dimS2 = dim2;
    dimR1 = dim2;
    dimR2 = dim3;
    if (mode == MultMode::Alice_has_A)
      use_straight_ot = true;
    else if (mode == MultMode::Bob_has_A)
      use_reversed_ot = true;
    else
    {
      use_straight_ot = true;
      use_reversed_ot = true;
    }
    row_batching = false;
  }
  else
  { // bwA == 1
    inS = inB;
    inR = inA;
    dimS1 = dim2;
    dimS2 = dim3;
    dimR1 = dim1;
    dimR2 = dim2;
    if (mode == MultMode::Alice_has_B)
      use_straight_ot = true;
    else if (mode == MultMode::Bob_has_B)
      use_reversed_ot = true;
    else
    {
      use_straight_ot = true;
      use_reversed_ot = true;
    }
    row_batching = true;
  }

  int32_t dim = dimR1 * dimR2;
  int32_t msgs_per_ot = (row_batching ? dimS2 : dimS1);

  PRG128 prg;
  uint64_t *data = new uint64_t[2 * dim * msgs_per_ot];
  uint64_t *ABs = new uint64_t[dim * msgs_per_ot];
  uint64_t *ABr = new uint64_t[dim * msgs_per_ot];
  uint8_t *choice = new uint8_t[dim];

  if (accumulate)
  {
    for (int k = 0; k < dim1 * dim3; k++)
    {
      outC[k] = 0;
    }
  }
  else
  {
    for (int k = 0; k < dim1 * dim2 * dim3; k++)
    {
      outC[k] = 0;
    }
  }
  memset(ABs, 0, dim * msgs_per_ot * sizeof(uint64_t));
  memset(ABr, 0, dim * msgs_per_ot * sizeof(uint64_t));

  if (!row_batching)
  {
    // inplace transposing is fine because inS is a copy if row_batching = false
    matrix_transpose(inS, dimS1, dimS2);
  }

  if (use_straight_ot && party == ALICE)
  {
    prg.random_data(ABs, dim * msgs_per_ot * sizeof(uint64_t));
  }
  if (use_reversed_ot && party == BOB)
  {
    prg.random_data(ABs, dim * msgs_per_ot * sizeof(uint64_t));
  }

  for (int i = 0; i < dim; i++)
  {
    int row_idx_R = i / dimR2;
    int col_idx_R = i % dimR2;

    choice[i] = (bool)(inR[i] & 1);
    uint64_t choice_bit = choice[i];
    for (int h = 0; h < msgs_per_ot; h++)
    {
      uint64_t elemS;
      if (row_batching)
        elemS = inS[col_idx_R * dimS2 + h];
      // To preserve locality, inS is tranposed if row_batching = false
      else
        elemS = inS[row_idx_R * dimS1 + h];
      int idx = i * msgs_per_ot + h;
      data[idx * 2] = (ABs[idx] + elemS * choice_bit) & maskC;
      data[idx * 2 + 1] = (ABs[idx] + elemS * (1 - choice_bit)) & maskC;
    }
  }
#pragma omp parallel num_threads(2)
  {
    if (omp_get_thread_num() == 1 && use_reversed_ot)
    {
      if (party == primihub::sci::ALICE)
      {
        otpack->iknp_reversed->recv_batched_got(ABr, choice, dim, bwC,
                                                msgs_per_ot);
      }
      else
      { // party == primihub::sci::BOB
        otpack->iknp_reversed->send_batched_got(data, dim, bwC, msgs_per_ot);
      }
    }
    else if (omp_get_thread_num() == 0 && use_straight_ot)
    {
      if (party == primihub::sci::ALICE)
      {
        otpack->iknp_straight->send_batched_got(data, dim, bwC, msgs_per_ot);
      }
      else
      { // party == primihub::sci::BOB
        otpack->iknp_straight->recv_batched_got(ABr, choice, dim, bwC,
                                                msgs_per_ot);
      }
    }
  }
  for (int h = 0; h < dim2; h++)
  {
    for (int k = 0; k < dim1 * dim3; k++)
    {
      int row_idx = (row_batching ? k / dim3 : k / dim1);
      int col_idx = (row_batching ? k % dim3 : k % dim1);
      int idx;
      if (row_batching)
        idx = (row_idx * dimR2 + h) * msgs_per_ot + col_idx;
      // To preserve locality, outC-transpose is computed if row_batching =
      // false
      else
        idx = (h * dimR2 + row_idx) * msgs_per_ot + col_idx;
      if (use_straight_ot)
      {
        uint64_t temp = (party == ALICE ? -ABs[idx] : ABr[idx]);
        if (accumulate)
        {
          outC[k] += temp;
        }
        else
        {
          outC[k * dim2 + h] += temp;
        }
      }
      if (use_reversed_ot)
      {
        uint64_t temp = (party == BOB ? -ABs[idx] : ABr[idx]);
        if (accumulate)
        {
          outC[k] += temp;
        }
        else
        {
          outC[k * dim2 + h] += temp;
        }
      }
    }
  }
  if (!row_batching)
  {
    if (accumulate)
    {
      matrix_transpose(outC, dim3, dim1);
    }
    else
    {
      matrix_transpose(outC, dim3, dim1, dim2);
    }
  }

  if (accumulate)
  {
    for (int k = 0; k < dim1 * dim3; k++)
    {
      outC[k] &= maskC;
    }
  }
  else
  {
    for (int k = 0; k < dim1 * dim2 * dim3; k++)
    {
      outC[k] &= maskC;
    }
  }

  delete[] data;
  delete[] ABs;
  delete[] ABr;
  delete[] choice;
  if (!row_batching)
    delete[] inS;
}

uint64_t extension_cost(int dim1, int dim2, int m, int n, uint8_t *msb)
{
  if (msb != nullptr)
  {
    return dim1 * dim2 * (128.0 * (m + 1) + 13 * m + n);
  }
  else
  {
    return dim1 * dim2 * (128.0 * 2 - m + n + 2);
  }
}

void LinearOT::matrix_multiplication(int32_t dim1, int32_t dim2, int32_t dim3,
                                     uint64_t *inA, uint64_t *inB,
                                     uint64_t *outC, int32_t bwA, int32_t bwB,
                                     int32_t bwC, bool signed_arithmetic,
                                     bool signed_B, bool accumulate,
                                     MultMode mode, uint8_t *msbA,
                                     uint8_t *msbB)
{
  assert(bwC <= 64);
  assert((bwC <= (bwA + bwB)) && (bwC >= bwA) && (bwC >= bwB));
  int32_t extra_bits = (accumulate ? ceil(log2(dim2)) : 0);
  uint64_t *tmpA = new uint64_t[dim1 * dim2];
  uint64_t *tmpB = new uint64_t[dim2 * dim3];

  bool sender_A = false;
  bool sender_B = false;
  if (mode == MultMode::Alice_has_A || mode == MultMode::Alice_has_B)
  {
    sender_A = true;
  }
  else if (mode == MultMode::Bob_has_A || mode == MultMode::Bob_has_B)
  {
    sender_B = true;
  }
  else
  {
    if (extension_cost(dim1, dim2, bwA, bwA + extra_bits, msbA) > extension_cost(dim2, dim3, bwB, bwB + extra_bits, msbB))
    {
      sender_A = true;
    }
    else
    {
      sender_B = true;
    }
  }
  if (sender_A)
  {
    if (mode == MultMode::Alice_has_A || mode == MultMode::Bob_has_A)
    {
      for (int i = 0; i < dim1 * dim2; i++)
      {
        tmpA[i] = (signed_arithmetic ? signed_val(inA[i], bwA) : inA[i]);
      }
    }
    else
    {
      if (signed_arithmetic)
      {
        xt->s_extend(dim1 * dim2, inA, tmpA, bwA, bwA + extra_bits, msbA);
      }
      else
      {
        xt->z_extend(dim1 * dim2, inA, tmpA, bwA, bwA + extra_bits, msbA);
      }
    }
    memcpy(tmpB, inB, dim2 * dim3 * sizeof(uint64_t));
    bwA = bwA + extra_bits;
  }
  else if (sender_B)
  {
    if (mode == MultMode::Alice_has_B || mode == MultMode::Bob_has_B)
    {
      for (int i = 0; i < dim2 * dim3; i++)
      {
        tmpB[i] = ((signed_arithmetic && signed_B) ? signed_val(inB[i], bwB)
                                                   : inB[i]);
      }
    }
    else
    {
      if (signed_arithmetic && signed_B)
      {
        xt->s_extend(dim2 * dim3, inB, tmpB, bwB, bwB + extra_bits, msbB);
      }
      else
      {
        xt->z_extend(dim2 * dim3, inB, tmpB, bwB, bwB + extra_bits, msbB);
      }
    }
    memcpy(tmpA, inA, dim1 * dim2 * sizeof(uint64_t));
    bwB = bwB + extra_bits;
  }
  bwC = bwC + extra_bits;
  uint64_t maskA = (bwA == 64 ? -1 : ((1ULL << bwA) - 1));
  uint64_t maskB = (bwB == 64 ? -1 : ((1ULL << bwB) - 1));
  uint64_t maskC = (bwC == 64 ? -1 : ((1ULL << bwC) - 1));
  uint64_t pow_A = 1ULL << bwA;
  uint64_t pow_B = 1ULL << bwB;
  uint64_t pow_A_2 = (1ULL << (bwA - 1));
  uint64_t pow_B_2 = (1ULL << (bwB - 1));
  int32_t dim;
  if (accumulate)
    dim = dim1 * dim3;
  else
    dim = dim1 * dim2 * dim3;

  for (int i = 0; i < dim1 * dim2; i++)
  {
    if (signed_arithmetic)
    {
      if (mode == MultMode::Alice_has_A)
      {
        tmpA[i] = (party == ALICE ? tmpA[i] + pow_A_2 : 0) & maskA;
      }
      else if (mode == MultMode::Bob_has_A)
      {
        tmpA[i] = (party == BOB ? tmpA[i] + pow_A_2 : 0) & maskA;
      }
      else
      {
        tmpA[i] = (tmpA[i] + (party == BOB ? pow_A_2 : 0)) & maskA;
      }
    }
    else
    {
      tmpA[i] = tmpA[i] & maskA;
    }
  }
  for (int i = 0; i < dim2 * dim3; i++)
  {
    if (signed_arithmetic && signed_B)
    {
      if (mode == MultMode::Alice_has_B)
      {
        tmpB[i] = (party == ALICE ? tmpB[i] + pow_B_2 : 0) & maskB;
      }
      else if (mode == MultMode::Bob_has_B)
      {
        tmpB[i] = (party == BOB ? tmpB[i] + pow_B_2 : 0) & maskB;
      }
      else
      {
        tmpB[i] = (tmpB[i] + (party == BOB ? pow_B_2 : 0)) & maskB;
      }
    }
    else
    {
      tmpB[i] = tmpB[i] & maskB;
    }
  }
  /* print_share(tmpA, bwA, dim1*dim2); */
  /* print_share(tmpB, bwB, dim2*dim3); */
  /* print_vec(this, tmpA, bwA, dim1*dim2); */
  /* print_vec(this, tmpB, bwB, dim2*dim3); */
  uint64_t *cross_terms = new uint64_t[dim];
  matmul_cross_terms(dim1, dim2, dim3, tmpA, tmpB, cross_terms, bwA, bwB, bwC,
                     accumulate, mode);
  /* print_vec(this, cross_terms, bwC, dim); */

  uint64_t *local_terms = new uint64_t[dim];
  if (party == ALICE &&
      (mode == MultMode::Alice_has_A || mode == MultMode::Alice_has_B))
  {
    matmul_cleartext(dim1, dim2, dim3, tmpA, tmpB, local_terms, accumulate);
  }
  else if (party == BOB &&
           (mode == MultMode::Bob_has_A || mode == MultMode::Bob_has_B))
  {
    matmul_cleartext(dim1, dim2, dim3, tmpA, tmpB, local_terms, accumulate);
  }
  else if (mode == MultMode::None)
  {
    matmul_cleartext(dim1, dim2, dim3, tmpA, tmpB, local_terms, accumulate);
  }
  else
  {
    memset(local_terms, 0, dim * sizeof(uint64_t));
  }
  /* print_vec(this, local_terms, bwC, dim); */

  uint8_t *wA = new uint8_t[dim1 * dim2];
  uint8_t *wB = new uint8_t[dim2 * dim3];
  uint64_t *wA_B = new uint64_t[dim];
  uint64_t *wB_A = new uint64_t[dim];

  if (bwC > bwA)
  {
    if (mode == MultMode::Alice_has_A || mode == MultMode::Bob_has_A)
    {
      memset(wA, 0, dim1 * dim2 * sizeof(uint8_t));
      memset(wA_B, 0, dim * sizeof(uint64_t));
    }
    else
    {
      if (msbA != nullptr)
      {
        uint8_t *tmp_msbA = new uint8_t[dim1 * dim2];
        if (signed_arithmetic)
        {
          for (int i = 0; i < dim1 * dim2; i++)
          {
            tmp_msbA[i] = (party == ALICE ? msbA[i] ^ 1 : msbA[i]);
          }
        }
        else
        {
          for (int i = 0; i < dim1 * dim2; i++)
          {
            tmp_msbA[i] = (sender_A && (extra_bits > 0) ? 0 : msbA[i]);
          }
        }
        aux->MSB_to_Wrap(tmpA, tmp_msbA, wA, dim1 * dim2, bwA);
        delete[] tmp_msbA;
      }
      else
      {
        aux->wrap_computation(tmpA, wA, dim1 * dim2, bwA);
      }
      uint64_t *wA64 = new uint64_t[dim1 * dim2];
      for (int i = 0; i < dim1 * dim2; i++)
      {
        wA64[i] = uint64_t(wA[i]);
      }
      matmul_multiplexer(dim1, dim2, dim3, wA64, tmpB, wA_B, 1, bwB, bwC - bwA,
                         accumulate, mode);
      delete[] wA64;
    }
  }
  if (bwC > bwB)
  {
    if (mode == MultMode::Alice_has_B || mode == MultMode::Bob_has_B)
    {
      memset(wB, 0, dim2 * dim3 * sizeof(uint8_t));
      memset(wB_A, 0, dim * sizeof(uint64_t));
    }
    else
    {
      if (msbB != nullptr)
      {
        uint8_t *tmp_msbB = new uint8_t[dim2 * dim3];
        if (signed_arithmetic && signed_B)
        {
          for (int i = 0; i < dim2 * dim3; i++)
          {
            tmp_msbB[i] = (party == ALICE ? msbB[i] ^ 1 : msbB[i]);
          }
        }
        else
        {
          for (int i = 0; i < dim2 * dim3; i++)
          {
            tmp_msbB[i] = (sender_B && (extra_bits > 0) ? 0 : msbB[i]);
          }
        }
        aux->MSB_to_Wrap(tmpB, tmp_msbB, wB, dim2 * dim3, bwB);
        delete[] tmp_msbB;
      }
      else
      {
        aux->wrap_computation(tmpB, wB, dim2 * dim3, bwB);
      }
      uint64_t *wB64 = new uint64_t[dim2 * dim3];
      for (int i = 0; i < dim2 * dim3; i++)
      {
        wB64[i] = uint64_t(wB[i]);
      }
      matmul_multiplexer(dim1, dim2, dim3, tmpA, wB64, wB_A, bwA, 1, bwC - bwB,
                         accumulate, mode);
      delete[] wB64;
    }
  }
  /* print_vec(this, wA_B, bwC - bwA, dim); */
  /* print_vec(this, wB_A, bwC - bwB, dim); */

  uint64_t *tmpC = new uint64_t[dim];
  int inner_loop_size = (accumulate ? dim2 : 1);
  for (int i = 0; i < dim; i++)
  {
    tmpC[i] =
        (local_terms[i] + cross_terms[i] - pow_A * wA_B[i] - pow_B * wB_A[i]) &
        maskC;
    if (signed_arithmetic)
    {
      for (int j = 0; j < inner_loop_size; j++)
      {
        int idx = (accumulate ? i : i / dim2);
        int common_idx = (accumulate ? j : i % dim2);
        int row_idx = idx / dim3;
        int col_idx = idx % dim3;
        int A_idx = row_idx * dim2 + common_idx;
        int B_idx = common_idx * dim3 + col_idx;
        if (signed_B)
        {
          if (party == ALICE)
          {
            tmpC[i] = (tmpC[i] - pow_A_2 * (tmpB[B_idx] - pow_B * wB[B_idx]) -
                       pow_B_2 * (tmpA[A_idx] - pow_A * wA[A_idx])) &
                      maskC;
          }
          else
          { // party == BOB
            tmpC[i] = (tmpC[i] - pow_A_2 * (tmpB[B_idx] - pow_B * wB[B_idx]) -
                       pow_B_2 * (tmpA[A_idx] - pow_A * wA[A_idx]) +
                       pow_A_2 * pow_B_2) &
                      maskC;
          }
        }
        else
        {
          tmpC[i] =
              (tmpC[i] - pow_A_2 * (tmpB[B_idx] - pow_B * wB[B_idx])) & maskC;
        }
      }
    }
  }
  if (accumulate)
  {
    trunc->truncate_and_reduce(dim1 * dim3, tmpC, outC, extra_bits, bwC);
  }
  else
  {
    memcpy(outC, tmpC, dim * sizeof(uint64_t));
  }
  /* print_vec(this, outC, bwC, dim); */

  delete[] cross_terms;
  delete[] local_terms;
  delete[] wA;
  delete[] wB;
  delete[] wA_B;
  delete[] wB_A;
  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;
}
