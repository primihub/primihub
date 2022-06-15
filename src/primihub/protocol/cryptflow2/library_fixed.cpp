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

#include "library_fixed.h"
#include "globals.h"
#include "library_fixed_common.h"

using namespace std;
using namespace primihub::sci;
using namespace primihub::cryptflow2;

#define START_IDX 0
#define print_vec(vec, bw, N)                     \
  {                                               \
    uint64_t *rec_vec = new uint64_t[N];          \
    reconstruct(N, vec + START_IDX, rec_vec, bw); \
    std::cout << #vec << "_pub:" << std::endl;    \
    for (int i = 0; i < N; i++)                   \
    {                                             \
      std::cout << rec_vec[i] << " ";             \
    }                                             \
    std::cout << std::endl;                       \
  }

void primihub::cryptflow2::initialize()
{
  assert(num_threads <= MAX_THREADS);

  for (int i = 0; i < num_threads; i++)
  {
    iopackArr[i] = new primihub::sci::IOPack(party, port + i, address);
    ioArr[i] = iopackArr[i]->io;
    if (i & 1)
    {
      otpackArr[i] = new OTPack(iopackArr[i], 3 - party);
    }
    else
    {
      otpackArr[i] = new OTPack(iopackArr[i], party);
    }
  }
  io = ioArr[0];
  iopack = iopackArr[0];
  otpack = otpackArr[0];

  for (int i = 0; i < num_threads; i++)
  {
    if (i & 1)
    {
      auxArr[i] = new AuxProtocols(3 - party, iopackArr[i], otpackArr[i]);
      truncationArr[i] = new Truncation(3 - party, iopackArr[i], otpackArr[i]);
      xtArr[i] = new XTProtocol(3 - party, iopackArr[i], otpackArr[i]);
      multArr[i] = new LinearOT(3 - party, iopackArr[i], otpackArr[i]);
      mathArr[i] = new MathFunctions(3 - party, iopackArr[i], otpackArr[i]);
    }
    else
    {
      auxArr[i] = new AuxProtocols(party, iopackArr[i], otpackArr[i]);
      truncationArr[i] = new Truncation(party, iopackArr[i], otpackArr[i]);
      xtArr[i] = new XTProtocol(party, iopackArr[i], otpackArr[i]);
      multArr[i] = new LinearOT(party, iopackArr[i], otpackArr[i]);
      mathArr[i] = new MathFunctions(party, iopackArr[i], otpackArr[i]);
    }
  }
  aux = auxArr[0];
  truncation = truncationArr[0];
  xt = xtArr[0];
  mult = multArr[0];
  math = mathArr[0];

  io->sync();
  num_rounds = iopack->get_rounds();
  st_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_threads; i++)
  {
    auto temp = iopackArr[i]->get_comm();
    comm_threads[i] = temp;
  }
}

