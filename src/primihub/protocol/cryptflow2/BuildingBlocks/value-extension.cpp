/*
Authors: Mayank Rathee
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

#include "src/primihub/protocol/cryptflow2/BuildingBlocks/value-extension.h"

using namespace std;
using namespace primihub::sci;
using namespace primihub::cryptflow2;

XTProtocol::XTProtocol(int party, IOPack *iopack, OTPack *otpack)
{
  this->party = party;
  this->iopack = iopack;
  this->otpack = otpack;
  this->aux = new AuxProtocols(party, iopack, otpack);
  this->millionaire = this->aux->mill;
  this->triple_gen = this->millionaire->triple_gen;
}

XTProtocol::~XTProtocol() { delete aux; };

void XTProtocol::z_extend(int32_t dim, uint64_t *inA, uint64_t *outB,
                          int32_t bwA, int32_t bwB, uint8_t *msbA)
{
  if (bwA == bwB)
  {
    memcpy(outB, inA, sizeof(uint64_t) * dim);
    return;
  }
  assert(bwB > bwA && "Extended bitwidth should be > original");
  uint64_t mask_bwA = (bwA == 64 ? -1 : ((1ULL << bwA) - 1));
  uint64_t mask_bwB = (bwB == 64 ? -1 : ((1ULL << bwB) - 1));
  uint8_t *wrap = new uint8_t[dim];

  if (msbA != nullptr)
  {
    this->aux->MSB_to_Wrap(inA, msbA, wrap, dim, bwA);
  }
  else
  {
    this->aux->wrap_computation(inA, wrap, dim, bwA);
  }

  uint64_t *arith_wrap = new uint64_t[dim];
  this->aux->B2A(wrap, arith_wrap, dim, (bwB - bwA));

  for (int i = 0; i < dim; i++)
  {
    outB[i] = ((inA[i] & mask_bwA) - (1ULL << bwA) * arith_wrap[i]) & mask_bwB;
  }

  delete[] wrap;
  delete[] arith_wrap;
}

void XTProtocol::s_extend(int32_t dim, uint64_t *inA, uint64_t *outB,
                          int32_t bwA, int32_t bwB, uint8_t *msbA)
{
  if (bwA == bwB)
  {
    memcpy(outB, inA, sizeof(uint64_t) * dim);
    return;
  }
  assert(bwB > bwA && "Extended bitwidth should be > original");
  uint64_t mask_bwA = (bwA == 64 ? -1 : ((1ULL << bwA) - 1));
  uint64_t mask_bwB = (bwB == 64 ? -1 : ((1ULL << bwB) - 1));

  uint64_t *mapped_inA = new uint64_t[dim];
  uint64_t *mapped_outB = new uint64_t[dim];
  if (party == ALICE)
  {
    for (int i = 0; i < dim; i++)
    {
      mapped_inA[i] = (inA[i] + (1ULL << (bwA - 1))) & mask_bwA;
    }
  }
  else
  { // BOB
    for (int i = 0; i < dim; i++)
    {
      mapped_inA[i] = inA[i];
    }
  }

  uint8_t *tmp_msbA = nullptr;
  if (msbA != nullptr)
  {
    tmp_msbA = new uint8_t[dim];
    for (int i = 0; i < dim; i++)
    {
      tmp_msbA[i] = (party == ALICE ? msbA[i] ^ 1 : msbA[i]);
    }
  }
  this->z_extend(dim, mapped_inA, mapped_outB, bwA, bwB, tmp_msbA);
  if (msbA != nullptr)
  {
    delete[] tmp_msbA;
  }

  if (party == ALICE)
  {
    for (int i = 0; i < dim; i++)
    {
      outB[i] = (mapped_outB[i] - (1ULL << (bwA - 1))) & mask_bwB;
    }
  }
  else
  { // BOB
    for (int i = 0; i < dim; i++)
    {
      outB[i] = (mapped_outB[i]) & mask_bwB;
    }
  }
  delete[] mapped_inA;
  delete[] mapped_outB;
}
