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

#ifndef FUNCTIONALITIES_UNIFORM_H__
#define FUNCTIONALITIES_UNIFORM_H__

#include "globals.h"
#include <cmath>
namespace primihub::cryptflow2
{
  void funcLocalTruncate(int s, intType *arr, int consSF)
  {
    if (party == SERVER)
    {
      for (int i = 0; i < s; i++)
      {
        arr[i] =
            static_cast<intType>((static_cast<signedIntType>(arr[i])) >> consSF);
      }
    }
    else if (party == CLIENT)
    {
      for (int i = 0; i < s; i++)
      {
        arr[i] = -static_cast<intType>((static_cast<signedIntType>(-arr[i])) >>
                                       consSF);
      }
    }
    else
    {
      assert(false);
    }
  }

  signedIntType getAnyRingSignedVal(intType x)
  {
    signedIntType ans = x;
    if (x > (prime_mod / 2))
    {
      ans = (x - prime_mod);
    }
    return ans;
  }

  intType funcSSCons(int32_t x)
  {
    /*
            Secret share public value x between the two parties.
            Corresponding ezpc statement would be int32_al x = 0;
            Set one party share as x and other party's share as 0.
    */
    if (party == SERVER)
    {
      return x;
    }
    else
    {
      return 0;
    }
  }

  intType funcSSCons(int64_t x)
  {
    /*
            Secret share public value x between the two parties.
            Corresponding ezpc statement would be int32_al x = 0;
            Set one party share as x and other party's share as 0.
    */
    if (party == SERVER)
    {
      return x;
    }
    else
    {
      return 0;
    }
  }

  inline intType getFieldMsb(intType x) { return (x > (prime_mod / 2)); }

  void funcReconstruct2PCCons(signedIntType *y, const intType *x, int len)
  {
    intType temp = 0;
    signedIntType ans = 0;
    if (party == SERVER)
    {
      io->send_data(x, len * sizeof(intType));
    }
    else if (party == CLIENT)
    {
      io->recv_data(y, len * sizeof(intType));
      for (int i = 0; i < len; i++)
      {
        temp = y[i] + x[i];
        if (!isNativeRing)
        {
          temp = primihub::sci::neg_mod(temp,
                                        (int64_t)prime_mod); // cast temp to signed int and
                                                             // then take a proper modulo p
          y[i] = getAnyRingSignedVal(temp);
        }
        else
        {
          temp = temp & (prime_mod - 1);
          if (temp >= (prime_mod / 2))
            y[i] = temp - prime_mod;
          else
            y[i] = temp;
        }
      }
    }
    return;
  }

  signedIntType funcReconstruct2PCCons(intType x, int revealParty)
  {
    assert(revealParty == 2 && "Reveal to only client is supported right now.");
    intType temp = 0;
    signedIntType ans = 0;
    static const uint64_t moduloMask = primihub::sci::all1Mask(bitlength);
    if (party == SERVER)
    {
      io->send_data(&x, sizeof(intType));
    }
    else if (party == CLIENT)
    {
      io->recv_data(&temp, sizeof(intType));
      temp = temp + x;
      if (!isNativeRing)
      {
        temp =
            primihub::sci::neg_mod(temp, (int64_t)prime_mod); // cast temp to signed int and
                                                              // then take a proper modulo p
        ans = getAnyRingSignedVal(temp);
      }
      else
      {
        ans = temp & moduloMask;
      }
    }
    return ans;
  }

  signedIntType div_floor(signedIntType a, signedIntType b)
  {
    signedIntType q = a / b;
    signedIntType r = a % b;
    signedIntType corr = ((r != 0) && (r < 0));
    return q - corr;
  }

  void funcTruncationIdeal(int size, intType *arr, int consSF)
  {
    if (party == SERVER)
    {
      io->send_data(arr, sizeof(intType) * size);
      for (int i = 0; i < size; i++)
      {
        arr[i] = 0;
      }
    }
    else
    {
      intType *arrOther = new intType[size];
      io->recv_data(arrOther, sizeof(intType) * size);
      for (int i = 0; i < size; i++)
      {
        signedIntType ans;
        intType temp = arr[i] + arrOther[i];
        if (!isNativeRing)
        {
          temp = primihub::sci::neg_mod(temp, (int64_t)prime_mod);
          ans = getAnyRingSignedVal(temp);
          ans = div_floor(ans, 1ULL << consSF);
        }
        else
        {
          uint64_t moduloMask = (1ULL << bitlength) - 1;
          if (bitlength == 64)
            moduloMask = -1;
          temp = temp & moduloMask;
          ans = ((signedIntType)temp) >> consSF;
        }
        arr[i] = ans;
      }
    }
  }

  void printAllReconstructedValuesSigned(int size, intType *arr)
  {
    if (party == SERVER)
    {
      io->send_data(arr, sizeof(intType) * size);
    }
    else
    {
      intType *arrOther = new intType[size];
      io->recv_data(arrOther, sizeof(intType) * size);
      for (int i = 0; i < size; i++)
      {
        signedIntType ans;
        intType temp = arr[i] + arrOther[i];
        if (!isNativeRing)
        {
          temp = primihub::sci::neg_mod(temp, (int64_t)prime_mod);
          ans = getAnyRingSignedVal(temp);
        }
        else
        {
          ans = temp;
        }
        std::cout << ans << std::endl;
      }
    }
  }

  intType funcSigendDivIdeal(intType x, uint32_t y)
  {
    if (party == SERVER)
    {
      io->send_data(&x, sizeof(intType));
      return 0;
    }
    else
    {
      intType temp;
      io->recv_data(&temp, sizeof(intType));
      temp += x;
      return (((signedIntType)temp) / y);
    }
  }

  void funcReLUThread(int tid, intType *outp, intType *inp, int numRelu,
                      uint8_t *drelu_res = nullptr, bool skip_ot = false)
  {
    reluArr[tid]->relu(outp, inp, numRelu, drelu_res, skip_ot);
  }