void primihub::cryptflow2::finalize()
{
  auto end_time = chrono::high_resolution_clock::now();
  auto execTimeInMilliSec =
      chrono::duration_cast<chrono::milliseconds>(end_time - st_time)
          .count();
  uint64_t totalComm = 0;
  for (int i = 0; i < num_threads; i++)
  {
    auto temp = iopackArr[i]->get_comm();
    totalComm += (temp - comm_threads[i]);
  }
  uint64_t totalCommClient;
  std::cout << "------------------------------------------------------\n";
  std::cout << "------------------------------------------------------\n";
  std::cout << "------------------------------------------------------\n";
  std::cout << "Total time taken = " << execTimeInMilliSec
            << " milliseconds.\n";
  std::cout << "Total data sent = " << (totalComm / (1.0 * (1ULL << 20)))
            << " MiB." << std::endl;
  std::cout << "Number of rounds = " << iopack->get_rounds() - num_rounds
            << std::endl;
  if (party == SERVER)
  {
    io->recv_data(&totalCommClient, sizeof(uint64_t));
    std::cout << "Total comm (sent+received) = "
              << ((totalComm + totalCommClient) / (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
  }
  else if (party == CLIENT)
  {
    io->send_data(&totalComm, sizeof(uint64_t));
    std::cout << "Total comm (sent+received) = (see SERVER OUTPUT)"
              << std::endl;
  }
  std::cout << "------------------------------------------------------\n";

#ifdef LOG_LAYERWISE
  std::cout << "Total time in MatAdd = " << (MatAddTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in BatchNorm = " << (BatchNormInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in MatMul = " << (MatMulTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in Conv = " << (ConvTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in ReLU = " << (ReluTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in MatAddBroadCast = "
            << (MatAddBroadCastTimeInMilliSec / 1000.0) << " seconds."
            << std::endl;
  std::cout << "Total time in MulCir = " << (MulCirTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in ScalarMul = "
            << (ScalarMulTimeInMilliSec / 1000.0) << " seconds." << std::endl;
  std::cout << "Total time in Sigmoid = " << (SigmoidTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in Tanh = " << (TanhTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in Sqrt = " << (SqrtTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "Total time in NormaliseL2 = "
            << (NormaliseL2TimeInMilliSec / 1000.0) << " seconds." << std::endl;
  std::cout << "Total time in ArgMax = " << (ArgMaxTimeInMilliSec / 1000.0)
            << " seconds." << std::endl;
  std::cout << "------------------------------------------------------\n";
  std::cout << "MatAdd data sent = "
            << ((MatAddCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "BatchNorm data sent = "
            << ((BatchNormCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "MatMul data sent = "
            << ((MatMulCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "Conv data sent = " << ((ConvCommSent) / (1.0 * (1ULL << 20)))
            << " MiB." << std::endl;
  std::cout << "ReLU data sent = " << ((ReluCommSent) / (1.0 * (1ULL << 20)))
            << " MiB." << std::endl;
  std::cout << "MatAddBroadCast data sent = "
            << ((MatAddBroadCastCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "MulCir data sent = "
            << ((MulCirCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "Sigmoid data sent = "
            << ((SigmoidCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "Tanh data sent = " << ((TanhCommSent) / (1.0 * (1ULL << 20)))
            << " MiB." << std::endl;
  std::cout << "Sqrt data sent = " << ((SqrtCommSent) / (1.0 * (1ULL << 20)))
            << " MiB." << std::endl;
  std::cout << "NormaliseL2 data sent = "
            << ((NormaliseL2CommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "ArgMax data sent = "
            << ((ArgMaxCommSent) / (1.0 * (1ULL << 20))) << " MiB."
            << std::endl;
  std::cout << "------------------------------------------------------\n";

  if (party == SERVER)
  {
    uint64_t MatAddCommSentClient = 0;
    uint64_t BatchNormCommSentClient = 0;
    uint64_t MatMulCommSentClient = 0;
    uint64_t ConvCommSentClient = 0;
    uint64_t ReluCommSentClient = 0;
    uint64_t MatAddBroadCastCommSentClient = 0;
    uint64_t MulCirCommSentClient = 0;
    uint64_t ScalarMulCommSentClient = 0;
    uint64_t SigmoidCommSentClient = 0;
    uint64_t TanhCommSentClient = 0;
    uint64_t SqrtCommSentClient = 0;
    uint64_t NormaliseL2CommSentClient = 0;
    uint64_t ArgMaxCommSentClient = 0;
    io->recv_data(&MatAddCommSentClient, sizeof(uint64_t));
    io->recv_data(&BatchNormCommSentClient, sizeof(uint64_t));
    io->recv_data(&MatMulCommSentClient, sizeof(uint64_t));
    io->recv_data(&ConvCommSentClient, sizeof(uint64_t));
    io->recv_data(&ReluCommSentClient, sizeof(uint64_t));
    io->recv_data(&MatAddBroadCastCommSentClient, sizeof(uint64_t));
    io->recv_data(&MulCirCommSentClient, sizeof(uint64_t));
    io->recv_data(&ScalarMulCommSentClient, sizeof(uint64_t));
    io->recv_data(&SigmoidCommSentClient, sizeof(uint64_t));
    io->recv_data(&TanhCommSentClient, sizeof(uint64_t));
    io->recv_data(&SqrtCommSentClient, sizeof(uint64_t));
    io->recv_data(&NormaliseL2CommSentClient, sizeof(uint64_t));
    io->recv_data(&ArgMaxCommSentClient, sizeof(uint64_t));
    std::cout << "MatAdd data (sent+received) = "
              << ((MatAddCommSent + MatAddCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "BatchNorm data (sent+received) = "
              << ((BatchNormCommSent + BatchNormCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "MatMul data (sent+received) = "
              << ((MatMulCommSent + MatMulCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "Conv data (sent+received) = "
              << ((ConvCommSent + ConvCommSentClient) / (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "ReLU data (sent+received) = "
              << ((ReluCommSent + ReluCommSentClient) / (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "MatAddBroadCast data (sent+received) = "
              << ((MatAddBroadCastCommSent + MatAddBroadCastCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "MulCir data (sent+received) = "
              << ((MulCirCommSent + MulCirCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "ScalarMul data (sent+received) = "
              << ((ScalarMulCommSent + ScalarMulCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "Sigmoid data (sent+received) = "
              << ((SigmoidCommSent + SigmoidCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "Sqrt data (sent+received) = "
              << ((SqrtCommSent + SqrtCommSentClient) / (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "Tanh data (sent+received) = "
              << ((TanhCommSent + TanhCommSentClient) / (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "NormaliseL2 data (sent+received) = "
              << ((NormaliseL2CommSent + NormaliseL2CommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
    std::cout << "ArgMax data (sent+received) = "
              << ((ArgMaxCommSent + ArgMaxCommSentClient) /
                  (1.0 * (1ULL << 20)))
              << " MiB." << std::endl;
  }
  else if (party == CLIENT)
  {
    io->send_data(&MatAddCommSent, sizeof(uint64_t));
    io->send_data(&BatchNormCommSent, sizeof(uint64_t));
    io->send_data(&MatMulCommSent, sizeof(uint64_t));
    io->send_data(&ConvCommSent, sizeof(uint64_t));
    io->send_data(&ReluCommSent, sizeof(uint64_t));
    io->send_data(&MatAddBroadCastCommSent, sizeof(uint64_t));
    io->send_data(&MulCirCommSent, sizeof(uint64_t));
    io->send_data(&ScalarMulCommSent, sizeof(uint64_t));
    io->send_data(&SigmoidCommSent, sizeof(uint64_t));
    io->send_data(&TanhCommSent, sizeof(uint64_t));
    io->send_data(&SqrtCommSent, sizeof(uint64_t));
    io->send_data(&NormaliseL2CommSent, sizeof(uint64_t));
    io->send_data(&ArgMaxCommSent, sizeof(uint64_t));
  }
#endif

  for (int i = 0; i < num_threads; i++)
  {
    delete ioArr[i];
    delete otpackArr[i];
    delete auxArr[i];
    delete xtArr[i];
    delete truncationArr[i];
    delete multArr[i];
    delete mathArr[i];
  }
}

void primihub::cryptflow2::reconstruct(int64_t *A, int64_t *B, int32_t I, int32_t J, int bwA)
{
  reconstruct(I * J, (uint64_t *)A, (uint64_t *)B, bwA);
  for (int i = 0; i < I * J; i++)
  {
    B[i] = signed_val((uint64_t)B[i], bwA);
  }
}

void primihub::cryptflow2::reconstruct(int dim, uint64_t *x, uint64_t *y, int bw_x)
{
  uint64_t mask = (bw_x == 64 ? -1 : ((1ULL << bw_x) - 1));
  if (party == primihub::sci::ALICE)
  {
    io->send_data(x, dim * sizeof(uint64_t));
    for (int i = 0; i < dim; i++)
    {
      y[i] = 0;
    }
  }
  else
  {
    io->recv_data(y, dim * sizeof(uint64_t));
    for (int i = 0; i < dim; i++)
    {
      y[i] = (y[i] + x[i]) & mask;
    }
  }
}

void primihub::cryptflow2::typecast_to_uint64(int64_t *A, uint64_t *A64, int32_t I, int32_t J,
                        int32_t bwA)
{
  uint64_t mask = (bwA == 64 ? -1 : ((1ULL << bwA) - 1));

  for (int i = 0; i < I * J; i++)
  {
    A64[i] = uint64_t(A[i]) & mask;
  }
  return;
}

void primihub::cryptflow2::typecast_from_uint64(uint64_t *A64, int64_t *A, int32_t I, int32_t J,
                          int bwA)
{
  for (int i = 0; i < I * J; i++)
  {
    if (bwA <= 8)
      A[i] = int64_t(int8_t(A64[i]));
    else if (bwA <= 16)
      A[i] = int64_t(int16_t(A64[i]));
    else if (bwA <= 32)
      A[i] = int64_t(int32_t(A64[i]));
    else
      A[i] = int64_t(A64[i]);
  }
  return;
}

void reconstruct(uint64_t *A, int64_t *B, int32_t I, int32_t J, int bwA)
{
  reconstruct(I * J, A, (uint64_t *)B, bwA);
  for (int i = 0; i < I * J; i++)
  {
    B[i] = signed_val((uint64_t)B[i], bwA);
  }
}

void primihub::cryptflow2::AdjustScaleShr(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
                    int32_t scale)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  int32_t dim = I * J;
#ifdef DIV_RESCALING
  truncation->div_pow2(dim, A, B, scale, bwA, true);
#else
  truncation->truncate(dim, A, B, scale, bwA, true);
#endif

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MatAddTimeInMilliSec += temp;
  std::cout << "Time in sec for current AdjustScaleShr = " << (temp / 1000.0)
            << std::endl;
  MatAddCommSent += curComm;
#endif
}

void primihub::cryptflow2::MatAdd(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
            int32_t bwA, int32_t bwB, int32_t bwC, int32_t bwTemp, int32_t shrA,
            int32_t shrB, int32_t shrC, int32_t demote, bool subroutine)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  assert(bwTemp <= 64);
  assert(bwA <= bwTemp);
  assert(bwB <= bwTemp);
  assert(bwC <= bwTemp);

  int32_t dim = I * J;
  uint64_t maskTemp = (bwTemp == 64 ? -1 : ((1ULL << bwTemp) - 1));

  uint64_t *tmpA = new uint64_t[dim];
  uint64_t *tmpB = new uint64_t[dim];
  uint64_t *tmpC = new uint64_t[dim];

  xt->s_extend(dim, A, tmpA, bwA, bwTemp);
  xt->s_extend(dim, B, tmpB, bwB, bwTemp);

#ifdef DIV_RESCALING
  truncation->div_pow2(dim, tmpA, A, shrA + shrC, bwTemp, true);
  truncation->div_pow2(dim, tmpB, B, shrB + shrC, bwTemp, true);
#else
  truncation->truncate(dim, tmpA, A, shrA + shrC, bwTemp, true);
  truncation->truncate(dim, tmpB, B, shrB + shrC, bwTemp, true);
#endif

  for (int i = 0; i < dim; i++)
  {
    tmpC[i] = (A[i] + B[i]) & maskTemp;
  }

#ifdef DIV_RESCALING
  truncation->div_pow2(dim, tmpC, C, demote, bwTemp, true);
#else
  truncation->truncate(dim, tmpC, C, demote, bwTemp, true);
#endif
  aux->reduce(dim, C, C, bwTemp, bwC);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  if (!subroutine)
  {
    MatAddTimeInMilliSec += temp;
    std::cout << "Time in sec for current MatAdd = " << (temp / 1000.0)
              << std::endl;
    MatAddCommSent += curComm;
  }
#endif
}

void primihub::cryptflow2::MatAddBroadCast(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I,
                     int32_t J, int32_t bwA, int32_t bwB, int32_t bwC,
                     int32_t bwTemp, int32_t shrA, int32_t shrB, int32_t shrC,
                     int32_t demote, bool scalar_A)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  int32_t dim = I * J;

  uint64_t tmp;
  if (scalar_A)
  {
    if (party == BOB)
      A[0] = 0; // assert(A[0] == 0);
    uint64_t *tmpA = new uint64_t[dim];
    for (int i = 0; i < dim; i++)
    {
#ifndef DIV_RESCALING
      tmpA[i] = signed_val(A[0], bwA) >> (shrA + shrC);
#else
      tmpA[i] = signed_val(A[0], bwA) / (1LL << (shrA + shrC));
#endif
    }
    MatAdd(tmpA, B, C, I, J, bwTemp, bwB, bwC, bwTemp, 0, shrB + shrC, 0,
           demote, true);

    delete[] tmpA;
  }
  else
  {
    if (party == BOB)
      B[0] = 0; // assert(B[0] == 0);
    uint64_t *tmpB = new uint64_t[dim];
    for (int i = 0; i < dim; i++)
    {
#ifndef DIV_RESCALING
      tmpB[i] = signed_val(B[0], bwB) >> (shrB + shrC);
#else
      tmpB[i] = signed_val(B[0], bwB) / (1LL << (shrB + shrC));
#endif
    }
    MatAdd(A, tmpB, C, I, J, bwA, bwTemp, bwC, bwTemp, shrA + shrC, 0, 0,
           demote, true);

    delete[] tmpB;
  }

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  MatAddBroadCastTimeInMilliSec += temp;
  std::cout << "Time in sec for current MatAddBroadCast = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MatAddBroadCastCommSent += curComm;
#endif
}

void primihub::cryptflow2::AddOrSubCir(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
                 int32_t bwA, int32_t bwB, int32_t bwC, int32_t bwTemp,
                 int32_t shrA, int32_t shrB, int32_t shrC, bool add,
                 int32_t demote)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  int32_t dim = I * J;

  uint64_t *tmpB = new uint64_t[dim];

  xt->s_extend(J, B, tmpB, bwB, bwTemp);
#ifdef DIV_RESCALING
  truncation->div_pow2(J, tmpB, B, shrB + shrC, bwTemp, true);
#else
  truncation->truncate(J, tmpB, B, shrB + shrC, bwTemp, true);
#endif
  for (int i = 0; i < I; i++)
  {
    for (int j = 0; j < J; j++)
    {
      if (add)
      {
        tmpB[i * J + j] = B[j];
      }
      else
      {
        tmpB[i * J + j] = (-1 * B[j]);
      }
    }
  }
  MatAdd(A, tmpB, C, I, J, bwA, bwTemp, bwC, bwTemp, shrA + shrC, 0, 0, demote,
         true);

  delete[] tmpB;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  MatAddTimeInMilliSec += temp;
  std::cout << "Time in sec for current AddOrSubCir = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MatAddCommSent += curComm;
#endif
}

void primihub::cryptflow2::ScalarMul(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
               int32_t bwA, int32_t bwB, int32_t bwTemp, int32_t bwC,
               int32_t shrA, int32_t shrB, int32_t demote)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  int32_t shift = shrA + shrB + demote;

  uint64_t maskTemp = (bwTemp == 64 ? -1 : ((1ULL << bwTemp) - 1));
  uint64_t *tmpC = new uint64_t[I * J];

  xt->s_extend(I * J, B, tmpC, bwB, bwTemp);
  for (int i = 0; i < I * J; i++)
  {
    C[i] = (tmpC[i] * A[0]) & maskTemp;
  }
#ifdef DIV_RESCALING
  truncation->div_pow2(I * J, C, tmpC, shift, bwTemp, true);
  aux->reduce(I * J, tmpC, C, bwTemp, bwC);
#else
  if ((bwTemp - bwC) >= shift)
  {
    truncation->truncate_and_reduce(I * J, C, tmpC, shift, bwTemp);
    aux->reduce(I * J, tmpC, C, bwTemp - shift, bwC);
  }
  else
  {
    truncation->truncate(I * J, C, tmpC, shift, bwTemp, true);
    aux->reduce(I * J, tmpC, C, bwTemp, bwC);
  }
#endif

  delete[] tmpC;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  ScalarMulTimeInMilliSec += temp;
  std::cout << "Time in sec for current ScalarMul = " << (temp / 1000.0)
            << std::endl;
  ScalarMulCommSent += curComm;
#endif
}

void MulCir_thread(int32_t tid, uint64_t *A, uint64_t *B, uint64_t *C,
                   int32_t dim, int32_t bwA, int32_t bwB, int32_t bwC,
                   int32_t bwTemp, int32_t shrA, int32_t shrB, int32_t demote)
{
  int32_t shift = shrA + shrB + demote;

  uint64_t *tmpC = new uint64_t[dim];

  multArr[tid]->hadamard_product(dim, A, B, C, bwA, bwB, bwA + bwB, true, true,
                                 MultMode::None);
#ifdef DIV_RESCALING
  truncationArr[tid]->div_pow2(dim, C, tmpC, shift, bwA + bwB, true);
  auxArr[tid]->reduce(dim, tmpC, C, bwA + bwB, bwC);
#else
  if ((bwA + bwB - bwC) >= shift)
  {
    truncationArr[tid]->truncate_and_reduce(dim, C, tmpC, shift, bwA + bwB);
    auxArr[tid]->reduce(dim, tmpC, C, bwA + bwB - shift, bwC);
  }
  else
  {
    truncationArr[tid]->truncate(dim, C, tmpC, shift, bwA + bwB, true);
    auxArr[tid]->reduce(dim, tmpC, C, bwA + bwB, bwC);
  }
#endif
  delete[] tmpC;
}

void primihub::cryptflow2::MulCir(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
            int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, uint64_t *A,
            uint64_t *B, uint64_t *C)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". MulCir (" << I << " x " << J << ")" << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);

  int min_chunk_size = THREADING_MIN_CHUNK_SIZE;
  std::vector<int> chunks_per_thread =
      divide_instances(::num_threads, I * J, min_chunk_size);

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    threads[i] = std::thread(MulCir_thread, i, A + offset, B + offset,
                             C + offset, chunks_per_thread[i], bwA, bwB, bwC,
                             bwTemp, shiftA, shiftB, shift_demote);
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MulCirTimeInMilliSec += temp;
  std::cout << "Time in sec for current MulCir = " << (temp / 1000.0)
            << std::endl;
  MulCirCommSent += curComm;
#endif

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MulCir(recA, recB, correctC, I, J, shrA, shrB, demote);

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != signed_val(correctC[i], bwC))
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recC[i]) << "\t" <<
      // int(signed_val(correctC[i],bwC)) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MulCir Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MulCir Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
}

void MatMul_thread(int32_t tid, uint64_t *A, uint64_t *B, uint64_t *C,
                   int32_t I, int32_t K, int32_t J, int32_t bwA, int32_t bwB,
                   int32_t bwC, int32_t bwTemp, int32_t shrA, int32_t shrB,
                   int32_t H1, int32_t demote,
                   MultMode mode = MultMode::Alice_has_B)
{
  assert(bwA + bwB >= bwC);
  int32_t depth = ceil(log2(K));
  int32_t shift = shrA + shrB + demote + H1 - depth;

  uint64_t maskC = (bwC == 64 ? -1 : ((1ULL << bwC) - 1));
  uint64_t *tmpC = new uint64_t[I * J];

  multArr[tid]->matrix_multiplication(I, K, J, A, B, C, bwA, bwB, bwA + bwB,
                                      true, true, true, mode);
  if (shift <= 0)
  {
    for (int i = 0; i < I * J; i++)
    {
      tmpC[i] = (C[i] << (-1 * shift)) & maskC;
    }
    auxArr[tid]->reduce(I * J, tmpC, C, bwA + bwB, bwC);
#ifdef DIV_RESCALING
  }
  else
  {
    truncationArr[tid]->div_pow2(I * J, C, tmpC, shift, bwA + bwB, true);
    auxArr[tid]->reduce(I * J, tmpC, C, bwA + bwB, bwC);
  }
#else
  }
  else if ((bwA + bwB - bwC) >= shift)
  {
    truncationArr[tid]->truncate_and_reduce(I * J, C, tmpC, shift, bwA + bwB);
    auxArr[tid]->reduce(I * J, tmpC, C, bwA + bwB - shift, bwC);
  }
  else
  {
    truncationArr[tid]->truncate(I * J, C, tmpC, shift, bwA + bwB, true);
    auxArr[tid]->reduce(I * J, tmpC, C, bwA + bwB, bwC);
  }
#endif

  delete[] tmpC;
}

void primihub::cryptflow2::MatMul(int64_t I, int64_t K, int64_t J, int64_t shrA, int64_t shrB,
            int64_t H1, int64_t H2, int64_t demote, int32_t bwA, int32_t bwB,
            int32_t bwTemp, int32_t bwC, uint64_t *A, uint64_t *B, uint64_t *C,
            uint64_t *tmp, bool verbose)
{
#ifdef LOG_LAYERWISE
  if (verbose)
    std::cout << ctr++ << ". MatMul (" << I << " x " << K << " x " << J << ")"
              << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  if (party == CLIENT)
  {
    for (int i = 0; i < K * J; i++)
    {
      assert(B[i] == 0 &&
             "The multiplication mode for MatMul is set assuming the server "
             "has the model weights (B) in the clear, and therefore, the "
             "client's share for the weights must be zero. If the weights "
             "should also be secret-shared, then change the MultMode in this "
             "function to MultMode::None");
    }
  }

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);

  int min_chunk_size = ceil(THREADING_MIN_CHUNK_SIZE / double(K * J));
  std::vector<int> chunks_per_thread =
      divide_instances(::num_threads, I, min_chunk_size);

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  // cout << "lnum_threads: " << lnum_threads << endl;
  // cout << "chunks[0]: " << chunks_per_thread[0] << endl;
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    MultMode mode = (i & 1 ? MultMode::Bob_has_B : MultMode::Alice_has_B);
    threads[i] =
        std::thread(MatMul_thread, i, A + (K * offset), B, C + (J * offset),
                    chunks_per_thread[i], K, J, bwA, bwB, bwC, bwTemp, shiftA,
                    shiftB, H1, shift_demote, mode);
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }

  if (!verbose)
    return;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MatMulCommSent += curComm;
  MatMulTimeInMilliSec += temp;
  std::cout << "Time in sec for current MatMul = " << (temp / 1000.0)
            << std::endl;
#endif
}

void primihub::cryptflow2::MatMul(int64_t I, int64_t K, int64_t J, int64_t shrA, int64_t shrB,
            int64_t H1, int64_t H2, int64_t demote, int32_t bwA, int32_t bwB,
            int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B, int64_t *C,
            int64_t *tmp, bool verbose)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatMul(A, B, C, tmp, I, K, J, shrA, shrB, H1, H2, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }

#else
  uint64_t *tmpA = new uint64_t[I * K];
  typecast_to_uint64(A, tmpA, I, K, bwA);
  uint64_t *tmpB = new uint64_t[K * J];
  typecast_to_uint64(B, tmpB, K, J, bwB);
  uint64_t *tmpC = new uint64_t[I * J];

  MatMul(I, K, J, shrA, shrB, H1, H2, demote, bwA, bwB, bwTemp, bwC, tmpA, tmpB,
         tmpC, nullptr, verbose);

  typecast_from_uint64(tmpC, C, I, J, bwC);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;
#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * K];
  int64_t *recB = new int64_t[K * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, K, bwA);
  reconstruct(B, recB, K, J, bwB);
  reconstruct(C, recC, I, J, bwC);

  cleartext_MatMul(recA, recB, correctC, tmp, I, K, J, shrA, shrB, H1, H2,
                   demote);

  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }
  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
        if (verbose)
          std::cout << i << "\t" << int(recC[i]) << "\t" << int(correctC[i])
                    << std::endl;
      }
      // if (verbose) std::cout << i << "\t" << int(recC[i]) << "\t" <<
      // int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatMul Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MatMul Output Mismatch" << RESET << std::endl;
    /*
    if (!pass) {
        for(int i = 0; i < I*J; i++) {
            if(recC[i] != correctC[i]) {
                std::cout << i << "\t" << int(recC[i]) << "\t" <<
    int(correctC[i]) << std::endl;
            }
        }
    }
    */
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void Sigmoid_thread(int32_t tid, uint64_t *A, uint64_t *B, int32_t dim,
                    int32_t bwA, int32_t bwB, int32_t sA, int32_t sB)
{
  mathArr[tid]->sigmoid(dim, A, B, bwA, bwB, sA, sB);
}

void primihub::cryptflow2::Sigmoid(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
             int64_t bwA, int64_t bwB, uint64_t *A, uint64_t *B)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Sigmoid (" << I << " x " << J << ")" << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  int32_t s_A = log2(scale_in);
  int32_t s_B = log2(scale_out);

  int min_chunk_size = THREADING_MIN_CHUNK_SIZE;
  std::vector<int> chunks_per_thread =
      divide_instances(::num_threads, I * J, min_chunk_size);

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    threads[i] = std::thread(Sigmoid_thread, i, A + offset, B + offset,
                             chunks_per_thread[i], bwA, bwB, s_A, s_B);
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  SigmoidTimeInMilliSec += temp;
  std::cout << "Time in sec for current Sigmoid = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  SigmoidCommSent += curComm;
#endif

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *correctB = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwA);
  cleartext_Sigmoid(recA, I, J, scale_in, scale_out, bwA, bwA, correctB);

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
        std::cout << recA[i] / double(1LL << s_A) << "\t"
                  << recB[i] / double(1LL << s_B) << "\t"
                  << correctB[i] / double(1LL << s_B) << std::endl;
      }
      // std::cout << recA[i]/double(1LL << s_A) << "\t" << recB[i]/double(1LL
      // << s_B) << "\t" << correctB[i]/double(1LL << s_B) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "Sigmoid Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Sigmoid Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
}

void TanH_thread(int32_t tid, uint64_t *A, uint64_t *B, int32_t dim,
                 int32_t bwA, int32_t bwB, int32_t sA, int32_t sB)
{
  mathArr[tid]->tanh(dim, A, B, bwA, bwB, sA, sB);
}

void primihub::cryptflow2::TanH(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, uint64_t *A, uint64_t *B)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". TanH (" << I << " x " << J << ")" << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  int32_t s_A = log2(scale_in);
  int32_t s_B = log2(scale_out);

  int min_chunk_size = THREADING_MIN_CHUNK_SIZE;
  std::vector<int> chunks_per_thread =
      divide_instances(::num_threads, I * J, min_chunk_size);

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    threads[i] = std::thread(TanH_thread, i, A + offset, B + offset,
                             chunks_per_thread[i], bwA, bwB, s_A, s_B);
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }
#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  TanhTimeInMilliSec += temp;
  std::cout << "Time in sec for current TanH = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  TanhCommSent += curComm;
#endif

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *correctB = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwA);
  cleartext_TanH(recA, I, J, scale_in, scale_out, bwA, bwA, correctB);

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
      // std::cout << recA[i]/double(1LL << s_A) << "\t" << recB[i]/double(1LL
      // << s_B) << "\t" << correctB[i]/double(1LL << s_B) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "TanH Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "TanH Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
}

void Sqrt_thread(int32_t tid, uint64_t *A, uint64_t *B, int32_t dim,
                 int32_t bwA, int32_t bwB, int32_t sA, int32_t sB,
                 bool inverse)
{
  mathArr[tid]->sqrt(dim, A, B, bwA, bwB, sA, sB, inverse);
}

void primihub::cryptflow2::Sqrt(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, bool inverse, uint64_t *A, uint64_t *B)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Sqrt (" << I << " x " << J << ")" << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  int32_t s_A = log2(scale_in);
  int32_t s_B = log2(scale_out);

  int min_chunk_size = THREADING_MIN_CHUNK_SIZE;
  std::vector<int> chunks_per_thread =
      divide_instances(::num_threads, I * J, min_chunk_size);

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    threads[i] = std::thread(Sqrt_thread, i, A + offset, B + offset,
                             chunks_per_thread[i], bwA, bwB, s_A, s_B, inverse);
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }
#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  SqrtTimeInMilliSec += temp;
  std::cout << "Time in sec for current Sqrt = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  SqrtCommSent += curComm;
#endif

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *correctB = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwB);
  cleartext_sqrt(recA, I, J, scale_in, scale_out, bwA, bwB, correctB, inverse);

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
    }
    if (pass == true)
      std::cout << GREEN << "Sqrt Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Sqrt Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
}

void primihub::cryptflow2::Exp(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
         int32_t bwB, int32_t sA, int32_t sB)
{
  math->lookup_table_exp(I * J, A, B, bwA, bwB, sA, sB);
}

void primihub::cryptflow2::Div(uint64_t *A, uint64_t *B, uint64_t *C, int32_t I, int32_t J,
         int32_t bwA, int32_t bwB, int32_t bwC, int32_t sA, int32_t sB,
         int32_t sC)
{
  math->div(I * J, A, B, C, bwA, bwB, bwC, sA, sB, sC, true, false);
}

void primihub::cryptflow2::ArgMax(uint64_t *A, int32_t I, int32_t J, int32_t bwA, int32_t bw_index,
            uint64_t *index)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  argmax = new ArgMaxProtocol<uint64_t>(party, RING, iopack, bwA, MILL_PARAM,
                                        0, otpack);

  argmax->ArgMaxMPC(I * J, A, index, false, nullptr);
  if (bw_index > bwA)
  {
    xt->z_extend(1, index, index, bwA, bw_index);
  }

  delete argmax;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  ArgMaxTimeInMilliSec += temp;
  std::cout << "Time in sec for current ArgMax = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  ArgMaxCommSent += curComm;
#endif
}

void primihub::cryptflow2::MaxPool2D(uint64_t *A, int32_t I, int32_t J, int32_t bwA, int32_t bwB,
               uint64_t *B)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  maxpool = new MaxPoolProtocol<uint64_t>(party, RING, iopack, bwA,
                                          MILL_PARAM, 0, otpack);
  uint64_t *B_temp = new uint64_t[I];
  maxpool->funcMaxMPC(I, J, A, B_temp, nullptr);
  if (bwB > bwA)
  {
    xt->z_extend(I, B_temp, B, bwA, bwB);
  }
  else
  {
    memcpy(B, B_temp, sizeof(uint64_t) * I);
  }
  delete[] B_temp;
  delete maxpool;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  MaxpoolTimeInMilliSec += temp;
  std::cout << "Time in sec for current MaxPool = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  MaxpoolCommSent += curComm;
#endif
}

void GroupedMatMul_thread(int32_t tid, uint64_t *A, uint64_t *B, uint64_t *C,
                          int32_t I, int32_t K, int32_t J, int32_t G,
                          int32_t bwA, int32_t bwB, int32_t bwC, int32_t bwTemp,
                          int32_t shrA, int32_t shrB, int32_t H1, int32_t H2,
                          int32_t demote,
                          MultMode mode = MultMode::Alice_has_A)
{
  // assert(bwA + bwB >= bwC);
  int32_t depth = H1 + H2;
  int32_t shift = shrA + shrB + demote - H2;
  int out_size = G * I * J;

  for (int32_t g = 0; g < G; g++)
  {
    multArr[tid]->matrix_multiplication(I, K, J, A + (g * I * K),
                                        B + (g * K * J), C + (g * I * J), bwA,
                                        bwB, bwA + bwB, true, true, true, mode);
  }

  uint64_t maskC = (bwC == 64 ? -1 : ((1ULL << bwC) - 1));
  uint64_t *tmpC = new uint64_t[out_size];

  if (shift <= 0)
  {
    if (bwA + bwB < bwC)
    {
      xtArr[tid]->s_extend(out_size, C, tmpC, bwA + bwB, bwC);
    }
    else
    {
      auxArr[tid]->reduce(out_size, C, tmpC, bwA + bwB, bwC);
    }
    for (int i = 0; i < out_size; i++)
    {
      C[i] = (tmpC[i] * (1ULL << (-1 * shift))) & maskC;
    }
#ifdef DIV_RESCALING
  }
  else
  {
    truncationArr[tid]->div_pow2(out_size, C, tmpC, shift, bwA + bwB, true);
    if (bwA + bwB < bwC)
    {
      xtArr[tid]->s_extend(out_size, tmpC, C, bwA + bwB, bwC);
    }
    else
    {
      auxArr[tid]->reduce(out_size, tmpC, C, bwA + bwB, bwC);
    }
  }
#else
  }
  else if ((bwA + bwB - bwC) >= shift)
  {
    truncationArr[tid]->truncate_and_reduce(out_size, C, tmpC, shift,
                                            bwA + bwB);
    if (bwA + bwB < bwC)
    {
      xtArr[tid]->s_extend(out_size, tmpC, C, bwA + bwB - shift, bwC);
    }
    else
    {
      auxArr[tid]->reduce(out_size, tmpC, C, bwA + bwB - shift, bwC);
    }
  }
  else
  {
    truncationArr[tid]->truncate(out_size, C, tmpC, shift, bwA + bwB, true);
    if (bwA + bwB < bwC)
    {
      xtArr[tid]->s_extend(out_size, tmpC, C, bwA + bwB, bwC);
    }
    else
    {
      auxArr[tid]->reduce(out_size, tmpC, C, bwA + bwB, bwC);
    }
  }
#endif

  delete[] tmpC;
}

void primihub::cryptflow2::Convolution(int32_t N, int32_t H, int32_t W, int32_t CIN, int32_t HF,
                 int32_t WF, int32_t CINF, int32_t COUTF, int32_t HOUT,
                 int32_t WOUT, int32_t HPADL, int32_t HPADR, int32_t WPADL,
                 int32_t WPADR, int32_t HSTR, int32_t WSTR, int32_t HDL,
                 int32_t WDL, int32_t G, int32_t bwA, int32_t bwB, int32_t bwC,
                 int32_t bwTemp, int32_t shrA, int32_t shrB, int32_t H1,
                 int32_t H2, int32_t demote, uint64_t *A, uint64_t *B,
                 uint64_t *C)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Convolution (I = (" << N << "x" << H << "x" << W
            << "x" << CIN << "), F = (" << G << "x" << HF << "x" << WF << "x"
            << CINF << "x" << COUTF << "), S = (" << HSTR << "x" << WSTR << "))"
            << std::endl;
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  if (party == CLIENT)
  {
    for (int i = 0; i < G * HF * WF * CINF * COUTF; i++)
    {
      assert(B[i] == 0 &&
             "The multiplication mode for convolution is set assuming the "
             "server has the model weights (B) in the clear, and therefore, "
             "the client's share for the weights must be zero. If the weights "
             "should also be secret-shared, then change the MultMode in this "
             "function to MultMode::None");
    }
  }
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);

  int32_t reshaped_image_size = HF * WF * CINF * N * HOUT * WOUT;

  uint64_t *Image = new uint64_t[G * reshaped_image_size];
  uint64_t *Filter = new uint64_t[G * COUTF * HF * WF * CINF];
  uint64_t *Output = new uint64_t[G * COUTF * N * HOUT * WOUT];
  std::vector<int> chunks_per_thread;
  if (G > 1)
  {
    chunks_per_thread = divide_instances(::num_threads, G, COUTF);
  }
  else
  {
    int min_chunk_size =
        ceil(THREADING_MIN_CHUNK_SIZE / double(reshaped_image_size));
    chunks_per_thread = divide_instances(::num_threads, COUTF, min_chunk_size);
  }

  cout << "chunks_per_thread[0]: " << chunks_per_thread[0] << endl;
  for (int g = 0; g < G; g++)
  {
    Conv2DReshapeInputGroup(N, H, W, CIN, HF, WF, HPADL, HPADR, WPADL, WPADR,
                            HSTR, WSTR, g, G, HF * WF * CINF, N * HOUT * WOUT,
                            A, Image + (g * reshaped_image_size));
    for (int co = 0; co < COUTF; co++)
    {
      for (int f = 0; f < HF * WF * CINF; f++)
      {
        Filter[g * (COUTF * HF * WF * CINF) + co * (HF * WF * CINF) + f] =
            B[g * (HF * WF * CINF * COUTF) + f * COUTF + co];
      }
    }
  }

  int offset = 0;
  int lnum_threads = chunks_per_thread.size();
  std::thread threads[lnum_threads];
  for (int i = 0; i < lnum_threads; i++)
  {
    MultMode mode = (i & 1 ? MultMode::Bob_has_A : MultMode::Alice_has_A);
    if (G > 1)
    {
      threads[i] = std::thread(
          GroupedMatMul_thread, i, Filter + (offset * COUTF * HF * WF * CINF),
          Image + (offset * reshaped_image_size),
          Output + (offset * COUTF * N * HOUT * WOUT), COUTF, HF * WF * CINF,
          N * HOUT * WOUT, chunks_per_thread[i], bwB, bwA, bwC, bwTemp, shiftB,
          shiftA, H1, H2, shift_demote, mode);
    }
    else
    {
      threads[i] = std::thread(
          GroupedMatMul_thread, i, Filter + (offset * HF * WF * CINF), Image,
          Output + (offset * N * HOUT * WOUT), chunks_per_thread[i],
          HF * WF * CINF, N * HOUT * WOUT, 1, bwB, bwA, bwC, bwTemp, shiftB,
          shiftA, H1, H2, shift_demote, mode);
    }
    offset += chunks_per_thread[i];
  }
  for (int i = 0; i < lnum_threads; ++i)
  {
    threads[i].join();
  }

  for (int g = 0; g < G; g++)
  {
    Conv2DReshapeMatMulOPGroup(N, HOUT, WOUT, COUTF * G, g, G,
                               Output + (g * COUTF * N * HOUT * WOUT), C);
  }
  delete[] Image;
  delete[] Filter;
  delete[] Output;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  ConvCommSent += curComm;
  ConvTimeInMilliSec += temp;
  std::cout << "Time in sec for current Conv = " << (temp / 1000.0)
            << std::endl;
#endif

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[N * H * W * CIN];
  int64_t *recB = new int64_t[G * HF * WF * CINF * COUTF];
  int64_t *recC = new int64_t[N * HOUT * WOUT * COUTF * G];
  int64_t *correctC = new int64_t[N * HOUT * WOUT * COUTF * G];
  reconstruct(A, recA, N, H * W * CIN, bwA);
  reconstruct(B, recB, G, HF * WF * CINF * COUTF, bwB);
  reconstruct(C, recC, N, HOUT * WOUT * COUTF * G, bwC);
  cleartext_Convolution(recA, recB, correctC, nullptr, N, H, W, CIN, HF, WF,
                        CINF, COUTF, HOUT, WOUT, HPADL, HPADR, WPADL, WPADR,
                        HSTR, WSTR, HDL, WDL, G, shrA, shrB, H1, H2, demote);

  for (int i = 0; i < N * HOUT * WOUT * COUTF * G; i++)
  {
    uint64_t tmpC = uint64_t(correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < N * HOUT * WOUT * COUTF * G; i++)
    {
      if (recC[i] != (correctC[i]))
      {
        pass = false;
        std::cout << i << "\t" << int(recC[i]) << "\t" << int(correctC[i])
                  << std::endl;
      }
    }
    if (pass == true)
      std::cout << GREEN << "Convolution Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Convolution Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
}

void primihub::cryptflow2::ReLU(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
          int32_t bwB, uint64_t six, int32_t div)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif

  assert(bwA >= bwB);

  int32_t dim = I * J;
  uint64_t *tmpB = new uint64_t[dim];
  math->ReLU(dim, A, tmpB, bwA, six);

#ifdef DIV_RESCALING
  truncation->div_pow2(dim, tmpB, B, div, bwA, true);
  aux->reduce(dim, B, B, bwA, bwB);
#else
  if ((bwA - bwB) >= div)
  {
    truncation->truncate_and_reduce(dim, tmpB, B, div, bwA);
    aux->reduce(dim, B, B, bwA - div, bwB);
  }
  else
  {
    truncation->truncate(dim, tmpB, B, div, bwA, true);
    aux->reduce(dim, B, B, bwA, bwB);
  }
#endif

  delete[] tmpB;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  ReluTimeInMilliSec += temp;
  std::cout << "Time in sec for current ReLU = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  ReluCommSent += curComm;
#endif
}

void primihub::cryptflow2::BNorm(uint64_t *A, uint64_t *BNW, uint64_t *BNB, uint64_t *B, int32_t I,
           int32_t J, int32_t bwA, int32_t bwBNW, int32_t bwBNB, int32_t bwTemp,
           int32_t bwB, int32_t shA, int32_t shBNB, int32_t shB)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  uint64_t maskTemp = (bwTemp == 64 ? -1 : ((1ULL << bwTemp) - 1));

  uint64_t *tmpA = new uint64_t[I * J];
  uint64_t *tmpBNB = new uint64_t[J];
  uint64_t *tmpBNW = new uint64_t[J];
  uint64_t *tmpB = new uint64_t[I * J];
  xt->s_extend(I * J, A, tmpA, bwA, bwTemp);
  xt->s_extend(J, BNB, tmpBNB, bwBNB, bwTemp);
  // xt->s_extend(J, BNW, tmpBNW, bwBNW, bwTemp);
  memcpy(tmpBNW, BNW, J * sizeof(uint64_t));
  if (shA <= 0)
  {
    for (int i = 0; i < I * J; i++)
    {
      A[i] = (tmpA[i] << (-1 * shA));
    }
  }
  else
  {
#ifdef DIV_RESCALING
    truncation->div_pow2(I * J, tmpA, A, shA, bwTemp, true);
#else
    truncation->truncate(I * J, tmpA, A, shA, bwTemp, true);
#endif
  }
  if (shBNB <= 0)
  {
    for (int i = 0; i < J; i++)
    {
      BNB[i] = (tmpBNB[i] << (-1 * shBNB));
    }
  }
  else
  {
#ifdef DIV_RESCALING
    truncation->div_pow2(J, tmpBNB, BNB, shBNB, bwTemp, true);
#else
    truncation->truncate(J, tmpBNB, BNB, shBNB, bwTemp, true);
#endif
  }
  for (int i = 0; i < I; i++)
  {
    for (int j = 0; j < J; j++)
    {
      tmpA[i * J + j] = (A[i * J + j] + BNB[j]) & maskTemp;
    }
  }
  // mult->hadamard_product(I*J, tmpA, tmpBNW, tmpB, bwTemp, bwTemp, bwTemp,
  // true, true);
  mult->matrix_multiplication(I, J, 1, tmpA, tmpBNW, tmpB, bwTemp, bwBNW,
                              bwTemp, true, true, false, MultMode::Alice_has_B);

  if (shB <= 0)
  {
    for (int i = 0; i < I * J; i++)
    {
      B[i] = (tmpB[i] << (-1 * shB));
    }
  }
  else
  {
#ifdef DIV_RESCALING
    truncation->div_pow2(I * J, tmpB, B, shB, bwTemp, true);
#else
    truncation->truncate(I * J, tmpB, B, shB, bwTemp, true);
#endif
  }
  aux->reduce(I * J, B, B, bwTemp, bwB);

  delete[] tmpA;
  delete[] tmpBNB;
  delete[] tmpBNW;
  delete[] tmpB;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  BatchNormInMilliSec += temp;
  std::cout << "Time in sec for current BatchNorm = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  BatchNormCommSent += curComm;
#endif
}

void primihub::cryptflow2::NormaliseL2(uint64_t *A, uint64_t *B, int32_t I, int32_t J, int32_t bwA,
                 int32_t scaleA, int32_t shrA)
{
#ifdef LOG_LAYERWISE
  INIT_TIMER;
  INIT_ALL_IO_DATA_SENT;
#endif
  int32_t scale_in = -1 * scaleA;
  int32_t scale_out = -1 * (scaleA + 1);
  int32_t bw_sumSquare = (2 * bwA - 2 * shrA) + ceil(log2(J));
  uint64_t mask_sumSquare =
      (bw_sumSquare == 64 ? -1 : ((1ULL << (bw_sumSquare)) - 1));

  uint64_t *tmpB = new uint64_t[I * J];
  uint64_t *sumSquare = new uint64_t[I];
  uint64_t *inverseNorm = new uint64_t[I];
  mult->hadamard_product(I * J, A, A, tmpB, bwA, bwA, 2 * bwA, true, true,
                         MultMode::None);
  truncation->truncate(I * J, tmpB, B, 2 * shrA, 2 * bwA, true);
  // truncation->truncate_and_reduce(I*J, tmpB, B, 2*shrA, 2*bwA);
  for (int i = 0; i < I; i++)
  {
    sumSquare[i] = 0;
    for (int j = 0; j < J; j++)
    {
      sumSquare[i] = (sumSquare[i] + B[i * J + j]);
    }
    sumSquare[i] &= mask_sumSquare;
  }

  math->sqrt(I, sumSquare, inverseNorm, bw_sumSquare, bwA + shrA,
             2 * (scale_in - shrA), scale_out - scale_in + shrA, true);

  mult->matrix_multiplication(1, I, J, inverseNorm, A, B, bwA + shrA, bwA,
                              bwA + shrA, true, true, false, MultMode::None);

#ifdef DIV_RESCALING
  truncation->div_pow2(I * J, B, tmpB, shrA, bwA + shrA, true);
  aux->reduce(I * J, tmpB, tmpB, bwA + shrA, bwA);
#else
  truncation->truncate_and_reduce(I * J, B, tmpB, shrA, bwA + shrA);
#endif

  for (int i = 0; i < I; i++)
  {
    for (int j = 0; j < J; j++)
    {
      B[i * J + j] = tmpB[j * I + i];
    }
  }

  delete[] tmpB;
  delete[] sumSquare;
  delete[] inverseNorm;

#ifdef LOG_LAYERWISE
  auto temp = TIMER_TILL_NOW;
  NormaliseL2TimeInMilliSec += temp;
  std::cout << "Time in sec for current NormaliseL2 = " << (temp / 1000.0)
            << std::endl;
  uint64_t curComm;
  FIND_ALL_IO_TILL_NOW(curComm);
  NormaliseL2CommSent += curComm;
#endif
}

// template<class int64_t, class int64_t, class int64_t, class int64_t>
void primihub::cryptflow2::MatAdd(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t shrC,
            int64_t demote, int64_t bwA, int64_t bwB, int64_t bwTemp,
            int64_t bwC, int64_t *A, int64_t *B, int64_t *C, bool verbose)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatAdd(A, B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }

#else
#ifdef LOG_LAYERWISE
  if (verbose)
    std::cout << "MatAdd (" << I << " x " << J << ")" << std::endl;
#endif
  // C = (A / (shrA*shrC) + B / (shrB*ShrC)) / demote
  // int32_t bwA = sizeof(int64_t)*8;
  // int32_t bwB = sizeof(int64_t)*8;
  // int32_t bwC = sizeof(int64_t)*8;
  // int32_t bwTemp = sizeof(int64_t)*8;

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);
  uint64_t *tmpB = new uint64_t[I * J];
  typecast_to_uint64(B, tmpB, I, J, bwB);
  uint64_t *tmpC = new uint64_t[I * J];

  MatAdd(tmpA, tmpB, tmpC, I, J, bwA, bwB, bwC, bwTemp, shiftA, shiftB, shiftC,
         shift_demote);

  typecast_from_uint64(tmpC, C, I, J, bwC);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;

  if (!verbose)
    return;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatAdd(recA, recB, correctC, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }
  /*
  for(int i = 0; i < I*J; i++) {
      std::cout << i << "\t" << int64_t(recA[i]) << std::endl;
  }
  for(int i = 0; i < I*J; i++) {
      std::cout << i << "\t" << int64_t(recB[i]) << std::endl;
  }
  */
  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // if (verbose) std::cout << i << "\t" << int(recC[i]) << "\t" <<
      // int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatAdd Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MatAdd Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

// template<class int64_t, class int64_t, class int64_t, class int64_t>
void primihub::cryptflow2::MatSub(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t shrC,
            int64_t demote, int64_t bwA, int64_t bwB, int64_t bwTemp,
            int64_t bwC, int64_t *A, int64_t *B, int64_t *C)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatSub(A, B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "MatSub (" << I << " x " << J << ")" << std::endl;
#endif
  int64_t *minus_B = new int64_t[I * J];
  for (int i = 0; i < I * J; i++)
  {
    minus_B[i] = int64_t(-1) * B[i];
  }
  MatAdd(I, J, shrA, shrB, shrC, demote, bwA, bwB, bwTemp, bwC, A, minus_B, C,
         false);

  delete[] minus_B;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatSub(recA, recB, correctC, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recC[i]) << "\t" << int(correctC[i]) <<
      // std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatSub Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MatSub Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::MatAddBroadCastA(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t A, int64_t *B,
                      int64_t *C, bool verbose)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatAddBroadCastA(&A, B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  if (verbose)
    std::cout << "MatAddBroadCastA (" << I << " x " << J << ")" << std::endl;
#endif

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);

  uint64_t tmpA = uint64_t(A);
  uint64_t *tmpB = new uint64_t[I * J];
  typecast_to_uint64(B, tmpB, I, J, bwB);
  uint64_t *tmpC = new uint64_t[I * J];

  MatAddBroadCast(&tmpA, tmpB, tmpC, I, J, bwA, bwB, bwC, bwTemp, shiftA,
                  shiftB, shiftC, shift_demote, true);
  typecast_from_uint64(tmpC, C, I, J, bwC);

  delete[] tmpB;
  delete[] tmpC;

  if (!verbose)
    return;

#ifdef VERIFY_LAYERWISE
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatAddBroadCastA(&A, recB, correctC, I, J, shrA, shrB, shrC,
                             demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // if (verbose) std::cout << i << "\t" << int(recC[i]) << "\t" <<
      // int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatAddBroadCastA Output Matches" << RESET
                << std::endl;
    else
      std::cout << RED << "MatAddBroadCastA Output Mismatch" << RESET
                << std::endl;
  }

  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::MatAddBroadCastB(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t *A, int64_t B,
                      int64_t *C, bool verbose)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatAddBroadCastB(A, &B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }

#else
#ifdef LOG_LAYERWISE
  if (verbose)
    std::cout << "MatAddBroadCastB (" << I << " x " << J << ")" << std::endl;
#endif
  // int32_t bwA = sizeof(int64_t)*8;
  // int32_t bwB = sizeof(int64_t)*8;
  // int32_t bwC = sizeof(int64_t)*8;
  // int32_t bwTemp = sizeof(int64_t)*8;

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);

  uint64_t tmpB = uint64_t(B);
  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);
  uint64_t *tmpC = new uint64_t[I * J];

  MatAddBroadCast(tmpA, &tmpB, tmpC, I, J, bwA, bwB, bwC, bwTemp, shiftA,
                  shiftB, shiftC, shift_demote, false);
  typecast_from_uint64(tmpC, C, I, J, bwC);

  delete[] tmpA;
  delete[] tmpC;

  if (!verbose)
    return;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatAddBroadCastB(recA, &B, correctC, I, J, shrA, shrB, shrC,
                             demote);

  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // if (verbose) std::cout << i << "\t" << int(recA[i]) << "\t" <<
      // int(recC[i]) << "\t" << int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatAddBroadCastB Output Matches" << RESET
                << std::endl;
    else
      std::cout << RED << "MatAddBroadCastB Output Mismatch" << RESET
                << std::endl;
  }

  delete[] recA;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::MatSubBroadCastA(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t A, int64_t *B,
                      int64_t *C)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatSubBroadCastA(&A, B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "MatSubBroadCastA (" << I << " x " << J << ")" << std::endl;
#endif
  int64_t *minus_B = new int64_t[I * J];
  for (int i = 0; i < I * J; i++)
  {
    minus_B[i] = int64_t(-1) * B[i];
  }
  MatAddBroadCastA(I, J, shrA, shrB, shrC, demote, bwA, bwB, bwTemp, bwC, A,
                   minus_B, C, false);

  delete[] minus_B;

#ifdef VERIFY_LAYERWISE
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatSubBroadCastA(&A, recB, correctC, I, J, shrA, shrB, shrC,
                             demote);

  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recC[i]) << "\t" << int(correctC[i]) <<
      // std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatSubBroadCastA Output Matches" << RESET
                << std::endl;
    else
      std::cout << RED << "MatSubBroadCastA Output Mismatch" << RESET
                << std::endl;
  }

  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::MatSubBroadCastB(int64_t I, int64_t J, int64_t shrA, int64_t shrB,
                      int64_t shrC, int64_t demote, int64_t bwA, int64_t bwB,
                      int64_t bwTemp, int64_t bwC, int64_t *A, int64_t B,
                      int64_t *C)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MatSubBroadCastB(A, &B, C, I, J, shrA, shrB, shrC, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "MatSubBroadCastB (" << I << " x " << J << ")" << std::endl;
#endif

  int64_t minus_B = int64_t(-1) * (B);
  MatAddBroadCastB(I, J, shrA, shrB, shrC, demote, bwA, bwB, bwTemp, bwC, A,
                   minus_B, C, false);

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(C, recC, I, J, bwC);
  cleartext_MatSubBroadCastB(recA, &B, correctC, I, J, shrA, shrB, shrC,
                             demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }
  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // if (verbose) std::cout << i << "\t" << int(recA[i]) << "\t" <<
      // int(recC[i]) << "\t" << int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MatSubBroadCastB Output Matches" << RESET
                << std::endl;
    else
      std::cout << RED << "MatSubBroadCastB Output Mismatch" << RESET
                << std::endl;
  }

  delete[] recA;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

// template<class int64_t>
void primihub::cryptflow2::AdjustScaleShl(int64_t I, int64_t J, int64_t scale, int64_t *A)
{
  // A = (A * scale)
  for (int i = 0; i < I * J; i++)
  {
    A[i] = A[i] * int64_t(scale);
  }
}

void MulCir(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
            int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, int64_t *A,
            int64_t *B, int64_t *C)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MulCir(A, B, C, I, J, shrA, shrB, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }

#else
#ifdef LOG_LAYERWISE
  std::cout << "MulCir (" << I << " x " << J << ")" << std::endl;
#endif

  MulCir(I, J, shrA, shrB, demote, bwA, bwB, bwTemp, bwC, (uint64_t *)A,
         (uint64_t *)B, (uint64_t *)C);

#endif
}

void primihub::cryptflow2::ScalarMul(int64_t I, int64_t J, int64_t shrA, int64_t shrB, int64_t demote,
               int64_t bwA, int64_t bwB, int64_t bwTemp, int64_t bwC, int64_t A,
               int64_t *B, int64_t *C)
{
#ifdef CLEARTEXT_ONLY
  cleartext_ScalarMul(&A, B, C, I, J, shrA, shrB, demote);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". ScalarMul (" << I << " x " << J << ")" << std::endl;
#endif

  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shift_demote = log2(demote);

  uint64_t *tmpA = new uint64_t[1];
  typecast_to_uint64(&A, tmpA, 1, 1, bwA);
  uint64_t *tmpB = new uint64_t[I * J];
  typecast_to_uint64(B, tmpB, I, J, bwB);

  uint64_t *tmpC = new uint64_t[I * J];

  ScalarMul(tmpA, tmpB, tmpC, I, J, bwA, bwB, bwTemp, bwC, shiftA, shiftB,
            shift_demote);
  typecast_from_uint64(tmpC, C, I, J, bwC);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;

#ifdef VERIFY_LAYERWISE
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(B, recB, I, J, bwB);
  reconstruct(C, recC, I, J, bwC);
  cleartext_ScalarMul(&A, recB, correctC, I, J, shrA, shrB, demote);

  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // if (verbose) std::cout << i << "\t" << int(recC[i]) << "\t" <<
      // int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "ScalarMul Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "ScalarMul Output Mismatch" << RESET << std::endl;
  }

  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::Sigmoid(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
             int64_t bwA, int64_t bwB, int64_t *A, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_Sigmoid(A, I, J, scale_in, scale_out, bwA, bwB, B);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (B[i]) % (1LL << bwB);
    B[i] = signed_val(tmpC, bwB);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "Sigmoid (" << I << " x " << J << ")" << std::endl;
#endif

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmpB = new uint64_t[I * J];
  Sigmoid(I, J, scale_in, scale_out, bwA, bwB, tmpA, tmpB);
  typecast_from_uint64(tmpB, B, I, J, bwB);

  delete[] tmpA;
  delete[] tmpB;

#endif
}

void primihub::cryptflow2::TanH(int64_t I, int64_t J, int64_t scale_in, int64_t scale_out,
          int64_t bwA, int64_t bwB, int64_t *A, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_TanH(A, I, J, scale_in, scale_out, bwA, bwB, B);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (B[i]) % (1LL << bwB);
    B[i] = signed_val(tmpC, bwB);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "TanH (" << I << " x " << J << ")" << std::endl;
#endif

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmpB = new uint64_t[I * J];
  TanH(I, J, scale_in, scale_out, bwA, bwA, tmpA, tmpB);
  typecast_from_uint64(tmpB, B, I, J, bwB);

  delete[] tmpA;
  delete[] tmpB;

#endif
}

// template<class int64_t>
void primihub::cryptflow2::ArgMax(int64_t I, int64_t J, int32_t bwA, int32_t bw_index, int64_t *A,
            int64_t *index)
{
#ifdef CLEARTEXT_ONLY
  int32_t index_tmp;
  cleartext_ArgMax(A, (int32_t)I, (int32_t)J, &index_tmp);
  *index = index_tmp;
#else
#ifdef LOG_LAYERWISE
  std::cout << "ArgMax (" << I << " x " << J << ")" << std::endl;
#endif

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmp_index = new uint64_t[1];
  ArgMax(tmpA, I, J, bwA, bw_index, tmp_index);
  typecast_from_uint64(tmp_index, index, 1, 1, bw_index);

  delete[] tmpA;
  delete[] tmp_index;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *rec_index = new int64_t[1];
  int *correct_index = new int[1];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(index, rec_index, 1, 1, bw_index);

  cleartext_ArgMax(recA, I, J, correct_index);

  if (party == 2)
  {
    bool pass = true;
    if (rec_index[0] != correct_index[0])
    {
      pass = false;
    }
    std::cout << rec_index[0] << "\t" << correct_index[0] << std::endl;
    if (pass == true)
      std::cout << GREEN << "ArgMax Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "ArgMax Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] rec_index;
  delete[] correct_index;
#endif
#endif
}

void primihub::cryptflow2::AdjustScaleShr(int32_t I, int32_t J, int32_t scale, int64_t bwA,
                    int64_t *A)
{
#ifdef CLEARTEXT_ONLY
  for (int i = 0; i < I * J; i++)
  {
    A[i] /= scale;
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". AdjustScaleShr (" << I << " x " << J << ")"
            << std::endl;
#endif

  int32_t shift_scale = ceil(log2(scale));

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmpB = new uint64_t[I * J];
  AdjustScaleShr(tmpA, tmpB, I, J, bwA, shift_scale);
  typecast_from_uint64(tmpB, A, I, J, bwA);

  delete[] tmpA;
  delete[] tmpB;
#endif
}

void primihub::cryptflow2::Convolution(int32_t N, int32_t H, int32_t W, int32_t CIN, int32_t HF,
                 int32_t WF, int32_t CINF, int32_t COUTF, int32_t HOUT,
                 int32_t WOUT, int32_t HPADL, int32_t HPADR, int32_t WPADL,
                 int32_t WPADR, int32_t HSTR, int32_t WSTR, int32_t HDL,
                 int32_t WDL, int32_t G, int32_t shrA, int32_t shrB, int32_t H1,
                 int32_t H2, int32_t demote, int32_t bwA, int32_t bwB,
                 int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B,
                 int64_t *C, int64_t *tmp, bool verbose)
{
#ifdef CLEARTEXT_ONLY
  cleartext_Convolution(A, B, C, nullptr, N, H, W, CIN, HF, WF, CINF, COUTF,
                        HOUT, WOUT, HPADL, HPADR, WPADL, WPADR, HSTR, WSTR, HDL,
                        WDL, G, shrA, shrB, H1, H2, demote);
  for (int i = 0; i < N * HOUT * WOUT * COUTF * G; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }

#else
  assert((HDL == 1) && (WDL == 1));

  uint64_t *tmpA = new uint64_t[N * H * W * CIN];
  typecast_to_uint64(A, tmpA, N, H * W * CIN, bwA);
  uint64_t *tmpB = new uint64_t[G * HF * WF * CINF * COUTF];
  typecast_to_uint64(B, tmpB, G, HF * WF * CINF * COUTF, bwB);

  uint64_t *tmpC = new uint64_t[N * HOUT * WOUT * COUTF * G];
  Convolution(N, H, W, CIN, HF, WF, CINF, COUTF, HOUT, WOUT, HPADL, HPADR,
              WPADL, WPADR, HSTR, WSTR, HDL, WDL, G, bwA, bwB, bwC, bwTemp,
              shrA, shrB, H1, H2, demote, tmpA, tmpB, tmpC);

  typecast_from_uint64(tmpC, C, N, HOUT * WOUT * COUTF * G, bwC);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;
#endif
}

void primihub::cryptflow2::NormaliseL2(int32_t N, int32_t H, int32_t W, int32_t C, int32_t scaleA,
                 int32_t shrA, int32_t bwA, int64_t *A, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_NormaliseL2(A, B, N, H, W, C, scaleA, shrA, bwA, bwA);
  for (int i = 0; i < (N * H * W * C); i++)
  {
    uint64_t tmpB = (B[i]) % (1LL << bwA);
    B[i] = signed_val(tmpB, bwA);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". NormaliseL2 (" << N * H * W << " x " << C << ")"
            << std::endl;
#endif
  int32_t scale_in = -1 * scaleA;
  int32_t scale_out = -1 * (scaleA + 1);

  uint64_t *tmpA = new uint64_t[N * H * W * C];
  typecast_to_uint64(A, tmpA, N * H * W, C, bwA);
  uint64_t *tmpB = new uint64_t[N * H * W * C];

  NormaliseL2(tmpA, tmpB, N * H * W, C, bwA, scaleA, shrA);
  typecast_from_uint64(tmpB, B, N * H * W, C, bwA);

  delete[] tmpA;
  delete[] tmpB;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[N * H * W * C];
  int64_t *recB = new int64_t[N * H * W * C];
  int64_t *correctB = new int64_t[N * H * W * C];
  reconstruct(A, recA, N * H * W, C, bwA);
  reconstruct(B, recB, N * H * W, C, bwA);
  cleartext_NormaliseL2(recA, correctB, N, H, W, C, scaleA, shrA, bwA, bwA);
  // cleartext_NormaliseL2_seedot<int64_t>(recA, correctB, N, H, W, C, scaleA,
  // shrA);
  for (int i = 0; i < (N * H * W * C); i++)
  {
    uint64_t tmpB = (correctB[i]) % (1LL << bwA);
    correctB[i] = signed_val(tmpB, bwA);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < N * H * W * C; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
      std::cout << double(recA[i]) / (1LL << scale_in) << "\t"
                << double(recB[i]) / (1LL << scale_out) << "\t"
                << double(correctB[i]) / (1LL << scale_out) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "NormaliseL2 Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "NormaliseL2 Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
#endif
}

void primihub::cryptflow2::Relu6(int32_t N, int32_t H, int32_t W, int32_t C, int64_t six, int32_t div,
           int32_t bwA, int32_t bwB, int64_t *A, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_Relu6(A, B, N, H, W, C, six, div);
  for (int i = 0; i < N * H * W * C; i++)
  {
    uint64_t tmpC = (B[i]) % (1LL << bwB);
    B[i] = signed_val(tmpC, bwB);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Relu6 (" << N << "x" << H << "x" << W << "x" << C
            << ")" << std::endl;
#endif

  int32_t shift_div = log2(div);

  uint64_t *tmpA = new uint64_t[N * H * W * C];
  typecast_to_uint64(A, tmpA, N * H * W, C, bwA);

  uint64_t *tmpB = new uint64_t[N * H * W * C];
  ReLU(tmpA, tmpB, N * H * W, C, bwA, bwB, six, shift_div);
  typecast_from_uint64(tmpB, B, N * H * W, C, bwB);

  delete[] tmpA;
  delete[] tmpB;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[N * H * W * C];
  int64_t *recB = new int64_t[N * H * W * C];
  int64_t *correctB = new int64_t[N * H * W * C];
  reconstruct(A, recA, N * H * W, C, bwA);
  reconstruct(B, recB, N * H * W, C, bwB);

  cleartext_Relu6(recA, correctB, N, H, W, C, six, div);
  for (int i = 0; i < N * H * W * C; i++)
  {
    uint64_t tmpC = (correctB[i]) % (1LL << bwB);
    correctB[i] = signed_val(tmpC, bwB);
  }
  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < N * H * W * C; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
    }
    if (pass == true)
      std::cout << GREEN << "Relu6 Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Relu6 Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
#endif
}

void primihub::cryptflow2::BNorm(int32_t I, int32_t J, int32_t shA, int32_t shBNB, int32_t shB,
           int32_t bwA, int32_t bwBNW, int32_t bwBNB, int32_t bwTemp,
           int32_t bwB, int64_t *A, int64_t *BNW, int64_t *BNB, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_BNorm(A, BNW, BNB, B, I, J, shA, shBNB, shB);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (B[i]) % (1LL << bwB);
    B[i] = signed_val(tmpC, bwB);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". BNorm (" << I << " x " << J << ")" << std::endl;
#endif

  assert(bwA <= bwTemp);
  assert(bwBNB <= bwTemp);
  assert(bwBNW <= bwTemp);
  assert(bwB <= bwTemp);

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);
  uint64_t *tmpBNW = new uint64_t[J];
  typecast_to_uint64(BNW, tmpBNW, 1, J, bwBNW);
  uint64_t *tmpBNB = new uint64_t[J];
  typecast_to_uint64(BNB, tmpBNB, 1, J, bwBNB);
  uint64_t *tmpB = new uint64_t[I * J];

  BNorm(tmpA, tmpBNW, tmpBNB, tmpB, I, J, bwA, bwBNW, bwBNB, bwTemp, bwB, shA,
        shBNB, shB);

  typecast_from_uint64(tmpB, B, I, J, bwB);

  delete[] tmpA;
  delete[] tmpBNW;
  delete[] tmpBNB;
  delete[] tmpB;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recBNW = new int64_t[J];
  int64_t *recBNB = new int64_t[J];
  int64_t *recB = new int64_t[I * J];
  int64_t *correctB = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(BNW, recBNW, 1, J, bwBNW);
  reconstruct(BNB, recBNB, 1, J, bwBNB);
  reconstruct(B, recB, I, J, bwB);
  cleartext_BNorm(recA, recBNW, recBNB, correctB, I, J, shA, shBNB, shB);
  for (int i = 0; i < I * J; i++)
  {
    uint64_t tmpC = (correctB[i]) % (1LL << bwB);
    correctB[i] = signed_val(tmpC, bwB);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recB[i]) << "\t" << int(correctB[i]) <<
      // std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "BNorm Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "BNorm Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recBNW;
  delete[] recBNB;
  delete[] recB;
  delete[] correctB;
#endif
#endif
}

void primihub::cryptflow2::MaxPool2D(int I, int J, int bwA, int bwB, int64_t *A, int64_t *B)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MaxPool2D(A, I, J, B);
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". MaxPool2D (" << I << " x " << J << ")" << std::endl;
#endif

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmp_max = new uint64_t[I];
  MaxPool2D(tmpA, I, J, bwA, bwB, tmp_max);
  typecast_from_uint64(tmp_max, B, I, 1, bwB);

  delete[] tmpA;
  delete[] tmp_max;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I];
  int64_t *correct_max = new int64_t[I];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, 1, bwB);

  cleartext_MaxPool2D(recA, I, J, correct_max);

  // for(int i=0; i<I; i++){
  // B[i] = (party == 2 ? correct_max[i] : 0);
  //}
  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I; i++)
    {
      if (recB[i] != correct_max[i])
      {
        // if(i <= 10)
        // std::cout<<"s: "<<recB[i]<<" | e: "<<correct_max[i]<<std::endl;
        pass = false;
      }
    }
    if (pass == true)
      std::cout << GREEN << "MaxPool Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MaxPool Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correct_max;
#endif
#endif
}

// void fillArrayFullSize2D(int64_t *src, int64_t *dst, int I1, int I2, int J1,
// int J2)
// {
//     int itersx = (J1) / (I1);
//     int itersy = (J2) / (I2);

//     for (int i = 0; i < itersx; i++)
//     {
//         for (int j = 0; j < itersy; j++)
//         {
//             for (int k = 0; k < I1 * I2; k++)
//                 dst[i * J2 + j + k] = src[k];
//         }
//     }
// }

void primihub::cryptflow2::MBConv(int32_t N, int32_t H, int32_t W, int32_t Cin, int32_t Ct,
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
            int64_t *BN3B, int64_t *C, int64_t *X, int64_t *T, int64_t *U)
{
#ifdef CLEARTEXT_ONLY
  cleartext_MBConv(A, F1, BN1W, BN1B, F2, BN2W, BN2B, F3, BN3W, BN3B, C,
                   nullptr, nullptr, nullptr, N, H, W, Cin, Ct, HF, WF, Cout,
                   Hout, Wout, HPADL, HPADR, WPADL, WPADR, HSTR, WSTR, D1, D2,
                   D3, SIX_1, SIX_2, shr1, shr2, shr3, shr4, shr5, shr6, shr7,
                   shr8, shr9, shl1, shl2, shl3, shl4, shl5, shl6, shl7, shl8,
                   shl9);
  for (int i = 0; i < N * Hout * Wout * Cout; i++)
  {
    uint64_t tmpC = (C[i]) % (1LL << bwC);
    C[i] = signed_val(tmpC, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << "MBConv: Conv -> BNorm -> ReLU6 -> Conv -> BNorm -> ReLU6 -> "
               "Conv -> BNorm"
            << std::endl;
#endif
  int32_t depth_1 = ceil(log2(Cin));
  int64_t *tmpU = new int64_t[N * H * W * Ct];
  Convolution(N, H, W, Cin, 1, 1, Cin, Ct, H, W, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
              1, D1, depth_1 - D1, 1, bwA, bwF1, bwU, bwU, A, F1, tmpU, nullptr,
              true);

  int64_t *tmpUB1W = new int64_t[N * H * W * Ct];
  int64_t *tmpX = new int64_t[N * H * W * Ct];
  BNorm(N * H * W, Ct, int32_t(log2(shr1 / double(shl1))),
        int32_t(log2(shr2 / double(shl2))), 0, bwU, bwB1W, bwB1B, bwUB1W,
        bwUB1W, tmpU, BN1W, BN1B, tmpUB1W);

  Relu6(N, H, W, Ct, SIX_1, shr3, bwUB1W, bwX, tmpUB1W, tmpX);
  if (shl3 > 1)
  {
    for (int i = 0; i < N * H * W * Ct; i++)
    {
      tmpX[i] = (tmpX[i] * shl3);
    }
  }

  int32_t depth_2 = ceil(log2(HF * WF));
  delete[] tmpU;
  tmpU = new int64_t[N * Hout * Wout * Ct];
  Convolution(N, H, W, Ct, HF, WF, 1, 1, Hout, Wout, HPADL, HPADR, WPADL, WPADR,
              HSTR, WSTR, 1, 1, Ct, 1, 1, D2, depth_2 - D2, 1, bwX, bwF2, bwU,
              bwU, tmpX, F2, tmpU, nullptr, true);

  int64_t *tmpUB2W = new int64_t[N * Hout * Wout * Ct];
  int64_t *tmpT = new int64_t[N * Hout * Wout * Ct];
  BNorm(N * Hout * Wout, Ct, int32_t(log2(shr4 / double(shl4))),
        int32_t(log2(shr5 / double(shl5))), 0, bwU, bwB2W, bwB2B, bwUB2W,
        bwUB2W, tmpU, BN2W, BN2B, tmpUB2W);

  Relu6(N, Hout, Wout, Ct, SIX_2, shr6, bwUB2W, bwT, tmpUB2W, tmpT);

  int32_t depth_3 = ceil(log2(Ct));
  delete[] tmpU;
  tmpU = new int64_t[N * Hout * Wout * Cout];
  Convolution(N, Hout, Wout, Ct, 1, 1, Ct, Cout, Hout, Wout, 0, 0, 0, 0, 1, 1,
              1, 1, 1, 1, 1, D3, depth_3 - D3, 1, bwT, bwF3, bwU, bwU, tmpT, F3,
              tmpU, nullptr, true);

  BNorm(N * Hout * Wout, Cout, int32_t(log2(shr7 / double(shl7))),
        int32_t(log2(shr8 / double(shl8))), int32_t(log2(shr9 / double(shl9))),
        bwU, bwB3W, bwB3B, bwUB3W, bwC, tmpU, BN3W, BN3B, C);

  delete[] tmpU;
  delete[] tmpT;
  delete[] tmpX;
  delete[] tmpUB1W;
  delete[] tmpUB2W;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[N * H * W * Cin];
  int64_t *recF1 = new int64_t[Cin * Ct];
  int64_t *recBN1W = new int64_t[Ct];
  int64_t *recBN1B = new int64_t[Ct];
  int64_t *recF2 = new int64_t[Ct * HF * WF];
  int64_t *recBN2W = new int64_t[Ct];
  int64_t *recBN2B = new int64_t[Ct];
  int64_t *recF3 = new int64_t[Ct * Cout];
  int64_t *recBN3W = new int64_t[Cout];
  int64_t *recBN3B = new int64_t[Cout];
  int64_t *recC = new int64_t[N * Hout * Wout * Cout];
  int64_t *correctC = new int64_t[N * Hout * Wout * Cout];

  reconstruct(A, recA, N * H * W, Cin, bwA);
  reconstruct(F1, recF1, Cin, Ct, bwF1);
  reconstruct(BN1W, recBN1W, 1, Ct, bwB1W);
  reconstruct(BN1B, recBN1B, 1, Ct, bwB1B);
  reconstruct(F2, recF2, Ct, HF * WF, bwF2);
  reconstruct(BN2W, recBN2W, 1, Ct, bwB2W);
  reconstruct(BN2B, recBN2B, 1, Ct, bwB2B);
  reconstruct(F3, recF3, Ct, Cout, bwF3);
  reconstruct(BN3W, recBN3W, 1, Cout, bwB3W);
  reconstruct(BN3B, recBN3B, 1, Cout, bwB3B);
  reconstruct(C, recC, N * Hout * Wout, Cout, bwC);

  cleartext_MBConv(recA, recF1, recBN1W, recBN1B, recF2, recBN2W, recBN2B,
                   recF3, recBN3W, recBN3B, correctC, nullptr, nullptr, nullptr,
                   N, H, W, Cin, Ct, HF, WF, Cout, Hout, Wout, HPADL, HPADR,
                   WPADL, WPADR, HSTR, WSTR, D1, D2, D3, SIX_1, SIX_2, shr1,
                   shr2, shr3, shr4, shr5, shr6, shr7, shr8, shr9, shl1, shl2,
                   shl3, shl4, shl5, shl6, shl7, shl8, shl9);
  for (int i = 0; i < N * Hout * Wout * Cout; i++)
  {
    uint64_t tmpC = (correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpC, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < N * Hout * Wout * Cout; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recC[i]) << "\t" << int(correctC[i]) <<
      // std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "MBConv Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "MBConv Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recC;
  delete[] correctC;
#endif
#endif
}

void primihub::cryptflow2::AddOrSubCir4D(int32_t N, int32_t H, int32_t W, int32_t C, int32_t shrA,
                   int32_t shrB, int32_t shrC, bool add, int32_t demote,
                   int32_t bwA, int32_t bwB, int32_t bwTemp, int32_t bwC,
                   int64_t *A, int64_t *B, int64_t *X)
{
#ifdef CLEARTEXT_ONLY
  cleartext_AddOrSubCir4D(A, B, X, N, H, W, C, shrA, shrB, shrC, add, demote);
  for (int i = 0; i < (N * H * W * C); i++)
  {
    uint64_t tmpX = (X[i]) % (1LL << bwC);
    X[i] = signed_val(tmpX, bwC);
  }
#else
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". AddOrSubCir4D (" << N << "x" << H << "x" << W << "x"
            << C << ")" << std::endl;
#endif
  // C = (A / (shrA*shrC) + B / (shrB*ShrC)) / demote
  int32_t shiftA = log2(shrA);
  int32_t shiftB = log2(shrB);
  int32_t shiftC = log2(shrC);
  int32_t shift_demote = log2(demote);

  uint64_t *tmpA = new uint64_t[N * H * W * C];
  typecast_to_uint64(A, tmpA, N * H * W, C, bwA);
  uint64_t *tmpB = new uint64_t[C];
  typecast_to_uint64(B, tmpB, 1, C, bwB);
  uint64_t *tmpC = new uint64_t[N * H * W * C];

  AddOrSubCir(tmpA, tmpB, tmpC, N * H * W, C, bwA, bwB, bwC, bwTemp, shiftA,
              shiftB, shiftC, add, shift_demote);

  typecast_from_uint64(tmpC, X, N * H * W, C, bwC);

#ifdef VERIFY_LAYERWISE
  int64_t *origA = new int64_t[N * H * W * C];
  typecast_from_uint64(tmpA, origA, N * H * W, C, bwA);
  int64_t *recA = new int64_t[N * H * W * C];
  int64_t *recB = new int64_t[C];
  int64_t *recC = new int64_t[N * H * W * C];
  int64_t *correctC = new int64_t[N * H * W * C];
  reconstruct(origA, recA, N * H * W, C, bwA);
  reconstruct(B, recB, C, 1, bwB);
  reconstruct(X, recC, N * H * W, C, bwC);
  cleartext_AddOrSubCir4D(recA, recB, correctC, N, H, W, C, shrA, shrB, shrC,
                          add, demote);
  for (int i = 0; i < (N * H * W * C); i++)
  {
    uint64_t tmpX = uint64_t(correctC[i]) % (1LL << bwC);
    correctC[i] = signed_val(tmpX, bwC);
  }

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < N * H * W * C; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // std::cout << i << "\t" << int(recA[i]) << "\t" << int(recB[i%C]) <<
      // "\t" << int(recC[i]) << "\t" << int(correctC[i]) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "AddOrSubCir4D Output Matches" << RESET
                << std::endl;
    else
      std::cout << RED << "AddOrSubCir4D Output Mismatch" << RESET << std::endl;
  }

  delete[] origA;
  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;
#endif
}

void primihub::cryptflow2::MatAdd4(int32_t N, int32_t H, int32_t W, int32_t C, int32_t shrA,
             int32_t shrB, int32_t shrC, int32_t demote, int32_t bwA,
             int32_t bwB, int32_t bwTemp, int32_t bwC, int64_t *A, int64_t *B,
             int64_t *X)
{
  MatAdd(N * H * W, C, shrA, shrB, shrC, demote, bwA, bwB, bwTemp, bwC, A, B, X,
         true);
}

// Athos wrappers. Athos passes scale factor as is. We need to take pow_2
int64_t pow_2(int32_t p)
{
  int64_t res = 1;
  for (int i = 0; i < p; i++)
  {
    res *= 2;
  }
  return res;
}

void primihub::cryptflow2::Sigmoid(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
             int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B)
{
  Sigmoid((int64_t)I, (int64_t)J, pow_2(scale_in), pow_2(scale_out),
          (int64_t)bwA, (int64_t)bwB, A, B);
}

void primihub::cryptflow2::TanH(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, uint64_t *A, uint64_t *B)
{
  TanH((int64_t)I, (int64_t)J, pow_2(scale_in), pow_2(scale_out), (int64_t)bwA,
       (int64_t)bwB, A, B);
}

void primihub::cryptflow2::Sqrt(int32_t I, int32_t J, int32_t scale_in, int32_t scale_out,
          int32_t bwA, int32_t bwB, bool inverse, uint64_t *A, uint64_t *B)
{
  Sqrt((int64_t)I, (int64_t)J, pow_2(scale_in), pow_2(scale_out), (int64_t)bwA,
       (int64_t)bwB, inverse, A, B);
}

void primihub::cryptflow2::Exp(int32_t I, int32_t J, int32_t shrA, int32_t shrB, int32_t bwA,
         int64_t *A, int64_t *B)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Exp (" << I << " x " << J << ")" << std::endl;
#endif

  int32_t s_A = log2(shrA);
  int32_t s_B = log2(shrB);

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmpB = new uint64_t[I * J];
  Exp(tmpA, tmpB, I, J, bwA, bwA, s_A, s_B);
  typecast_from_uint64(tmpB, B, I, J, bwA);

  delete[] tmpA;
  delete[] tmpB;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *correctB = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwA);
  cleartext_Exp_lookup(recA, I, J, bwA, shrA, shrB, correctB, 1);

  if (party == 2)
  {
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recB[i] != correctB[i])
      {
        pass = false;
      }
      // std::cout << recA[i]/double(1LL << s_A) << "\t" << recB[i]/double(1LL
      // << s_B) << "\t" << correctB[i]/double(1LL << s_B) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "Exp Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Exp Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] correctB;
#endif
  return;
}

void primihub::cryptflow2::Div(int32_t I, int32_t J, int32_t shrA, int32_t shrB, int32_t shrC,
         int32_t bwA, int64_t *A, int64_t *B, int64_t *C)
{
#ifdef LOG_LAYERWISE
  std::cout << ctr++ << ". Div (" << I << " x " << J << ")" << std::endl;
#endif

  int32_t s_A = log2(shrA);
  int32_t s_B = log2(shrB);
  int32_t s_C = log2(shrB);

  uint64_t *tmpA = new uint64_t[I * J];
  typecast_to_uint64(A, tmpA, I, J, bwA);

  uint64_t *tmpB = new uint64_t[I * J];
  typecast_to_uint64(B, tmpB, I, J, bwA);

  uint64_t *tmpC = new uint64_t[I * J];
  Div(tmpA, tmpB, tmpC, I, J, bwA, bwA, bwA, s_A, s_B, s_C);
  typecast_from_uint64(tmpC, C, I, J, bwA);

  delete[] tmpA;
  delete[] tmpB;
  delete[] tmpC;

#ifdef VERIFY_LAYERWISE
  int64_t *recA = new int64_t[I * J];
  int64_t *recB = new int64_t[I * J];
  int64_t *recC = new int64_t[I * J];
  int64_t *correctC = new int64_t[I * J];
  reconstruct(A, recA, I, J, bwA);
  reconstruct(B, recB, I, J, bwA);
  reconstruct(C, recC, I, J, bwA);

  if (party == 2)
  {
    cleartext_div(recA, recB, I, J, shrA, shrB, shrC, correctC, false);
    bool pass = true;
    for (int i = 0; i < I * J; i++)
    {
      if (recC[i] != correctC[i])
      {
        pass = false;
      }
      // std::cout << recA[i]/double(1LL << s_A) << "\t" << recB[i]/double(1LL
      // << s_B) << "\t" << recC[i]/double(1LL << s_C) << "\t" <<
      // correctC[i]/double(1LL << s_C) << std::endl;
    }
    if (pass == true)
      std::cout << GREEN << "Div Output Matches" << RESET << std::endl;
    else
      std::cout << RED << "Div Output Mismatch" << RESET << std::endl;
  }

  delete[] recA;
  delete[] recB;
  delete[] recC;
  delete[] correctC;
#endif
  return;
}

void primihub::cryptflow2::output_vector(int64_t *x, int32_t I, int32_t J, int32_t bwX)
{
  int64_t *y = new int64_t[I * J];
  reconstruct(x, y, I, J, bwX);
  if (party == 2)
  {
    for (int i = 0; i < I * J; i++)
    {
      std::cout << y[i] << " ";
    }
    std::cout << std::endl;
  }
  delete[] y;
}
