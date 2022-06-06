/*
Authors: Deevashwer Rathee
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

#ifndef ELEMWISEPROD_FIELD_H__
#define ELEMWISEPROD_FIELD_H__

#include "LinearHE/utils-HE.h"

class ElemWiseProdField {
public:
  int party;
  sci::NetIO *io;
  std::shared_ptr<seal::SEALContext> context;
  seal::Encryptor *encryptor;
  seal::Decryptor *decryptor;
  seal::Evaluator *evaluator;
  seal::BatchEncoder *encoder;
  seal::GaloisKeys *gal_keys;
  seal::Ciphertext *zero;
  int slot_count;

  ElemWiseProdField(int party, sci::NetIO *io);

  ~ElemWiseProdField();

  std::vector<uint64_t> ideal_functionality(std::vector<uint64_t> &inArr,
                                            std::vector<uint64_t> &multArr);

  void elemwise_product(int32_t size, std::vector<uint64_t> &inArr,
                        std::vector<uint64_t> &multArr,
                        std::vector<uint64_t> &outputArr,
                        bool verify_output = false, bool verbose = false);

  void verify(std::vector<uint64_t> &inArr, std::vector<uint64_t> *multArr,
              std::vector<uint64_t> &outArr);
};

#endif