  void funcMaxpoolThread(int tid, int rows, int cols, intType *inpArr,
                         intType *maxi, intType *maxiIdx)
  {
    maxpoolArr[tid]->funcMaxMPC(rows, cols, inpArr, maxi, maxiIdx);
  }

#ifdef SCI_OT
  void funcMatmulThread(int tid, int N, int s1, int s2, int s3, intType *A,
                        intType *B, intType *C, int partyWithAInAB_mul)
  {
    assert(tid >= 0);
    int bucket_size = std::ceil(s2 / (double)N);
    int s2StartIdx = tid * bucket_size;                   // Inclusive
    int s2EndIdx = std::min((tid + 1) * bucket_size, s2); // Exclusive

    if (s2StartIdx == s2EndIdx || s2StartIdx > s2)
    {
      memset(C, 0, s1 * s3 * sizeof(intType));
      return;
    }

    intType *APtr = new intType[s1 * (s2EndIdx - s2StartIdx)];
    for (int i = 0; i < s1; i++)
    {
      for (int j = s2StartIdx; j < s2EndIdx; j++)
      {
        Arr2DIdxRowM(APtr, s1, (s2EndIdx - s2StartIdx), i, (j - s2StartIdx)) =
            Arr2DIdxRowM(A, s1, s2, i, j);
      }
    }
    intType *BPtr = B + s2StartIdx * s3;
    /*
      Case 1: If Alice has A and Bob has B
      Then for even threads, Bob (holding B) acts as receiver and Alice
       (holding A) acts as sender For odd threads, Bob (holding B) acts as sender
       and Alice (holding A) acts as receiver.

      Case 2: If Alice has B and Bob has A
      Then for even threads, Bob (holding A) acts as receiver and Alice
      (holding B) as sender For odd threads, Bob (holding A) acts as sender and
      Alice (holding B) as receiver.

      Note that for even threads, Bob is always the
      receiver and Alice is always the sender and for odd threads, Bob is always
      the sender and Alice the receiver This makes sure that the matmul and
      otinstances (indexed by tid) are always used by with the same
      sender-receiver pair.
    */

#ifdef USE_LINEAR_UNIFORM
    assert(s2EndIdx > s2StartIdx);
    bool useBobAsSender = tid & 1;
    if (partyWithAInAB_mul == primihub::sci::BOB)
      useBobAsSender = !useBobAsSender;
    if (useBobAsSender)
    {
      // Odd tid, use Bob (holding B) as sender and Alice (holding A) as receiver
      if (party == partyWithAInAB_mul)
      {
        multUniformArr[tid]->funcOTReceiverInputA(s1, (s2EndIdx - s2StartIdx), s3,
                                                  APtr, C, otInstanceArr[tid]);
      }
      else
      {
        multUniformArr[tid]->funcOTSenderInputB(s1, (s2EndIdx - s2StartIdx), s3,
                                                BPtr, C, otInstanceArr[tid]);
      }
    }
    else
    {
      // Even tid, use Bob (holding B) as receiver and Alice (holding A) as sender
      if (party == partyWithAInAB_mul)
      {
        multUniformArr[tid]->funcOTSenderInputA(s1, (s2EndIdx - s2StartIdx), s3,
                                                APtr, C, otInstanceArr[tid]);
      }
      else
      {
        multUniformArr[tid]->funcOTReceiverInputB(s1, (s2EndIdx - s2StartIdx), s3,
                                                  BPtr, C, otInstanceArr[tid]);
      }
    }
#else // USE_LINEAR_UNIFORM
    MultMode mode;
#ifdef TRAINING
    mode = MultMode::None;
#else
    if (tid & 1)
    {
      if (partyWithAInAB_mul == primihub::sci::ALICE)
        mode = MultMode::Bob_has_A;
      else
        mode = MultMode::Bob_has_B;
    }
    else
    {
      if (partyWithAInAB_mul == primihub::sci::ALICE)
        mode = MultMode::Alice_has_A;
      else
        mode = MultMode::Alice_has_B;
    }
#endif
    if (s2EndIdx > s2StartIdx)
    {
      multArr[tid]->matmul_cross_terms(s1, (s2EndIdx - s2StartIdx), s3, APtr,
                                       BPtr, C, bitlength, bitlength, bitlength,
                                       true, mode);
    }
    else
    {
      memset(C, 0, s1 * s3 * sizeof(intType));
    }
#endif // USE_LINEAR_UNIFORM
    delete[] APtr;
  }

  void funcDotProdThread(int tid, int N, int size, intType *multiplyArr,
                         intType *inArr, intType *outArr,
                         bool both_cross_terms = false)
  {
    if (size == 0)
      return;
    assert(tid >= 0);
#ifdef TRAINING
    multArr[tid]->hadamard_cross_terms(size, multiplyArr, inArr, outArr,
                                       bitlength, bitlength, bitlength, MultMode::None);
#else
    if (tid & 1)
    {
      MultMode mode = (both_cross_terms ? MultMode::None : MultMode::Bob_has_A);
      multArr[tid]->hadamard_cross_terms(size, multiplyArr, inArr, outArr,
                                         bitlength, bitlength, bitlength, mode);
    }
    else
    {
      MultMode mode = (both_cross_terms ? MultMode::None : MultMode::Alice_has_A);
      multArr[tid]->hadamard_cross_terms(size, multiplyArr, inArr, outArr,
                                         bitlength, bitlength, bitlength, mode);
    }
#endif
  }
#endif

