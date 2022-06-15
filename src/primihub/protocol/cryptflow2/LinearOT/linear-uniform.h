/*
Authors: Nishant Kumar
Copyright:
Copyright (c) 2020 Microsoft Research
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

#ifndef LINEAR_UNIFORM_H__
#define LINEAR_UNIFORM_H__
#include <iostream>

#ifdef USE_EIGEN
#include <Eigen/Dense>
#endif

#include "src/primihub/protocol/cryptflow2/OT/iknp.h"

namespace primihub::cryptflow2
{
  // Special case of LinearOT which works only for uniform bitwidth multiplication
  // Faster than LinearOT for uniform bitwidth matrix multiplication
  template <typename IO, typename intType, typename otType>
  class MatMulUniform
  {
  public:
    IO *io = nullptr;
    primihub::sci::OT<otType> *otImpl = nullptr;
    primihub::sci::OT<otType> *otImplRoleReversed = nullptr;
    int party;
    int bitlength;
    const uint32_t batchSizeOTs =
        1ULL << 18; // This is the default size of the batch of OTs that will be
                    // done in one go for matmul
                    // This will be scaled appropriately to manage memory if the
                    // matmul dimensions are too large.

    const uint64_t MaxMemToUseInBytes = 2.5 * (1 << 30); // 2.5 GiB
    intType moduloMask;

    MatMulUniform(int party, int bitlength, IO *io, primihub::sci::OT<otType> *otImpl,
                  primihub::sci::OT<otType> *otImplRoleReversed)
    {
      this->party = party;
      assert(((party == 1) || (party == 2)) && "PartyNum should be 1 or 2.");
      this->bitlength = bitlength;
      this->io = io;
      assert(io != nullptr && "IO can't be nullptr.");
      assert(otImpl != nullptr && "otImpl can't be nullptr.");
      this->otImpl = otImpl;
      this->otImplRoleReversed = otImplRoleReversed;
      if (bitlength == 64)
        moduloMask = -1;
      else
        moduloMask = (1ULL << bitlength) - 1;
    }

    ~MatMulUniform() {}

#ifdef USE_EIGEN
    void ideal_func_eigen(int s1, int s2, int s3, const intType *A,
                          const intType *B, intType *C)
    {
      Eigen::Matrix<intType, Eigen::Dynamic, Eigen::Dynamic> eigen_a(s1, s2);
      Eigen::Matrix<intType, Eigen::Dynamic, Eigen::Dynamic> eigen_b(s2, s3);
      Eigen::Matrix<intType, Eigen::Dynamic, Eigen::Dynamic> eigen_c(s1, s3);

      for (int i = 0; i < s1; i++)
      {
        for (int j = 0; j < s2; j++)
        {
          eigen_a(i, j) = Arr2DIdxRowM(A, s1, s2, i, j);
        }
      }
      for (int i = 0; i < s2; i++)
      {
        for (int j = 0; j < s3; j++)
        {
          eigen_b(i, j) = Arr2DIdxRowM(B, s2, s3, i, j);
        }
      }
      eigen_c = eigen_a * eigen_b;
      for (int i = 0; i < s1; i++)
      {
        for (int j = 0; j < s3; j++)
        {
          Arr2DIdxRowM(C, s1, s3, i, j) = eigen_c(i, j);
        }
      }
    }
#endif

    void ideal_func(int s1, int s2, int s3, const intType *A, const intType *B,
                    intType *C)
    {
#ifndef USE_EIGEN
      for (int i = 0; i < s1; i++)
      {
        for (int j = 0; j < s3; j++)
        {
          Arr2DIdxRowM(C, s1, s3, i, j) = 0;
          for (int k = 0; k < s2; k++)
          {
            Arr2DIdxRowM(C, s1, s3, i, j) +=
                (Arr2DIdxRowM(A, s1, s2, i, k) * Arr2DIdxRowM(B, s2, s3, k, j));
          }
        }
      }
#else
      ideal_func_eigen(s1, s2, s3, A, B, C);
#endif
      for (int i = 0; i < s1 * s3; i++)
      {
        C[i] = C[i] & moduloMask;
      }
    }

    void verifyMatmulShares(int s1, int s2, int s3, const intType *A_share,
                            const intType *B_share, const intType *C_share)
    {
      if (party == primihub::sci::ALICE)
      {
        intType *A_temp_share = new intType[s1 * s2];
        intType *B_temp_share = new intType[s2 * s3];
        intType *C_temp_share = new intType[s1 * s3];
        intType *C_clear = new intType[s1 * s3];
        io->recv_data(A_temp_share, sizeof(intType) * s1 * s2);
        io->recv_data(B_temp_share, sizeof(intType) * s2 * s3);
        io->recv_data(C_temp_share, sizeof(intType) * s1 * s3);
        primihub::sci::elemWiseAdd<intType>(s1 * s2, A_share, A_temp_share, A_temp_share);
        primihub::sci::elemWiseAdd<intType>(s2 * s3, B_share, B_temp_share, B_temp_share);
        primihub::sci::elemWiseAdd<intType>(s1 * s3, C_share, C_temp_share, C_temp_share);
        ideal_func(s1, s2, s3, A_temp_share, B_temp_share, C_clear);
        for (int i = 0; i < s1; i++)
        {
          for (int j = 0; j < s3; j++)
          {
            assert(Arr2DIdxRowM(C_clear, s1, s3, i, j) ==
                   (Arr2DIdxRowM(C_temp_share, s1, s3, i, j) & moduloMask));
          }
        }
        delete[] A_temp_share;
        delete[] B_temp_share;
        delete[] C_temp_share;
        delete[] C_clear;
      }
      else if (party == primihub::sci::BOB)
      {
        io->send_data(A_share, sizeof(intType) * s1 * s2);
        io->send_data(B_share, sizeof(intType) * s2 * s3);
        io->send_data(C_share, sizeof(intType) * s1 * s3);
      }
      else
      {
        assert(false);
      }
    }

    void fillInSimpleValues(int s1, int s2, intType *arr)
    {
      if (party == primihub::sci::ALICE)
      {
        for (int i = 0; i < s1; i++)
        {
          for (int j = 0; j < s2; j++)
          {
            Arr2DIdxRowM(arr, s1, s2, i, j) = i + j + 1;
          }
        }
      }
      else if (party == primihub::sci::BOB)
      {
        for (int i = 0; i < s1; i++)
        {
          for (int j = 0; j < s2; j++)
          {
            Arr2DIdxRowM(arr, s1, s2, i, j) = 0;
          }
        }
      }
    }

    /*
            This function is not being used anywhere right now
            - matmul where dimensions of matrices are (s1,s2) and (s2,s3)
            - inp is input of the sender of size (s1,s2)
            - outp is the share of the result of size (s1,s3)
    */
    void funcOTSenderNonBatched(int s1, int s2, int s3, const intType *inp,
                                intType *outp, primihub::sci::OT<otType> *otInstance)
    {
      uint64_t *corrData = new uint64_t[s1 * s2 * bitlength * s3];
      uint64_t *rData = new uint64_t[s1 * s2 * bitlength * s3];
      uint64_t *chunkSizes = new uint64_t[s2 * bitlength * s3];
      uint64_t *numChunks = new uint64_t[s2 * bitlength * s3];

      intType *inpColumnMajor = new intType[s1 * s2];
      intType *outpColumnMajor = new intType[s1 * s3];
      primihub::sci::convertRowToColMajor<intType>(s1, s2, inp, inpColumnMajor);

      for (int i = 0; i < s1 * s3; i++)
      {
        outpColumnMajor[i] = 0;
      }
      for (int col = 0; col < s3; col++)
      {
        for (int j = 0; j < s2; j++)
        {
          for (int k = 0; k < bitlength; k++)
          {
            for (int i = 0; i < s1; i++)
            {
              intType curElem = Arr2DIdxColM(inpColumnMajor, s1, s2, i, j);
              Arr4DIdxRowM(corrData, s3, s2, bitlength, s1, col, j, k, i) =
                  ((intType)(((intType)curElem) << k)) >> k;
              // casting is imp to make sure that
              // right shift happens in apt bitwidth
            }
            chunkSizes[col * s2 * bitlength + j * bitlength + k] = bitlength - k;
            numChunks[col * s2 * bitlength + j * bitlength + k] = s1;
          }
        }
      }
      otInstance->template send_cot_matmul<intType>(
          rData, corrData, chunkSizes, numChunks, s2 * bitlength * s3);
      for (int col = 0; col < s3; col++)
      {
        for (int i = 0; i < s2; i++)
        {
          for (int j = 0; j < bitlength; j++)
          {
            for (int k = 0; k < s1; k++)
            {
              // Arr3DIdxRowM(rData,s2,bitlength,s1,i,j,k) should be of
              // bitlength-j bits -- top j bits should be 0.
              uint64_t curElem =
                  Arr4DIdxRowM(rData, s3, s2, bitlength, s1, col, i, j, k);
              assert(((((1ULL << j) - 1) << (bitlength - j)) & curElem) == 0 &&
                     "assertion failed");
              Arr2DIdxColM(outpColumnMajor, s1, s3, k, col) -=
                  (((intType)curElem) << j);
            }
          }
        }
      }

      primihub::sci::convertColToRowMajor<intType>(s1, s3, outpColumnMajor, outp);
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] corrData;
      delete[] rData;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    /*
            This function is not being used anywhere right now
            - The matrix multiplication being performed is (s1, s2) * (s2, s3).
            - inp is the receiver's array to be multiplied of size (s2, s3)
            - outp is the share of the receiver after multiplication of size
       (s1,s3)
    */
    void funcOTReceiverNonBatched(int s1, int s2, int s3, const intType *inp,
                                  intType *outp, primihub::sci::OT<otType> *otInstance)
    {
      // copy inp from row to column and outp from column to row major
      uint8_t *choiceBitArr = new uint8_t[s2 * bitlength * s3];
      uint64_t *recv_data = new uint64_t[s1 * s2 * bitlength * s3];
      uint64_t *chunkSizes = new uint64_t[s2 * bitlength * s3];
      uint64_t *numChunks = new uint64_t[s2 * bitlength * s3];

      intType *inpColumnMajor = new intType[s2 * s3];
      intType *outpColumnMajor = new intType[s1 * s3];
      primihub::sci::convertRowToColMajor<intType>(s2, s3, inp, inpColumnMajor);

      for (int j = 0; j < s3; j++)
      {
        for (int i = 0; i < s2; i++)
        {
          for (int k = 0; k < bitlength; k++)
          {
            choiceBitArr[j * s2 * bitlength + i * bitlength + k] =
                (Arr2DIdxColM(inpColumnMajor, s2, s3, i, j) & (1ULL << k)) >>
                k; // Unsigned right shift
            chunkSizes[j * s2 * bitlength + i * bitlength + k] = (bitlength - k);
            numChunks[j * s2 * bitlength + i * bitlength + k] = s1;
          }
        }
      }
      otInstance->template recv_cot_matmul<intType>(
          recv_data, choiceBitArr, chunkSizes, numChunks, s2 * s3 * bitlength);
      // recv_data can be interpreted as a 4d array of size(s3,s2,bitlength,s1)
      for (int i = 0; i < s1 * s3; i++)
      {
        outpColumnMajor[i] = 0;
      }

      for (int i = 0; i < s3; i++)
      {
        for (int j = 0; j < s2; j++)
        {
          for (int k = 0; k < bitlength; k++)
          {
            for (int w = 0; w < s1; w++)
            {
              Arr2DIdxColM(outpColumnMajor, s1, s3, w, i) +=
                  (((intType)Arr4DIdxRowM(recv_data, s3, s2, bitlength, s1, i, j,
                                          k, w))
                   << k);
            }
          }
        }
      }

      primihub::sci::convertColToRowMajor<intType>(s1, s3, outpColumnMajor, outp);
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] choiceBitArr;
      delete[] recv_data;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    int chooseOptimalBatchSize(int senderMatmulDims)
    {
      uint64_t temp =
          MaxMemToUseInBytes / ((uint64_t)senderMatmulDims * sizeof(intType));
      if (temp > batchSizeOTs)
      {
        temp = batchSizeOTs;
      }
      return temp;
    }

    /*
            - matmul where dimensions of matrices are (s1,s2) and (s2,s3)
            - inp is input of the sender of size (s1,s2)
            - outp is the share of the result of size (s1,s3)
    */
    void funcOTSenderInputA(int s1, int s2, int s3, const intType *inp,
                            intType *outp, primihub::sci::OT<otType> *otInstance,
                            bool inpAlreadyColumnMajor = false)
    {
      intType *inpColumnMajor = const_cast<intType *>(inp);
      if (!inpAlreadyColumnMajor)
      {
        inpColumnMajor = new intType[s1 * s2];
        primihub::sci::convertRowToColMajor<intType>(s1, s2, inp, inpColumnMajor);
      }
      intType *outpColumnMajor = new intType[s1 * s3];
      int curBatchSizeOTs = chooseOptimalBatchSize(s1);

      intType *corrData = new intType[curBatchSizeOTs * s1];
      intType *data = new intType[curBatchSizeOTs * s1];
      uint32_t *chunkSizes = new uint32_t[curBatchSizeOTs];
      uint32_t *numChunks = new uint32_t[curBatchSizeOTs];

      for (int i = 0; i < s1 * s3; i++)
      {
        outpColumnMajor[i] = 0;
      }
      int numOTs = s2 * s3 * bitlength;
      int bitIdxRecv = 0, rowIdxRecv = 0, colIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s3, s2, bitlength, colIdxRecv,
                                                  rowIdxRecv, bitIdxRecv);
          for (int k = 0; k < s1; k++)
          {
            corrData[(j - i) * s1 + k] =
                (((intType)Arr2DIdxColM(inpColumnMajor, s1, s2, k, rowIdxRecv))
                 << bitIdxRecv) >>
                bitIdxRecv;
          }
          chunkSizes[j - i] = bitlength - bitIdxRecv;
          numChunks[j - i] = s1;
        }
        otInstance->template send_cot_matmul<intType>(data, corrData, chunkSizes,
                                                      numChunks, j - i, s1);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s3, s2, bitlength, colIdxRecv,
                                                  rowIdxRecv, bitIdxRecv);
          for (int k = 0; k < s1; k++)
          {
            Arr2DIdxColM(outpColumnMajor, s1, s3, k, colIdxRecv) -=
                (((intType)Arr2DIdxRowM(data, curBatchSizeOTs, s1, (j - i), k))
                 << bitIdxRecv);
          }
        }
      }

      primihub::sci::convertColToRowMajor<intType>(s1, s3, outpColumnMajor, outp);
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      if (!inpAlreadyColumnMajor)
        delete[] inpColumnMajor;
      delete[] outpColumnMajor;

      delete[] corrData;
      delete[] data;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    /*
            - The matrix multiplication being performed is (s1, s2) * (s2, s3).
            - inp is the receiver's array to be multiplied of size (s2, s3)
            - outp is the share of the receiver after multiplication of size
       (s1,s3)
    */
    void funcOTReceiverInputB(int s1, int s2, int s3, const intType *inp,
                              intType *outp, primihub::sci::OT<otType> *otInstance,
                              bool inpAlreadyColumnMajor = false)
    {
      // copy inp from row to column and outp from column to row major
      intType *inpColumnMajor = const_cast<intType *>(inp);
      intType *outpColumnMajor = new intType[s1 * s3];
      if (!inpAlreadyColumnMajor)
      {
        inpColumnMajor = new intType[s2 * s3];
        primihub::sci::convertRowToColMajor<intType>(s2, s3, inp, inpColumnMajor);
      }
      uint64_t masks[64];
      for (int i = 0; i < 64; i++)
      {
        masks[i] = 1ULL << i;
      }

      int curBatchSizeOTs = chooseOptimalBatchSize(s1);

      intType *data = new intType[curBatchSizeOTs * s1];
      uint8_t *choiceBitArr = new uint8_t[curBatchSizeOTs];
      uint32_t *chunkSizes = new uint32_t[curBatchSizeOTs];
      uint32_t *numChunks = new uint32_t[curBatchSizeOTs];

      for (int i = 0; i < s1 * s3; i++)
      {
        outpColumnMajor[i] = 0;
      }

      int numOTs = s2 * s3 * bitlength;
      int bitIdxRecv = 0, rowIdxRecv = 0, colIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s3, s2, bitlength, colIdxRecv,
                                                  rowIdxRecv, bitIdxRecv);
          choiceBitArr[j - i] =
              (Arr2DIdxColM(inpColumnMajor, s2, s3, rowIdxRecv, colIdxRecv) &
               masks[bitIdxRecv]) >>
              bitIdxRecv;
          chunkSizes[j - i] = bitlength - bitIdxRecv;
          numChunks[j - i] = s1;
        }
        otInstance->template recv_cot_matmul<intType>(
            data, choiceBitArr, chunkSizes, numChunks, j - i, s1);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s3, s2, bitlength, colIdxRecv,
                                                  rowIdxRecv, bitIdxRecv);
          for (int k = 0; k < s1; k++)
          {
            Arr2DIdxColM(outpColumnMajor, s1, s3, k, colIdxRecv) +=
                (((intType)Arr2DIdxRowM(data, curBatchSizeOTs, s1, (j - i), k))
                 << bitIdxRecv);
          }
        }
      }

      primihub::sci::convertColToRowMajor<intType>(s1, s3, outpColumnMajor, outp);
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      if (!inpAlreadyColumnMajor)
        delete[] inpColumnMajor;
      delete[] outpColumnMajor;

      delete[] data;
      delete[] choiceBitArr;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    /*
            - matmul where dimensions of matrices are (s1,s2) and (s2,s3)
            - inp is input of the sender of size (s2,s3)
            - outp is the share of the result of size (s1,s3)
    */
    void funcOTSenderInputB(int s1, int s2, int s3, const intType *inp,
                            intType *outp, primihub::sci::OT<otType> *otInstance)
    {
      int curBatchSizeOTs = chooseOptimalBatchSize(s3);

      intType *corrData = new intType[curBatchSizeOTs * s3];
      intType *data = new intType[curBatchSizeOTs * s3];
      uint32_t *chunkSizes = new uint32_t[curBatchSizeOTs];
      uint32_t *numChunks = new uint32_t[curBatchSizeOTs];

      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = 0;
      }

      int numOTs = s1 * s2 * bitlength;
      int bitIdxRecv = 0, rowIdxRecv = 0, colIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s1, s2, bitlength, rowIdxRecv,
                                                  colIdxRecv, bitIdxRecv);
          for (int k = 0; k < s3; k++)
          {
            corrData[(j - i) * s3 + k] =
                (((intType)Arr2DIdxRowM(inp, s2, s3, colIdxRecv, k))
                 << bitIdxRecv) >>
                bitIdxRecv;
          }
          chunkSizes[j - i] = bitlength - bitIdxRecv;
          numChunks[j - i] = s3;
        }
        otInstance->template send_cot_matmul<intType>(data, corrData, chunkSizes,
                                                      numChunks, j - i, s3);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s1, s2, bitlength, rowIdxRecv,
                                                  colIdxRecv, bitIdxRecv);
          for (int k = 0; k < s3; k++)
          {
            Arr2DIdxRowM(outp, s1, s3, rowIdxRecv, k) -=
                (((intType)Arr2DIdxRowM(data, curBatchSizeOTs, s3, (j - i), k))
                 << bitIdxRecv);
          }
        }
      }
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] corrData;
      delete[] data;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    /*
            - The matrix multiplication being performed is (s1, s2) * (s2, s3).
            - inp is the receiver's array to be multiplied of size (s1, s2)
            - outp is the share of the receiver after multiplication of size
       (s1,s3)
    */
    void funcOTReceiverInputA(int s1, int s2, int s3, const intType *inp,
                              intType *outp, primihub::sci::OT<otType> *otInstance)
    {
      uint64_t masks[64];
      for (int i = 0; i < 64; i++)
      {
        masks[i] = 1ULL << i;
      }

      int curBatchSizeOTs = chooseOptimalBatchSize(s3);

      intType *data = new intType[curBatchSizeOTs * s3];
      uint8_t *choiceBitArr = new uint8_t[curBatchSizeOTs];
      uint32_t *chunkSizes = new uint32_t[curBatchSizeOTs];
      uint32_t *numChunks = new uint32_t[curBatchSizeOTs];

      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = 0;
      }

      int numOTs = s1 * s2 * bitlength;
      int bitIdxRecv = 0, rowIdxRecv = 0, colIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s1, s2, bitlength, rowIdxRecv,
                                                  colIdxRecv, bitIdxRecv);
          choiceBitArr[j - i] =
              (Arr2DIdxRowM(inp, s1, s2, rowIdxRecv, colIdxRecv) &
               masks[bitIdxRecv]) >>
              bitIdxRecv;
          chunkSizes[j - i] = bitlength - bitIdxRecv;
          numChunks[j - i] = s3;
        }
        otInstance->template recv_cot_matmul<intType>(
            data, choiceBitArr, chunkSizes, numChunks, j - i, s3);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, s1, s2, bitlength, rowIdxRecv,
                                                  colIdxRecv, bitIdxRecv);
          for (int k = 0; k < s3; k++)
          {
            Arr2DIdxRowM(outp, s1, s3, rowIdxRecv, k) +=
                (((intType)Arr2DIdxRowM(data, curBatchSizeOTs, s3, (j - i), k))
                 << bitIdxRecv);
          }
        }
      }
      for (int i = 0; i < s1 * s3; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] data;
      delete[] choiceBitArr;
      delete[] chunkSizes;
      delete[] numChunks;
    }

    /*
            - Dot product functionality
            - Sender has input 'inp' of size 'size'
    */
    void funcDotProdOTSender(int size, const intType *inp, intType *outp,
                             primihub::sci::OT<otType> *otInstance)
    {
      for (int i = 0; i < size; i++)
      {
        outp[i] = 0;
      }
      int curBatchSizeOTs = batchSizeOTs;
      int numOTs = size * bitlength;
      intType *curCorrData = new intType[curBatchSizeOTs];
      uint32_t *curChunkSizes = new uint32_t[curBatchSizeOTs];
      uint32_t *curNumChunks = new uint32_t[curBatchSizeOTs];
      intType *curData = new intType[curBatchSizeOTs];
      int bitIdxRecv = 0, cellIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, size, bitlength, cellIdxRecv,
                                                  bitIdxRecv);
          curCorrData[j - i] =
              (((intType)inp[cellIdxRecv]) << bitIdxRecv) >> bitIdxRecv;
          curChunkSizes[j - i] = bitlength - bitIdxRecv;
          curNumChunks[j - i] = 1;
        }
        otInstance->template send_cot_matmul<intType>(
            curData, curCorrData, curChunkSizes, curNumChunks, j - i, size);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, size, bitlength, cellIdxRecv,
                                                  bitIdxRecv);
          outp[cellIdxRecv] -= (((intType)curData[j - i]) << bitIdxRecv);
        }
      }
      for (int i = 0; i < size; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] curCorrData;
      delete[] curChunkSizes;
      delete[] curNumChunks;
      delete[] curData;
    }

    /*
            - Dot product functionality
            - Receiver has input 'inp' of size 'size'
    */
    void funcDotProdOTReceiver(int size, const intType *inp, intType *outp,
                               primihub::sci::OT<otType> *otInstance)
    {
      uint64_t masks[64];
      for (int i = 0; i < 64; i++)
      {
        masks[i] = 1ULL << i;
      }
      for (int i = 0; i < size; i++)
      {
        outp[i] = 0;
      }
      int curBatchSizeOTs = batchSizeOTs;
      int numOTs = size * bitlength;
      uint8_t *curChoiceBits = new uint8_t[curBatchSizeOTs];
      uint32_t *curNumChunks = new uint32_t[curBatchSizeOTs];
      uint32_t *curChunkSizes = new uint32_t[curBatchSizeOTs];
      intType *curData = new intType[curBatchSizeOTs];
      int bitIdxRecv = 0, cellIdxRecv = 0;
      for (int i = 0; i < numOTs; i += curBatchSizeOTs)
      {
        int j;
        for (j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, size, bitlength, cellIdxRecv,
                                                  bitIdxRecv);
          curChoiceBits[j - i] =
              (inp[cellIdxRecv] & masks[bitIdxRecv]) >> bitIdxRecv;
          curChunkSizes[j - i] = bitlength - bitIdxRecv;
          curNumChunks[j - i] = 1;
        }
        otInstance->template recv_cot_matmul<intType>(
            curData, curChoiceBits, curChunkSizes, curNumChunks, j - i, size);
        for (int j = i; (j < numOTs) && (j < i + curBatchSizeOTs); j++)
        {
          // current OT# is j
          primihub::sci::linIdxRowMInverseMapping(j, size, bitlength, cellIdxRecv,
                                                  bitIdxRecv);
          outp[cellIdxRecv] += (curData[j - i] << bitIdxRecv);
        }
      }
      for (int i = 0; i < size; i++)
      {
        outp[i] = outp[i] & moduloMask;
      }

      delete[] curChoiceBits;
      delete[] curNumChunks;
      delete[] curChunkSizes;
      delete[] curData;
    }

    /*
            This code is unoptimized and not being used anywhere currently.
            - Matrix triplet of size (s1,s2)*(s2,s3)
            - A_share, B_share, C_share are shares of A,B,C
            - shape(A_share) = (s1,s2)
            - shape(B_share) = (s2,s3)
            - shape(C_share) = (s1,s3)
    */
    void generateBeaverMatrixTriplet(int s1, int s2, int s3, primihub::sci::PRG128 prg,
                                     intType *A_share, intType *B_share,
                                     intType *C_share)
    {
      assert(otImplRoleReversed != nullptr);
      prg.random_data(A_share, s1 * s2 * sizeof(intType));
      prg.random_data(B_share, s2 * s3 * sizeof(intType));
      intType *temp = new intType[s1 * s3];
      if (party == primihub::sci::ALICE)
      {
        // The OTs can be done in parallel
        funcOTSenderInputA(s1, s2, s3, A_share, C_share, otImpl);
        funcOTReceiverInputB(s1, s2, s3, B_share, temp, otImplRoleReversed);
      }
      else if (party == primihub::sci::BOB)
      {
        funcOTReceiverInputB(s1, s2, s3, B_share, C_share, otImpl);
        funcOTSenderInputA(s1, s2, s3, A_share, temp, otImplRoleReversed);
      }
      else
      {
        assert(false);
      }

      primihub::sci::elemWiseAdd<intType>(s1 * s3, C_share, temp, C_share);
      ideal_func(s1, s2, s3, A_share, B_share, temp);
      primihub::sci::elemWiseAdd<intType>(s1 * s3, C_share, temp, C_share);
    }

    /*
            This code is unoptimized and not being used anywhere currently.
            - Run the online phase of beaver
            - Matrices to be multiplied are of size (s1,s2) and (s2,s3).
            - X_share => size = (s1,s2), share of X
            - Y_share => size = (s2,s3), share of Y
            - Z_share => size = (s1,s3), share of output Z=X*Y
            - A_share, B_share, C_share are shares of the beaver triplet (A,B,C)
    */
    void runBeaverOnlinePhase(int s1, int s2, int s3, const intType *X_share,
                              const intType *Y_share, intType *Z_share,
                              const intType *A_share, const intType *B_share,
                              const intType *C_share)
    {
      intType *E_share = new intType[s1 * s2];
      intType *F_share = new intType[s2 * s3];
      intType *E_temp_share = new intType[s1 * s2];
      intType *F_temp_share = new intType[s2 * s3];
      intType *Z_temp_share = new intType[s1 * s3];
      primihub::sci::elemWiseSub<intType>(s1 * s2, X_share, A_share, E_share);
      primihub::sci::elemWiseSub<intType>(s2 * s3, Y_share, B_share, F_share);

      if (party == primihub::sci::ALICE)
      {
        io->send_data(E_share, sizeof(intType) * s1 * s2);
        io->send_data(F_share, sizeof(intType) * s2 * s3);
        io->recv_data(E_temp_share, sizeof(intType) * s1 * s2);
        io->recv_data(F_temp_share, sizeof(intType) * s2 * s3);
      }
      else if (party == primihub::sci::BOB)
      {
        io->recv_data(E_temp_share, sizeof(intType) * s1 * s2);
        io->recv_data(F_temp_share, sizeof(intType) * s2 * s3);
        io->send_data(E_share, sizeof(intType) * s1 * s2);
        io->send_data(F_share, sizeof(intType) * s2 * s3);
      }
      else
      {
        assert(false);
      }

      // Add the shares of E and F to get E and F in the clear
      primihub::sci::elemWiseAdd<intType>(s1 * s2, E_share, E_temp_share, E_share);
      primihub::sci::elemWiseAdd<intType>(s2 * s3, F_share, F_temp_share, F_share);

      // Now E_share and F_share hold the clear values of E & F
      ideal_func(s1, s2, s3, E_share, Y_share, Z_temp_share);
      if (party == primihub::sci::ALICE)
      {
        ideal_func(s1, s2, s3, X_share, F_share, Z_share);
      }
      else if (party == primihub::sci::BOB)
      {
        primihub::sci::elemWiseSub<intType>(s1 * s2, X_share, E_share, E_temp_share);
        ideal_func(s1, s2, s3, E_temp_share, F_share, Z_share);
      }

      primihub::sci::elemWiseAdd<intType>(s1 * s3, Z_share, Z_temp_share, Z_share);
      primihub::sci::elemWiseAdd<intType>(s1 * s3, C_share, Z_share, Z_share);

      delete[] E_share;
      delete[] F_share;
      delete[] E_temp_share;
      delete[] F_temp_share;
      delete[] Z_temp_share;
    }
  };
}
#endif // LINEAR_UNIFORM_H__
