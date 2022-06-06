/*
Original Author: ryanleh
Modified Work Copyright (c) 2020 Microsoft Research

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Modified by Deevashwer Rathee
*/

#ifndef FC_FIELD_H__
#define FC_FIELD_H__

#include "LinearHE/utils-HE.h"

struct FCMetadata {
  int slot_count;
  int32_t pack_num;
  int32_t inp_ct;
  // Filter is a matrix
  int32_t filter_h;
  int32_t filter_w;
  int32_t filter_size;
  // Image is a vector
  int32_t image_size;
};

seal::Ciphertext preprocess_vec(const uint64_t *input, const FCMetadata &data,
                                seal::Encryptor &encryptor,
                                seal::BatchEncoder &batch_encoder);

std::vector<seal::Plaintext>
preprocess_matrix(const uint64_t *const *matrix, const FCMetadata &data,
                  seal::BatchEncoder &batch_encoder);

seal::Ciphertext fc_preprocess_noise(const uint64_t *secret_share,
                                     const FCMetadata &data,
                                     seal::Encryptor &encryptor,
                                     seal::BatchEncoder &batch_encoder);

seal::Ciphertext fc_online(seal::Ciphertext &ct,
                           std::vector<seal::Plaintext> &enc_mat,
                           const FCMetadata &data, seal::Evaluator &evaluator,
                           seal::GaloisKeys &gal_keys, seal::Ciphertext &zero,
                           seal::Ciphertext &enc_noise);

uint64_t *fc_postprocess(seal::Ciphertext &result, const FCMetadata &data,
                         seal::BatchEncoder &batch_encoder,
                         seal::Decryptor &decryptor);

class FCField {
public:
  int party;
  sci::NetIO *io;
  FCMetadata data;
  std::shared_ptr<seal::SEALContext> context;
  seal::Encryptor *encryptor;
  seal::Decryptor *decryptor;
  seal::Evaluator *evaluator;
  seal::BatchEncoder *encoder;
  seal::GaloisKeys *gal_keys;
  seal::Ciphertext *zero;
  size_t slot_count;

  FCField(int party, sci::NetIO *io);

  ~FCField();

  void configure();

  std::vector<uint64_t> ideal_functionality(uint64_t *vec, uint64_t **matrix);

  void matrix_multiplication(int32_t num_rows, int32_t common_dim,
                             int32_t num_cols,
                             std::vector<std::vector<uint64_t>> &A,
                             std::vector<std::vector<uint64_t>> &B,
                             std::vector<std::vector<uint64_t>> &C,
                             bool verify_output = false, bool verbose = false);

  void verify(std::vector<uint64_t> *vec, std::vector<uint64_t *> *matrix,
              std::vector<std::vector<uint64_t>> &C);
};
#endif