  /*
          Note this assumes 1 <= s < ell
          More optimizations that could be done for this function:
          - The call to 1oo4 KKOT and 1oo2 IKNP results in 4 rounds. Round
     compression can potentially be done to reduce rounds to 2. The interface can
     also potentially be cleaned up to use only otpack. However, currently the COT
     required to be used is present in IKNP - so keeping that too in the
     interface.
  */
  void funcTruncateTwoPowerRing(
      int curParty, primihub::sci::IOPack *curiopack, primihub::sci::OTPack *curotpack,
      primihub::sci::IKNP<primihub::sci::NetIO> *curiknp, ReLUProtocol<intType> *curReluImpl,
      primihub::sci::PRG128 *curPrgInstance, int size, intType *inp, intType *outp,
      int consSF, uint8_t *msbShare, bool doCarryBitCalculation = true)
  {
    static const int rightShiftForMsb = bitlength - 1;
    uint64_t *carryBitCompArr;
    uint8_t *carryBitCompAns;
    uint64_t moduloMask = primihub::sci::all1Mask(bitlength);

    for (int i = 0; i < size; i++)
    {
      assert(inp[i] <= moduloMask);
    }

    if (doCarryBitCalculation)
    {
      carryBitCompArr = new uint64_t[size];
      carryBitCompAns = new uint8_t[size];
      for (int i = 0; i < size; i++)
      {
        carryBitCompArr[i] = (inp[i] & primihub::sci::all1Mask(consSF));
        if (curParty == primihub::sci::BOB)
        {
          carryBitCompArr[i] = (primihub::sci::all1Mask(consSF)) - carryBitCompArr[i];
        }
      }
      MillionaireProtocol millionaire(curParty, curiopack, curotpack);
      millionaire.compare(carryBitCompAns, carryBitCompArr, size, consSF);
    }

    bool createdMsbSharesHere = false;
    if (msbShare == nullptr)
    {
      msbShare = new uint8_t[size];
      curReluImpl->relu(nullptr, inp, size, msbShare, true);
      createdMsbSharesHere = true;
    }

    if (curParty == primihub::sci::ALICE)
    {
      uint64_t **otMessages1oo4 = new uint64_t *[size];
      intType *otMessages1oo2Data = new intType[size];
      intType *otMessages1oo2Corr = new intType[size];
      intType *localShare1oo4 = new intType[size];
      curPrgInstance->random_data(localShare1oo4, sizeof(intType) * size);

      for (int i = 0; i < size; i++)
      {
        otMessages1oo4[i] = new uint64_t[4];
        uint8_t localShareMSB = ((uint8_t)(inp[i] >> rightShiftForMsb));
        intType r = -localShare1oo4[i];
        for (int j = 0; j < 4; j++)
        {
          uint8_t b0 = j & 1;
          uint8_t b1 =
              (j >> 1) &
              1; // b1,b0 is the bit representation of j = [msb(a)]_1, msb(a_1)
          uint8_t temp = ((msbShare[i] + b1 + localShareMSB) & 1) &
                         ((msbShare[i] + b1 + b0) & 1);
          intType curMsg = r;
          if (temp & (localShareMSB == 0))
          {
            // msb(a_0)=0, msb(a_1)=0, msb(a)=1
            curMsg -= 1;
          }
          else if (temp & (localShareMSB == 1))
          {
            // msb(a_0)=1, msb(a_1)=1, msb(a)=0
            curMsg += 1;
          }
          // In other cases, extra term is 0
          otMessages1oo4[i][j] = (uint64_t)curMsg;
        }
        if (doCarryBitCalculation)
        {
          otMessages1oo2Corr[i] = ((intType)carryBitCompAns[i]);
        }
      }

      curotpack->kkot[1]->send_impl(otMessages1oo4, size, bitlength);
      if (doCarryBitCalculation)
      {
        curiknp->send_cot_moduloAdd<intType>(otMessages1oo2Data,
                                             otMessages1oo2Corr, size);
      }

      for (int i = 0; i < size; i++)
      {
        intType shareTerm1 = ((intType)((getAnyRingSignedVal(inp[i])) >> consSF));
        intType shareTerm2 = (localShare1oo4[i] << (bitlength - consSF));
        outp[i] = shareTerm1 + shareTerm2;
        if (doCarryBitCalculation)
        {
          intType curCarryShare = -otMessages1oo2Data[i];
          outp[i] = outp[i] + ((intType)carryBitCompAns[i]) - 2 * curCarryShare;
        }
        outp[i] = outp[i] & moduloMask;
        delete[] otMessages1oo4[i];
      }

      delete[] otMessages1oo4;
      delete[] otMessages1oo2Data;
      delete[] otMessages1oo2Corr;
      delete[] localShare1oo4;
    }
    else
    {
      uint64_t *otRecvMsg1oo4 = new uint64_t[size];
      uint8_t *choiceBits = new uint8_t[size];
      intType *otRecvMsg1oo2 = new intType[size];
      uint8_t *choiceBits1oo2OT = new uint8_t[size];
      for (int i = 0; i < size; i++)
      {
        uint8_t localShareMSB = ((uint8_t)(inp[i] >> rightShiftForMsb));
        choiceBits[i] = (msbShare[i] << 1) + localShareMSB;
        if (doCarryBitCalculation)
        {
          choiceBits1oo2OT[i] = carryBitCompAns[i];
        }
      }

      curotpack->kkot[1]->recv_impl(otRecvMsg1oo4, choiceBits, size, bitlength);
      if (doCarryBitCalculation)
      {
        curiknp->recv_cot_moduloAdd<intType>(otRecvMsg1oo2, choiceBits1oo2OT,
                                             size);
      }
      for (int i = 0; i < size; i++)
      {
        intType G1 = (intType)otRecvMsg1oo4[i];
        intType shareTerm1 = (getAnyRingSignedVal(inp[i])) >> consSF;
        intType shareTerm2 = (G1 << (bitlength - consSF));
        outp[i] = shareTerm1 + shareTerm2;
        if (doCarryBitCalculation)
        {
          outp[i] =
              outp[i] + ((intType)carryBitCompAns[i]) - 2 * otRecvMsg1oo2[i];
        }
        outp[i] = outp[i] & moduloMask;
      }

      delete[] otRecvMsg1oo4;
      delete[] choiceBits;
      delete[] otRecvMsg1oo2;
      delete[] choiceBits1oo2OT;
    }

    if (doCarryBitCalculation)
    {
      delete[] carryBitCompArr;
      delete[] carryBitCompAns;
    }

    if (createdMsbSharesHere)
    {
      delete[] msbShare;
    }
  }

