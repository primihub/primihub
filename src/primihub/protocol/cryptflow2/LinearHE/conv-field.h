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

#ifndef CONV_FIELD_H__
#define CONV_FIELD_H__

#include "LinearHE/utils-HE.h"
#include <Eigen/Dense>

// This is to keep compatibility for im2col implementations
typedef Eigen::Matrix<uint64_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
    Channel;
typedef std::vector<Channel> Image;
typedef std::vector<Image> Filters;

struct ConvMetadata {
  int slot_count;
  // Number of plaintext slots in a half ciphertext
  // (since ciphertexts are a two column matrix)
  int32_t pack_num;
  // Number of Channels that can fit in a half ciphertext
  int32_t chans_per_half;
  // Number of input ciphertexts for convolution
  int32_t inp_ct;
  // Number of output ciphertexts
  int32_t out_ct;
  // Image and Filters metadata
  int32_t image_h;
  int32_t image_w;
  size_t image_size;
  int32_t inp_chans;
  int32_t filter_h;
  int32_t filter_w;
  int32_t filter_size;
  int32_t out_chans;
  // How many total ciphertext halves the input and output take up
  int32_t inp_halves;
  int32_t out_halves;
  // The modulo used when deciding which output channels to pack into a mask
  int32_t out_mod;
  // How many permutations of ciphertexts are needed to generate all
  // intermediate rotation sets
  int32_t half_perms;
  /* The number of rotations for each ciphertext half */
  int32_t half_rots;
  // Total number of convolutions needed to generate all
  // intermediate rotations sets
  int32_t convs;
  int32_t stride_h;
  int32_t stride_w;
  int32_t output_h;
  int32_t output_w;
  int32_t pad_t;
  int32_t pad_b;
  int32_t pad_r;
  int32_t pad_l;
};

/* Use casting to do two conditionals instead of one - check if a > 0 and a < b
 */
inline bool condition_check(int a, int b) {
  return static_cast<unsigned>(a) < static_cast<unsigned>(b);
}

Image pad_image(ConvMetadata data, Image &image);

void i2c(Image &image, Channel &column, const int filter_h, const int filter_w,
         const int stride_h, const int stride_w, const int output_h,
         const int output_w);

std::vector<seal::Ciphertext>
HE_preprocess_noise(const uint64_t *const *secret_share,
                    const ConvMetadata &data, seal::Encryptor &encryptor,
                    seal::BatchEncoder &batch_encoder,
                    seal::Evaluator &evaluator);

std::vector<std::vector<uint64_t>> preprocess_image_OP(Image &image,
                                                       ConvMetadata data);

std::vector<std::vector<seal::Ciphertext>>
filter_rotations(std::vector<seal::Ciphertext> &input, const ConvMetadata &data,
                 seal::Evaluator *evaluator = NULL,
                 seal::GaloisKeys *gal_keys = NULL);

std::vector<seal::Ciphertext> HE_encrypt(std::vector<std::vector<uint64_t>> &pt,
                                         const ConvMetadata &data,
                                         seal::Encryptor &encryptor,
                                         seal::BatchEncoder &batch_encoder);

std::vector<std::vector<std::vector<seal::Plaintext>>>
HE_preprocess_filters_OP(Filters &filters, const ConvMetadata &data,
                         seal::BatchEncoder &batch_encoder);

std::vector<seal::Ciphertext>
HE_conv_OP(std::vector<std::vector<std::vector<seal::Plaintext>>> &masks,
           std::vector<std::vector<seal::Ciphertext>> &rotations,
           const ConvMetadata &data, seal::Evaluator &evaluator,
           seal::Ciphertext &zero);

std::vector<seal::Ciphertext>
HE_output_rotations(std::vector<seal::Ciphertext> &convs,
                    const ConvMetadata &data, seal::Evaluator &evaluator,
                    seal::GaloisKeys &gal_keys, seal::Ciphertext &zero,
                    std::vector<seal::Ciphertext> &enc_noise);

uint64_t **HE_decrypt(std::vector<seal::Ciphertext> &enc_result,
                      const ConvMetadata &data, seal::Decryptor &decryptor,
                      seal::BatchEncoder &batch_encoder);

class ConvField {
public:
  int party;
  sci::NetIO *io;
  std::shared_ptr<seal::SEALContext> context[2];
  seal::Encryptor *encryptor[2];
  seal::Decryptor *decryptor[2];
  seal::Evaluator *evaluator[2];
  seal::BatchEncoder *encoder[2];
  seal::GaloisKeys *gal_keys[2];
  seal::Ciphertext *zero[2];
  size_t slot_count;
  ConvMetadata data;

  ConvField(int party, sci::NetIO *io);

  ~ConvField();

  void configure();

  Image ideal_functionality(Image &image, Filters &filters);

  void non_strided_conv(int32_t H, int32_t W, int32_t CI, int32_t FH,
                        int32_t FW, int32_t CO, Image *image, Filters *filters,
                        std::vector<std::vector<std::vector<uint64_t>>> &outArr,
                        bool verbose = false);

  void convolution(
      int32_t N, int32_t H, int32_t W, int32_t CI, int32_t FH, int32_t FW,
      int32_t CO, int32_t zPadHLeft, int32_t zPadHRight, int32_t zPadWLeft,
      int32_t zPadWRight, int32_t strideH, int32_t strideW,
      std::vector<std::vector<std::vector<std::vector<uint64_t>>>> &inputArr,
      std::vector<std::vector<std::vector<std::vector<uint64_t>>>> &filterArr,
      std::vector<std::vector<std::vector<std::vector<uint64_t>>>> &outArr,
      bool verify_output = false, bool verbose = false);

  void
  verify(int H, int W, int CI, int CO, Image &image, Filters *filters,
         std::vector<std::vector<std::vector<std::vector<uint64_t>>>> &outArr);
};

#endif
