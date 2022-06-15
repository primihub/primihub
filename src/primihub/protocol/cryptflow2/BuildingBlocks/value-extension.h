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

#ifndef ZERO_EXT_H__
#define ZERO_EXT_H__

#include "src/primihub/protocol/cryptflow2/BuildingBlocks/aux-protocols.h"
#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire.h"
namespace primihub::cryptflow2
{

  class XTProtocol
  {
  public:
    primihub::sci::IOPack *iopack;
    primihub::sci::OTPack *otpack;
    TripleGenerator *triple_gen;
    MillionaireProtocol *millionaire;
    AuxProtocols *aux;
    int party;

    // Constructor
    XTProtocol(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack);

    // Destructor
    ~XTProtocol();

    void z_extend(int32_t dim, uint64_t *inA, uint64_t *outB, int32_t bwA,
                  int32_t bwB, uint8_t *msbA = nullptr);

    void s_extend(int32_t dim, uint64_t *inA, uint64_t *outB, int32_t bwA,
                  int32_t bwB, uint8_t *msbA = nullptr);
  };
}
#endif // ZERO_EXT_H__