  void funcTruncateTwoPowerRingWrapper(int size, intType *inp, intType *outp,
                                       int consSF, uint8_t *msbShare,
                                       bool doCarryBitCalculation = true)
  {
#ifdef MULTITHREADED_TRUNC
    std::thread truncThreads[num_threads];
    int chunk_size = size / num_threads;
    for (int i = 0; i < num_threads; i++)
    {
      int offset = i * chunk_size;
      int curSize;
      if (i == (num_threads - 1))
      {
        curSize = size - offset;
      }
      else
      {
        curSize = chunk_size;
      }
      int curParty = party;
      if (i & 1)
        curParty = 3 - curParty;
      uint8_t *msbShareArg = msbShare;
      if (msbShare != nullptr)
        msbShareArg = msbShareArg + offset;
      truncThreads[i] = std::thread(
          funcTruncateTwoPowerRing, curParty, iopackArr[i], otpackArr[i],
          otInstanceArr[i], reluArr[i], prgInstanceArr[i], curSize, inp + offset,
          outp + offset, consSF, msbShareArg, doCarryBitCalculation);
    }
    for (int i = 0; i < num_threads; ++i)
    {
      truncThreads[i].join();
    }
#else
    funcTruncateTwoPowerRing(party, iopack, otpack, iknpOT, kkot, relu,
                             prg128Instance, // Global variables
                             size, inp, outp, consSF, msbShare,
                             doCarryBitCalculation);
#endif
  }

  /*
          More optimizations that could be done for this function:
          - Currently if bitsForA + bitlength > 64, then the function resorts to
     using block128 based 1oo4 KKOT, skipping packing. This could be optimized to
     do ideal packing even in this case.
          - The last COT being performed is also not using packing for bitlenth !=
     32/64.
  */
  void funcAvgPoolTwoPowerRing(int curParty, primihub::sci::IOPack *curiopack,
                               primihub::sci::OTPack *curotpack,
                               primihub::sci::IKNP<primihub::sci::NetIO> *curiknp,
                               primihub::sci::KKOT<primihub::sci::NetIO> *curkkot,
                               ReLUProtocol<intType> *curReluImpl,
                               primihub::sci::PRG128 *curPrgInstance, int size,
                               intType *inp, intType *outp, intType divisor)
  {
    assert(inp != outp &&
           "Assumption is there is a separate array for input and output");
    assert(divisor > 0 && "working with positive divisor");
    static const int rightShiftForMsb = bitlength - 1;
    assert(primihub::sci::all1Mask(bitlength) >
           (6 * divisor - 1)); // 2^l-1 > 6d-1 => 2^l > 6d
    intType ringRem, ringQuot;
    if (bitlength == 64)
    {
      ringRem = ((intType)(-divisor)) % divisor;        //(2^l-d)%d = 2^l%d
      ringQuot = (((intType)(-divisor)) / divisor) + 1; //((2^l-d)/d)+1 = 2^l/d
    }
    else
    {
      ringRem = prime_mod % divisor;
      ringQuot = prime_mod / divisor;
    }
    uint8_t *msbShare = new uint8_t[size];
    uint64_t moduloMask = primihub::sci::all1Mask(bitlength);

    for (int i = 0; i < size; i++)
    {
      inp[i] = inp[i] & moduloMask;
      intType shareTerm1 = div_floor(getAnyRingSignedVal(inp[i]), divisor);
      intType localShareMSB = (inp[i] >> rightShiftForMsb);
      intType shareTerm2 = div_floor(
          (signedIntType)((inp[i] % divisor) - localShareMSB * ringRem), divisor);
      outp[i] = shareTerm1 - shareTerm2;
      if (curParty == primihub::sci::ALICE)
        outp[i] += 1;
    }

    curReluImpl->relu(nullptr, inp, size, msbShare, true);

    const uint64_t bitsForA = std::ceil(std::log2(6 * divisor));
    uint64_t totalRandomBytesForLargeRing = primihub::sci::ceil_val(bitlength * size, 8);
    uint8_t *localShareCorrPacked = new uint8_t[totalRandomBytesForLargeRing];
    intType *localShareCorr = new intType[size];
    int totalRandomBytesForSmallRing = primihub::sci::ceil_val(bitsForA * size, 8);
    uint8_t *localShareCorrSmallRingPacked =
        new uint8_t[totalRandomBytesForSmallRing];
    uint64_t *localShareCorrSmallRing = new uint64_t[size];
    intType *otMsgCorrRing = new intType[4 * size];
    intType *otMsgCorrSmallRing = new intType[4 * size];

    bool OT1oo4FitsIn64Bits = ((bitsForA + bitlength) <= 64);
    if (curParty == primihub::sci::ALICE)
    {
      curPrgInstance->random_data(localShareCorrPacked,
                                  totalRandomBytesForLargeRing);
      curPrgInstance->random_data(localShareCorrSmallRingPacked,
                                  totalRandomBytesForSmallRing);
      for (int i = 0; i < size; i++)
      {
        localShareCorr[i] = primihub::sci::readFromPackedArr(localShareCorrPacked,
                                                             totalRandomBytesForLargeRing,
                                                             i * (bitlength), bitlength);
        localShareCorrSmallRing[i] = primihub::sci::readFromPackedArr(
            localShareCorrSmallRingPacked, totalRandomBytesForSmallRing,
            i * bitsForA, bitsForA);
        uint8_t localShareMSB = ((uint8_t)(inp[i] >> rightShiftForMsb));
        for (int j = 0; j < 4; j++)
        {
          uint8_t b0 = j & 1;
          uint8_t b1 =
              (j >> 1) &
              1; // b1,b0 is the bit representation of j = [msb(a)]_1, msb(a_1)
          uint8_t temp = ((msbShare[i] + b1 + localShareMSB) & 1) &
                         ((msbShare[i] + b1 + b0) & 1);
          intType curMsg = -localShareCorr[i];
          intType curMsgSmallRing = -localShareCorrSmallRing[i];
          if (temp & (localShareMSB == 0))
          {
            // msb(a_0)=0, msb(a_1)=0, msb(a)=1
            curMsg -= 1;
            curMsgSmallRing -= 1;
          }
          else if (temp & (localShareMSB == 1))
          {
            // msb(a_0)=1, msb(a_1)=1, msb(a)=0
            curMsg += 1;
            curMsgSmallRing += 1;
          }
          curMsg = curMsg & moduloMask;
          curMsgSmallRing = curMsgSmallRing & primihub::sci::all1Mask(bitsForA);
          otMsgCorrRing[i * 4 + j] = curMsg;
          otMsgCorrSmallRing[i * 4 + j] = curMsgSmallRing;
        }
      }

      if (OT1oo4FitsIn64Bits)
      {
        uint64_t **otMessages1oo4 = new uint64_t *[size];
        for (int i = 0; i < size; i++)
        {
          otMessages1oo4[i] = new uint64_t[4];
          for (int j = 0; j < 4; j++)
            otMessages1oo4[i][j] =
                (((uint64_t)otMsgCorrSmallRing[i * 4 + j]) << bitlength) +
                ((uint64_t)otMsgCorrRing[i * 4 + j]);
        }
        curotpack->kkot[1]->send_impl(otMessages1oo4, size,
                                      (bitlength + bitsForA));
        for (int i = 0; i < size; i++)
        {
          delete[] otMessages1oo4[i];
        }
        delete[] otMessages1oo4;
      }
      else
      {
        primihub::sci::block128 **otMessages1oo4 = new primihub::sci::block128 *[size];
        for (int i = 0; i < size; i++)
        {
          otMessages1oo4[i] = new primihub::sci::block128[4];
          for (int j = 0; j < 4; j++)
            otMessages1oo4[i][j] = _mm_set_epi64x(otMsgCorrSmallRing[i * 4 + j],
                                                  otMsgCorrRing[i * 4 + j]);
        }
        curkkot->send_impl(otMessages1oo4, size, 4);
        for (int i = 0; i < size; i++)
        {
          delete[] otMessages1oo4[i];
        }
        delete[] otMessages1oo4;
      }
      for (int i = 0; i < size; i++)
      {
        outp[i] += (localShareCorr[i] * ringQuot);
      }
    }
    else
    {
      uint8_t *choiceBits = new uint8_t[size];
      for (int i = 0; i < size; i++)
      {
        uint8_t localShareMSB = ((uint8_t)(inp[i] >> rightShiftForMsb));
        choiceBits[i] = (msbShare[i] << 1) + localShareMSB;
      }

      if (OT1oo4FitsIn64Bits)
      {
        uint64_t *otRecvMsg1oo4 = new uint64_t[size];
        curotpack->kkot[1]->recv_impl(otRecvMsg1oo4, choiceBits, size,
                                      (bitlength + bitsForA));
        for (int i = 0; i < size; i++)
        {
          otMsgCorrRing[i] = otRecvMsg1oo4[i] & primihub::sci::all1Mask(bitlength);
          otMsgCorrSmallRing[i] = otRecvMsg1oo4[i] >> bitlength;
        }
        delete[] otRecvMsg1oo4;
      }
      else
      {
        primihub::sci::block128 *otRecvMsg1oo4 = new primihub::sci::block128[size];
        curkkot->recv_impl(otRecvMsg1oo4, choiceBits, size, 4);
        for (int i = 0; i < size; i++)
        {
          uint64_t temp = _mm_extract_epi64(otRecvMsg1oo4[i], 0);
          otMsgCorrRing[i] = temp;
          temp = _mm_extract_epi64(otRecvMsg1oo4[i], 1);
          otMsgCorrSmallRing[i] = temp;
        }
        delete[] otRecvMsg1oo4;
      }

      for (int i = 0; i < size; i++)
      {
        localShareCorr[i] = otMsgCorrRing[i];
        localShareCorrSmallRing[i] = otMsgCorrSmallRing[i];
        outp[i] += (localShareCorr[i] * ringQuot);
      }

      delete[] choiceBits;
    }

    intType *localShareA_all3 = new intType[3 * size];
    uint8_t *localShareA_all3_drelu = new uint8_t[3 * size];
    uint64_t bitsAmask = primihub::sci::all1Mask(bitsForA);
    uint64_t bitsAMinusOneMask = primihub::sci::all1Mask((bitsForA - 1));
    uint64_t *radixCompValues = new uint64_t[3 * size];
    uint8_t *carryBit = new uint8_t[3 * size];

    // Optimization to reduce comparions to 2
    int totalComp = 3 * size;
    int compPerElt = 3;
    if (2 * ringRem < divisor)
    {
      // A+d<0 becomes moot
      totalComp = 2 * size;
      compPerElt = 2;
    }

    for (int i = 0; i < size; i++)
    {
      intType localShareA = ((inp[i] % divisor) - ((inp[i] >> rightShiftForMsb) -
                                                   localShareCorrSmallRing[i]) *
                                                      ringRem) &
                            bitsAmask;
      for (int j = 0; j < compPerElt; j++)
      {
        localShareA_all3[compPerElt * i + j] = localShareA;
      }

      if (curParty == primihub::sci::ALICE)
      {
        if (compPerElt == 3)
        {
          localShareA_all3[3 * i] =
              (localShareA_all3[3 * i] - divisor) & bitsAmask;
          localShareA_all3[3 * i + 2] =
              (localShareA_all3[3 * i + 2] + divisor) & bitsAmask;
        }
        else
        {
          localShareA_all3[2 * i] =
              (localShareA_all3[2 * i] - divisor) & bitsAmask;
        }
      }

      for (int j = 0; j < compPerElt; j++)
      {
        radixCompValues[compPerElt * i + j] =
            (localShareA_all3[compPerElt * i + j] & bitsAMinusOneMask);
        localShareA_all3_drelu[compPerElt * i + j] =
            (localShareA_all3[compPerElt * i + j] >> (bitsForA - 1));
        if (curParty == primihub::sci::BOB)
        {
          radixCompValues[compPerElt * i + j] =
              bitsAMinusOneMask - radixCompValues[compPerElt * i + j];
        }
      }
    }

    MillionaireProtocol millionaire(curParty, curiopack, curotpack);
    millionaire.compare(carryBit, radixCompValues, totalComp, bitsForA - 1);
    for (int i = 0; i < totalComp; i++)
    {
      localShareA_all3_drelu[i] = (localShareA_all3_drelu[i] + carryBit[i]) & 1;
    }

    if (curParty == primihub::sci::ALICE)
    {
      intType *cotData = new intType[totalComp];
      intType *cotCorr = new intType[totalComp];
      for (int i = 0; i < totalComp; i++)
      {
        cotCorr[i] = (intType)localShareA_all3_drelu[i];
      }
      curiknp->send_cot_moduloAdd<intType>(cotData, cotCorr, totalComp);
      for (int i = 0; i < size; i++)
      {
        intType curCTermShare = 0;
        for (int j = 0; j < compPerElt; j++)
        {
          intType curMultTermShare = -cotData[compPerElt * i + j];
          intType curDReluAns =
              (intType)localShareA_all3_drelu[compPerElt * i + j] -
              2 * curMultTermShare;
          curCTermShare += curDReluAns;
        }
        outp[i] -= curCTermShare;
      }
      delete[] cotData;
      delete[] cotCorr;
    }
    else
    {
      intType *cotDataRecvd = new intType[totalComp];
      curiknp->recv_cot_moduloAdd<intType>(cotDataRecvd, localShareA_all3_drelu,
                                           totalComp);
      for (int i = 0; i < size; i++)
      {
        intType curCTermShare = 0;
        for (int j = 0; j < compPerElt; j++)
        {
          intType curMultTermShare = cotDataRecvd[compPerElt * i + j];
          intType curDReluAns =
              (intType)localShareA_all3_drelu[compPerElt * i + j] -
              2 * curMultTermShare;
          curCTermShare += curDReluAns;
        }
        outp[i] -= curCTermShare;
      }
      delete[] cotDataRecvd;
    }

    for (int i = 0; i < size; i++)
    {
      outp[i] = outp[i] & moduloMask;
    }

    delete[] msbShare;
    delete[] localShareCorr;
    delete[] localShareCorrSmallRingPacked;
    delete[] localShareCorrSmallRing;
    delete[] otMsgCorrRing;
    delete[] otMsgCorrSmallRing;
    delete[] localShareA_all3;
    delete[] localShareA_all3_drelu;
    delete[] radixCompValues;
    delete[] carryBit;
  }

  void funcAvgPoolTwoPowerRingWrapper(int size, intType *inp, intType *outp,
                                      intType divisor)
  {
#ifdef MULTITHREADED_TRUNC
    std::thread truncThreads[num_threads];
    int chunk_size = size / num_threads;
    for (int i = 0; i < num_threads; i++)
    {
      int offset = i * chunk_size;
      int curSize;
      if (i == (num_threads - 1))
      {
        curSize = size - offset;
      }
      else
      {
        curSize = chunk_size;
      }
      int curParty = party;
      if (i & 1)
        curParty = 3 - curParty;
      truncThreads[i] = std::thread(
          funcAvgPoolTwoPowerRing, curParty, iopackArr[i], otpackArr[i],
          otInstanceArr[i], kkotInstanceArr[i], reluArr[i], prgInstanceArr[i],
          curSize, inp + offset, outp + offset, divisor);
    }
    for (int i = 0; i < num_threads; ++i)
    {
      truncThreads[i].join();
    }
#else
    funcAvgPoolTwoPowerRing(party, iopack, otpack, iknpOT, kkot, relu,
                            prg128Instance, size, inp, outp, divisor);
#endif
  }

  /*
          Assume uint64_t for fields
          More optimizations that could be performed for this function:
          - The 1oo4 KKOT being performed resorts to using block128 and no packing
     when fieldBits + bitsForA > 64. This can be optimized to use ideal packing
     even in this case.
          - The last OT being performed is a COT, but is being done using an OT
     with no packing.
  */
  template <typename intType>
  void funcFieldDiv(int curParty, primihub::sci::IOPack *curiopack, primihub::sci::OTPack *curotpack,
                    primihub::sci::IKNP<primihub::sci::NetIO> *curiknp,
                    primihub::sci::KKOT<primihub::sci::NetIO> *curkkot,
                    ReLUProtocol<intType> *curReluImpl,
                    primihub::sci::PRG128 *curPrgInstance, int size, intType *inp,
                    intType *outp, intType divisor, uint8_t *msbShare)
  {
    assert(inp != outp &&
           "Assumption is there is a separate array for input and output");
    assert((divisor > 0) && (divisor < prime_mod) &&
           "working with positive divisor");
    assert(prime_mod > 6 * divisor);
    const intType ringRem = prime_mod % divisor;
    const intType ringQuot = prime_mod / divisor;
    bool doMSBComputation = (msbShare == nullptr);
    if (doMSBComputation)
      msbShare = new uint8_t[size];

    for (int i = 0; i < size; i++)
    {
      assert(inp[i] < prime_mod && "input is not a valid share modulo prime_mod");
      signedIntType shareTerm1 =
          div_floor((signedIntType)getAnyRingSignedVal(inp[i]), divisor);
      intType localShareMSB = getFieldMsb(inp[i]);
      signedIntType shareTerm2 = div_floor(
          (signedIntType)((inp[i] % divisor) - localShareMSB * ringRem), divisor);
      signedIntType temp = shareTerm1 - shareTerm2;
      if (curParty == primihub::sci::BOB)
        temp += 1;
      outp[i] = primihub::sci::neg_mod(temp, (int64_t)prime_mod);
    }

    if (doMSBComputation)
    {
      curReluImpl->relu(nullptr, inp, size, msbShare, true);
    }

    static const int fieldBits = std::ceil(std::log2(prime_mod));
    const uint64_t bitsForA = std::ceil(std::log2(6 * divisor));
    const uint64_t totalBitlen = fieldBits + bitsForA;
    bool OT1oo4FitsIn64Bits = (totalBitlen <= 64);

    intType *localShareCorr = new intType[size];
    int totalRandomBytesForSmallRing = primihub::sci::ceil_val(bitsForA * size, 8);
    uint8_t *localShareCorrSmallRingPacked =
        new uint8_t[totalRandomBytesForSmallRing];
    uint64_t *localShareCorrSmallRing = new uint64_t[size];
    intType *otMsgCorrField = new intType[4 * size];
    intType *otMsgCorrSmallRing = new intType[4 * size];

    if (curParty == primihub::sci::ALICE)
    {
      curPrgInstance->random_mod_p<intType>(localShareCorr, size, prime_mod);
      curPrgInstance->random_data(localShareCorrSmallRingPacked,
                                  totalRandomBytesForSmallRing);
      for (int i = 0; i < size; i++)
      {
        localShareCorrSmallRing[i] = primihub::sci::readFromPackedArr(
            localShareCorrSmallRingPacked, totalRandomBytesForSmallRing,
            i * bitsForA, bitsForA);
        uint8_t localShareMSB = getFieldMsb(inp[i]);
        for (int j = 0; j < 4; j++)
        {
          uint8_t b0 = j & 1;
          uint8_t b1 =
              (j >> 1) &
              1; // b1,b0 is the bit representation of j = [msb(a)]_1, msb(a_1)
          uint8_t temp = ((msbShare[i] + b1 + localShareMSB) & 1) &
                         ((msbShare[i] + b1 + b0) & 1);
          signedIntType curMsg = -localShareCorr[i];
          intType curMsgSmallRing = -localShareCorrSmallRing[i];
          if (temp & (localShareMSB == 0))
          {
            // msb(a_0)=0, msb(a_1)=0, msb(a)=1
            curMsg -= 1;
            curMsgSmallRing -= 1;
          }
          else if (temp & (localShareMSB == 1))
          {
            // msb(a_0)=1, msb(a_1)=1, msb(a)=0
            curMsg += 1;
            curMsgSmallRing += 1;
          }
          intType curMsgField = primihub::sci::neg_mod(curMsg, (int64_t)prime_mod);
          curMsgSmallRing = curMsgSmallRing & primihub::sci::all1Mask(bitsForA);
          otMsgCorrField[i * 4 + j] = curMsgField;
          otMsgCorrSmallRing[i * 4 + j] = curMsgSmallRing;
        }
      }
      if (OT1oo4FitsIn64Bits)
      {
        uint64_t **otMessages1oo4 = new uint64_t *[size];
        for (int i = 0; i < size; i++)
        {
          otMessages1oo4[i] = new uint64_t[4];
          for (int j = 0; j < 4; j++)
            otMessages1oo4[i][j] =
                (((uint64_t)otMsgCorrSmallRing[i * 4 + j]) << fieldBits) +
                ((uint64_t)otMsgCorrField[i * 4 + j]);
        }
        curotpack->kkot[1]->send_impl(otMessages1oo4, size, totalBitlen);
        for (int i = 0; i < size; i++)
        {
          delete[] otMessages1oo4[i];
        }
        delete[] otMessages1oo4;
      }
      else
      {
        primihub::sci::block128 **otMessages1oo4 = new primihub::sci::block128 *[size];
        for (int i = 0; i < size; i++)
        {
          otMessages1oo4[i] = new primihub::sci::block128[4];
          for (int j = 0; j < 4; j++)
            otMessages1oo4[i][j] = _mm_set_epi64x(otMsgCorrSmallRing[i * 4 + j],
                                                  otMsgCorrField[i * 4 + j]);
        }
        curkkot->send_impl(otMessages1oo4, size, 4);
        for (int i = 0; i < size; i++)
        {
          delete[] otMessages1oo4[i];
        }
        delete[] otMessages1oo4;
      }
      for (int i = 0; i < size; i++)
      {
#ifdef __SIZEOF_INT128__
        intType temp =
            (((__int128)localShareCorr[i]) * ((__int128)ringQuot)) % prime_mod;
#else
        intType temp = primihub::sci::moduloMult(localShareCorr[i], ringQuot, prime_mod);
#endif
        outp[i] = primihub::sci::neg_mod(outp[i] + temp, (int64_t)prime_mod);
      }
    }
    else
    {
      uint8_t *choiceBits = new uint8_t[size];
      for (int i = 0; i < size; i++)
      {
        uint8_t localShareMSB = getFieldMsb(inp[i]);
        choiceBits[i] = (msbShare[i] << 1) + localShareMSB;
      }

      if (OT1oo4FitsIn64Bits)
      {
        uint64_t *otRecvMsg1oo4 = new uint64_t[size];
        curotpack->kkot[1]->recv_impl(otRecvMsg1oo4, choiceBits, size,
                                      totalBitlen);
        for (int i = 0; i < size; i++)
        {
          otMsgCorrField[i] = otRecvMsg1oo4[i] & primihub::sci::all1Mask(fieldBits);
          otMsgCorrSmallRing[i] = otRecvMsg1oo4[i] >> fieldBits;
        }
        delete[] otRecvMsg1oo4;
      }
      else
      {
        primihub::sci::block128 *otRecvMsg1oo4 = new primihub::sci::block128[size];
        curkkot->recv_impl(otRecvMsg1oo4, choiceBits, size, 4);
        for (int i = 0; i < size; i++)
        {
          uint64_t temp = _mm_extract_epi64(otRecvMsg1oo4[i], 0);
          otMsgCorrField[i] = temp;
          temp = _mm_extract_epi64(otRecvMsg1oo4[i], 1);
          otMsgCorrSmallRing[i] = temp;
        }
        delete[] otRecvMsg1oo4;
      }

      for (int i = 0; i < size; i++)
      {
        localShareCorr[i] = otMsgCorrField[i];
        localShareCorrSmallRing[i] = otMsgCorrSmallRing[i];
#ifdef __SIZEOF_INT128__
        intType temp =
            (((__int128)localShareCorr[i]) * ((__int128)ringQuot)) % prime_mod;
#else
        intType temp = primihub::sci::moduloMult(localShareCorr[i], ringQuot, prime_mod);
#endif
        outp[i] = primihub::sci::neg_mod(outp[i] + temp, (int64_t)prime_mod);
      }

      delete[] choiceBits;
    }

    int totalComp = 3 * size;
    int compPerElt = 3;
    if (2 * ringRem < divisor)
    {
      // A+d<0 becomes moot
      totalComp = 2 * size;
      compPerElt = 2;
    }

    intType *localShareA_all3 = new intType[3 * size];
    uint8_t *localShareA_all3_drelu = new uint8_t[3 * size];
    uint64_t bitsAmask = primihub::sci::all1Mask(bitsForA);
    uint64_t bitsAMinusOneMask = primihub::sci::all1Mask((bitsForA - 1));
    uint64_t *radixCompValues = new uint64_t[3 * size];
    uint8_t *carryBit = new uint8_t[3 * size];
    for (int i = 0; i < size; i++)
    {
      intType localShareA =
          (inp[i] % divisor) -
          (getFieldMsb(inp[i]) - localShareCorrSmallRing[i]) * ringRem;
      for (int j = 0; j < compPerElt; j++)
      {
        localShareA_all3[compPerElt * i + j] = localShareA;
      }

      if (curParty == primihub::sci::ALICE)
      {
        if (compPerElt == 3)
        {
          localShareA_all3[3 * i] =
              (localShareA_all3[3 * i] - divisor) & bitsAmask;
          localShareA_all3[3 * i + 2] =
              (localShareA_all3[3 * i + 2] + divisor) & bitsAmask;
        }
        else
        {
          localShareA_all3[2 * i] =
              (localShareA_all3[2 * i] - divisor) & bitsAmask;
        }
      }
      for (int j = 0; j < compPerElt; j++)
      {
        radixCompValues[compPerElt * i + j] =
            (localShareA_all3[compPerElt * i + j] & bitsAMinusOneMask);
        localShareA_all3_drelu[compPerElt * i + j] =
            (localShareA_all3[compPerElt * i + j] >> (bitsForA - 1));
        if (curParty == primihub::sci::BOB)
        {
          radixCompValues[compPerElt * i + j] =
              bitsAMinusOneMask - radixCompValues[compPerElt * i + j];
        }
      }
    }

    MillionaireProtocol millionaire(curParty, curiopack, curotpack);
    millionaire.compare(carryBit, radixCompValues, totalComp, bitsForA - 1);
    for (int i = 0; i < totalComp; i++)
    {
      localShareA_all3_drelu[i] = (localShareA_all3_drelu[i] + carryBit[i]) & 1;
    }

    if (curParty == primihub::sci::ALICE)
    {
      uint64_t **otMsg = new uint64_t *[totalComp];
      intType *localShareDRelu = new intType[totalComp];
      curPrgInstance->random_mod_p<intType>(localShareDRelu, totalComp,
                                            prime_mod);
      for (int i = 0; i < totalComp; i++)
      {
        otMsg[i] = new uint64_t[2];
        otMsg[i][0] =
            primihub::sci::neg_mod(-localShareDRelu[i] + (localShareA_all3_drelu[i]),
                                   (int64_t)prime_mod);
        otMsg[i][1] = primihub::sci::neg_mod(-localShareDRelu[i] +
                                                 ((localShareA_all3_drelu[i] + 1) & 1),
                                             (int64_t)prime_mod);
      }
      curiknp->send_impl(otMsg, totalComp, fieldBits);
      for (int i = 0; i < size; i++)
      {
        intType curCTermShare = 0;
        for (int j = 0; j < compPerElt; j++)
        {
          curCTermShare =
              primihub::sci::neg_mod(curCTermShare + localShareDRelu[compPerElt * i + j],
                                     (int64_t)prime_mod);
        }
        outp[i] = primihub::sci::neg_mod(outp[i] - curCTermShare, (int64_t)prime_mod);
        delete[] otMsg[i];
      }
      delete[] otMsg;
      delete[] localShareDRelu;
    }
    else
    {
      uint64_t *otDataRecvd = new uint64_t[totalComp];
      curiknp->recv_impl(otDataRecvd, localShareA_all3_drelu, totalComp,
                         fieldBits);
      for (int i = 0; i < size; i++)
      {
        intType curCTermShare = 0;
        for (int j = 0; j < compPerElt; j++)
        {
          uint64_t curDReluAns = otDataRecvd[compPerElt * i + j];
          curCTermShare =
              primihub::sci::neg_mod(curCTermShare + curDReluAns, (int64_t)prime_mod);
        }
        outp[i] = primihub::sci::neg_mod(outp[i] - curCTermShare, (int64_t)prime_mod);
      }
      delete[] otDataRecvd;
    }

    if (doMSBComputation)
      delete[] msbShare;
    delete[] localShareCorr;
    delete[] localShareCorrSmallRingPacked;
    delete[] localShareCorrSmallRing;
    delete[] otMsgCorrField;
    delete[] otMsgCorrSmallRing;
    delete[] localShareA_all3;
    delete[] localShareA_all3_drelu;
    delete[] radixCompValues;
    delete[] carryBit;
  }

  template <typename intType>
  void funcFieldDivWrapper(int size, intType *inp, intType *outp, intType divisor,
                           uint8_t *msbShare)
  {
#ifdef MULTITHREADED_TRUNC
    std::thread truncThreads[num_threads];
    int chunk_size = size / num_threads;
    for (int i = 0; i < num_threads; i++)
    {
      int offset = i * chunk_size;
      int curSize;
      if (i == (num_threads - 1))
      {
        curSize = size - offset;
      }
      else
      {
        curSize = chunk_size;
      }
      int curParty = party;
      if (i & 1)
        curParty = 3 - curParty;
      uint8_t *msbShareArg = msbShare;
      if (msbShare != nullptr)
        msbShareArg = msbShareArg + offset;
      truncThreads[i] = std::thread(
          funcFieldDiv<intType>, curParty, iopackArr[i], otpackArr[i],
          otInstanceArr[i], kkotInstanceArr[i], reluArr[i], prgInstanceArr[i],
          curSize, inp + offset, outp + offset, divisor, msbShareArg);
    }
    for (int i = 0; i < num_threads; ++i)
    {
      truncThreads[i].join();
    }
#else
    funcFieldDiv<intType>(party, iopack, otpack, iknpOT, kkot, relu,
                          prg128Instance, size, inp, outp, divisor, msbShare);
#endif
  }
}
#endif // FUNCTIONALITIES_UNIFORM_H__
