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

#include "LinearHE/conv-field.h"

using namespace std;
using namespace primihub::sci;
using namespace seal;
using namespace Eigen;

Image pad_image(ConvMetadata data, Image &image) {
  int image_h = data.image_h;
  int image_w = data.image_w;
  Image p_image;

  int pad_h = data.pad_t + data.pad_b;
  int pad_w = data.pad_l + data.pad_r;
  int pad_top = data.pad_t;
  int pad_left = data.pad_l;

  for (Channel &channel : image) {
    Channel p_channel = Channel::Zero(image_h + pad_h, image_w + pad_w);
    p_channel.block(pad_top, pad_left, image_h, image_w) = channel;
    p_image.push_back(p_channel);
  }
  return p_image;
}

/* Adapted im2col algorithm from Caffe framework */
void i2c(Image &image, Channel &column, const int filter_h, const int filter_w,
         const int stride_h, const int stride_w, const int output_h,
         const int output_w) {
  int height = image[0].rows();
  int width = image[0].cols();
  int channels = image.size();

  int col_width = column.cols();

  // Index counters for images
  int column_i = 0;
  const int channel_size = height * width;
  for (auto &channel : image) {
    for (int filter_row = 0; filter_row < filter_h; filter_row++) {
      for (int filter_col = 0; filter_col < filter_w; filter_col++) {
        int input_row = filter_row;
        for (int output_rows = output_h; output_rows; output_rows--) {
          if (!condition_check(input_row, height)) {
            for (int output_cols = output_w; output_cols; output_cols--) {
              int row_i = column_i / col_width;
              int col_i = column_i % col_width;
              column(row_i, col_i) = 0;
              column_i++;
            }
          } else {
            int input_col = filter_col;
            for (int output_col = output_w; output_col; output_col--) {
              if (condition_check(input_col, width)) {
                int row_i = column_i / col_width;
                int col_i = column_i % col_width;
                column(row_i, col_i) = channel(input_row, input_col);
                column_i++;
              } else {
                int row_i = column_i / col_width;
                int col_i = column_i % col_width;
                column(row_i, col_i) = 0;
                column_i++;
              }
              input_col += stride_w;
            }
          }
          input_row += stride_h;
        }
      }
    }
  }
}

// Generates a masking vector of random noise that will be applied to parts of
// the ciphertext that contain leakage from the convolution
vector<Ciphertext> HE_preprocess_noise(const uint64_t *const *secret_share,
                                       const ConvMetadata &data,
                                       Encryptor &encryptor,
                                       BatchEncoder &batch_encoder,
                                       Evaluator &evaluator) {
  vector<vector<uint64_t>> noise(data.out_ct,
                                 vector<uint64_t>(data.slot_count, 0ULL));
  // Sample randomness into vector
  PRG128 prg;
  for (int ct_idx = 0; ct_idx < data.out_ct; ct_idx++) {
    prg.random_mod_p<uint64_t>(noise[ct_idx].data(), data.slot_count,
                               prime_mod);
  }
  vector<Ciphertext> enc_noise(data.out_ct);

  // Puncture the vector with 0s where an actual convolution result value lives
#pragma omp parallel for num_threads(num_threads) schedule(static)
  for (int ct_idx = 0; ct_idx < data.out_ct; ct_idx++) {
    int out_base = 2 * ct_idx * data.chans_per_half;
    for (int out_c = 0;
         out_c < 2 * data.chans_per_half && out_c + out_base < data.out_chans;
         out_c++) {
      int half_idx = out_c / data.chans_per_half;
      int half_off = out_c % data.chans_per_half;
      for (int col = 0; col < data.output_h; col++) {
        for (int row = 0; row < data.output_w; row++) {
          int noise_idx =
              half_idx * data.pack_num + half_off * data.image_size +
              col * data.stride_w * data.image_w + row * data.stride_h;
          int share_idx = col * data.output_w + row;
          noise[ct_idx][noise_idx] = secret_share[out_base + out_c][share_idx];
        }
      }
    }
    Plaintext tmp;
    batch_encoder.encode(noise[ct_idx], tmp);
    encryptor.encrypt(tmp, enc_noise[ct_idx]);
    evaluator.mod_switch_to_next_inplace(enc_noise[ct_idx]);
  }
  return enc_noise;
}

// Preprocesses the input image for output packing. Ciphertext is packed in
// RowMajor order. In this mode simply pack all the input channels as tightly as
// possible where each channel is padded to the nearest of two
vector<vector<uint64_t>> preprocess_image_OP(Image &image, ConvMetadata data) {
  vector<vector<uint64_t>> ct(data.inp_ct,
                              vector<uint64_t>(data.slot_count, 0));
  int inp_c = 0;
  for (int ct_idx = 0; ct_idx < data.inp_ct; ct_idx++) {
    int inp_c_limit = (ct_idx + 1) * 2 * data.chans_per_half;
    for (; inp_c < data.inp_chans && inp_c < inp_c_limit; inp_c++) {
      // Calculate which half of ciphertext the output channel
      // falls in and the offest from that half,
      int half_idx = (inp_c % (2 * data.chans_per_half)) / data.chans_per_half;
      int half_off = inp_c % data.chans_per_half;
      for (int row = 0; row < data.image_h; row++) {
        for (int col = 0; col < data.image_w; col++) {
          int idx = half_idx * data.pack_num + half_off * data.image_size +
                    row * data.image_w + col;
          ct[ct_idx][idx] = image[inp_c](row, col);
        }
      }
    }
  }
  return ct;
}

// Evaluates the filter rotations necessary to convole an input. Essentially,
// think about placing the filter in the top left corner of the padded image and
// sliding the image over the filter in such a way that we capture which
// elements of filter multiply with which elements of the image. We account for
// the zero padding by zero-puncturing the masks. This function can evaluate
// plaintexts and ciphertexts.
vector<vector<Ciphertext>> filter_rotations(vector<Ciphertext> &input,
                                            const ConvMetadata &data,
                                            Evaluator *evaluator,
                                            GaloisKeys *gal_keys) {
  vector<vector<Ciphertext>> rotations(input.size(),
                                       vector<Ciphertext>(data.filter_size));
  int pad_h = data.pad_t + data.pad_b;
  int pad_w = data.pad_l + data.pad_r;

  // This tells us how many filters fit on a single row of the padded image
  int f_per_row = data.image_w + pad_w - data.filter_w + 1;

  // This offset calculates rotations needed to bring filter from top left
  // corner of image to the top left corner of padded image
  int offset = f_per_row * data.pad_t + data.pad_l;

  // For each element of the filter, rotate the padded image s.t. the top
  // left position always contains the first element of the image it touches
#pragma omp parallel for num_threads(num_threads) schedule(static) collapse(2)
  for (int f = 0; f < data.filter_size; f++) {
    for (size_t ct_idx = 0; ct_idx < input.size(); ct_idx++) {
      int f_row = f / data.filter_w;
      int f_col = f % data.filter_w;
      int row_offset = f_row * data.image_w - offset;
      int rot_amt = row_offset + f_col;
      int idx = f_row * data.filter_w + f_col;
      evaluator->rotate_rows(input[ct_idx], rot_amt, *gal_keys,
                             rotations[ct_idx][idx]);
    }
  }
  return rotations;
}

// Encrypts the given input image
vector<Ciphertext> HE_encrypt(vector<vector<uint64_t>> &pt,
                              const ConvMetadata &data, Encryptor &encryptor,
                              BatchEncoder &batch_encoder) {
  vector<Ciphertext> ct(pt.size());
#pragma omp parallel for num_threads(num_threads) schedule(static)
  for (size_t ct_idx = 0; ct_idx < pt.size(); ct_idx++) {
    Plaintext tmp;
    batch_encoder.encode(pt[ct_idx], tmp);
    encryptor.encrypt(tmp, ct[ct_idx]);
  }
  return ct;
}

// Creates filter masks for an image input that has been output packed.
vector<vector<vector<Plaintext>>>
HE_preprocess_filters_OP(Filters &filters, const ConvMetadata &data,
                         BatchEncoder &batch_encoder) {
  // Mask is convolutions x cts per convolution x mask size
  vector<vector<vector<Plaintext>>> encoded_masks(
      data.convs, vector<vector<Plaintext>>(
                      data.inp_ct, vector<Plaintext>(data.filter_size)));
  // Since a half in a permutation may have a variable number of rotations we
  // use this index to track where we are at in the masks tensor
  // Build each half permutation as well as it's inward rotations
#pragma omp parallel for num_threads(num_threads) schedule(static) collapse(2)
  for (int perm = 0; perm < data.half_perms; perm += 2) {
    for (int rot = 0; rot < data.half_rots; rot++) {
      int conv_idx = perm * data.half_rots;
      for (int ct_idx = 0; ct_idx < data.inp_ct; ct_idx++) {
        // The output channel the current ct starts from
        // int out_base = (((perm/2) + ct_idx)*2*data.chans_per_half) %
        // data.out_mod;
        int out_base = (perm * data.chans_per_half) % data.out_mod;
        // Generate all inward rotations of each half -- half_rots loop
        for (int f = 0; f < data.filter_size; f++) {
          vector<vector<uint64_t>> masks(2,
                                         vector<uint64_t>(data.slot_count, 0));
          for (int half_idx = 0; half_idx < 2; half_idx++) {
            int inp_base = (2 * ct_idx + half_idx) * data.chans_per_half;
            for (int chan = 0; chan < data.chans_per_half &&
                               (chan + inp_base) < data.inp_chans;
                 chan++) {
              // Pull the value of this mask
              int f_w = f % data.filter_w;
              int f_h = f / data.filter_w;
              // Set the coefficients of this channel for both
              // permutations
              uint64_t val, val2;
              int out_idx, out_idx2;

              int offset = neg_mod(chan - rot, (int64_t)data.chans_per_half);
              if (half_idx) {
                // If out_halves < 1 we may repeat within a
                // ciphertext
                // TODO: Add the log optimization for this case
                if (data.out_halves > 1)
                  out_idx = offset + out_base + data.chans_per_half;
                else
                  out_idx = offset + out_base;
                out_idx2 = offset + out_base;
              } else {
                out_idx = offset + out_base;
                out_idx2 = offset + out_base + data.chans_per_half;
              }
              val = (out_idx < data.out_chans)
                        ? filters[out_idx][inp_base + chan](f_h, f_w)
                        : 0;
              val2 = (out_idx2 < data.out_chans)
                         ? filters[out_idx2][inp_base + chan](f_h, f_w)
                         : 0;
              // Iterate through the whole image and figure out which
              // values the filter value touches - this is the same
              // as for input packing
              for (int curr_h = 0; curr_h < data.image_h;
                   curr_h += data.stride_h) {
                for (int curr_w = 0; curr_w < data.image_w;
                     curr_w += data.stride_w) {
                  // curr_h and curr_w simulate the current top-left position of
                  // the filter. This detects whether the filter would fit over
                  // this section. If it's out-of-bounds we set the mask index
                  // to 0
                  bool zero = ((curr_w + f_w) < data.pad_l) ||
                              ((curr_w + f_w) >= (data.image_w + data.pad_l)) ||
                              ((curr_h + f_h) < data.pad_t) ||
                              ((curr_h + f_h) >= (data.image_h + data.pad_l));
                  // Calculate which half of ciphertext the output channel
                  // falls in and the offest from that half,
                  int idx = half_idx * data.pack_num + chan * data.image_size +
                            curr_h * data.image_w + curr_w;
                  // Add both values to appropiate permutations
                  masks[0][idx] = zero ? 0 : val;
                  if (data.half_perms > 1) {
                    masks[1][idx] = zero ? 0 : val2;
                  }
                }
              }
            }
          }
          batch_encoder.encode(masks[0],
                               encoded_masks[conv_idx + rot][ct_idx][f]);
          if (data.half_perms > 1) {
            batch_encoder.encode(
                masks[1],
                encoded_masks[conv_idx + data.half_rots + rot][ct_idx][f]);
          }
        }
      }
    }
  }
  return encoded_masks;
}

// Performs convolution for an output packed image. Returns the intermediate
// rotation sets
vector<Ciphertext> HE_conv_OP(vector<vector<vector<Plaintext>>> &masks,
                              vector<vector<Ciphertext>> &rotations,
                              const ConvMetadata &data, Evaluator &evaluator,
                              Ciphertext &zero) {
  vector<Ciphertext> result(data.convs);

  // Multiply masks and add for each convolution
#pragma omp parallel for num_threads(num_threads) schedule(static)
  for (int conv_idx = 0; conv_idx < data.convs; conv_idx++) {
    result[conv_idx] = zero;
    for (int ct_idx = 0; ct_idx < data.inp_ct; ct_idx++) {
      for (int f = 0; f < data.filter_size; f++) {
        Ciphertext tmp;
        if (!masks[conv_idx][ct_idx][f].is_zero()) {
          evaluator.multiply_plain(rotations[ct_idx][f],
                                   masks[conv_idx][ct_idx][f], tmp);

          evaluator.add_inplace(result[conv_idx], tmp);
        }
      }
    }
    evaluator.mod_switch_to_next_inplace(result[conv_idx]);
  }
  return result;
}

// Takes the result of an output-packed convolution, and rotates + adds all the
// ciphertexts to get a tightly packed output
vector<Ciphertext> HE_output_rotations(vector<Ciphertext> &convs,
                                       const ConvMetadata &data,
                                       Evaluator &evaluator,
                                       GaloisKeys &gal_keys, Ciphertext &zero,
                                       vector<Ciphertext> &enc_noise) {
  vector<Ciphertext> partials(data.half_perms);
  Ciphertext zero_next_level = zero;
  evaluator.mod_switch_to_next_inplace(zero_next_level);
  // Init the result vector to all 0
  vector<Ciphertext> result(data.out_ct);
  for (int ct_idx = 0; ct_idx < data.out_ct; ct_idx++) {
    result[ct_idx] = zero_next_level;
  }

  // For each half perm, add up all the inside channels of each half
#pragma omp parallel for num_threads(num_threads) schedule(static)
  for (int perm = 0; perm < data.half_perms; perm += 2) {
    partials[perm] = zero_next_level;
    if (data.half_perms > 1)
      partials[perm + 1] = zero_next_level;
    // The output channel the current ct starts from
    int total_rots = data.half_rots;
    for (int in_rot = 0; in_rot < total_rots; in_rot++) {
      int conv_idx = perm * data.half_rots + in_rot;
      int rot_amt;
      rot_amt =
          -neg_mod(-in_rot, (int64_t)data.chans_per_half) * data.image_size;

      evaluator.rotate_rows_inplace(convs[conv_idx], rot_amt, gal_keys);
      evaluator.add_inplace(partials[perm], convs[conv_idx]);
      // Do the same for the column swap if it exists
      if (data.half_perms > 1) {
        evaluator.rotate_rows_inplace(convs[conv_idx + data.half_rots], rot_amt,
                                      gal_keys);
        evaluator.add_inplace(partials[perm + 1],
                              convs[conv_idx + data.half_rots]);
      }
    }
    // The correct index for the correct ciphertext in the final output
    int out_idx = (perm / 2) % data.out_ct;
    if (perm == 0) {
      // The first set of convolutions is aligned correctly
      evaluator.add_inplace(result[out_idx], partials[perm]);
      ///*
      if (data.out_halves == 1 && data.inp_halves > 1) {
        // If the output fits in a single half but the input
        // doesn't, add the two columns
        evaluator.rotate_columns_inplace(partials[perm], gal_keys);
        evaluator.add_inplace(result[out_idx], partials[perm]);
      }
      //*/
      // Do the same for column swap if exists and we aren't on a repeat
      if (data.half_perms > 1) {
        evaluator.rotate_columns_inplace(partials[perm + 1], gal_keys);
        evaluator.add_inplace(result[out_idx], partials[perm + 1]);
      }
    } else {
      // Rotate the output ciphertexts by one and add
      evaluator.add_inplace(result[out_idx], partials[perm]);
      // If we're on a tight half we add both halves together and
      // don't look at the column flip
      if (data.half_perms > 1) {
        evaluator.rotate_columns_inplace(partials[perm + 1], gal_keys);
        evaluator.add_inplace(result[out_idx], partials[perm + 1]);
      }
    }
  }
  //// Add the noise vector to remove any leakage
  for (int ct_idx = 0; ct_idx < data.out_ct; ct_idx++) {
    evaluator.add_inplace(result[ct_idx], enc_noise[ct_idx]);
    // evaluator.mod_switch_to_next_inplace(result[ct_idx]);
  }
  return result;
}

// Decrypts and reshapes convolution result
uint64_t **HE_decrypt(vector<Ciphertext> &enc_result, const ConvMetadata &data,
                      Decryptor &decryptor, BatchEncoder &batch_encoder) {
  // Decrypt ciphertext
  vector<vector<uint64_t>> result(data.out_ct);

#pragma omp parallel for num_threads(num_threads) schedule(static)
  for (int ct_idx = 0; ct_idx < data.out_ct; ct_idx++) {
    Plaintext tmp;
    decryptor.decrypt(enc_result[ct_idx], tmp);
    batch_encoder.decode(tmp, result[ct_idx]);
  }

  uint64_t **final_result = new uint64_t *[data.out_chans];
  // Extract correct values to reshape
  for (int out_c = 0; out_c < data.out_chans; out_c++) {
    int ct_idx = out_c / (2 * data.chans_per_half);
    int half_idx = (out_c % (2 * data.chans_per_half)) / data.chans_per_half;
    int half_off = out_c % data.chans_per_half;
    // Depending on the padding type and stride the output values won't be
    // lined up so extract them into a temporary channel before placing
    // them in resultant Image
    final_result[out_c] = new uint64_t[data.output_h * data.output_w];
    for (int col = 0; col < data.output_h; col++) {
      for (int row = 0; row < data.output_w; row++) {
        int idx = half_idx * data.pack_num + half_off * data.image_size +
                  col * data.stride_w * data.image_w + row * data.stride_h;
        final_result[out_c][col * data.output_w + row] = result[ct_idx][idx];
      }
    }
  }
  return final_result;
}

ConvField::ConvField(int party, NetIO *io) {
  this->party = party;
  this->io = io;
  this->slot_count = POLY_MOD_DEGREE_LARGE;
  generate_new_keys(party, io, slot_count, context[1], encryptor[1],
                    decryptor[1], evaluator[1], encoder[1], gal_keys[1],
                    zero[1]);
  this->slot_count = POLY_MOD_DEGREE;
  generate_new_keys(party, io, slot_count, context[0], encryptor[0],
                    decryptor[0], evaluator[0], encoder[0], gal_keys[0],
                    zero[0]);
}

ConvField::~ConvField() {
  for (int i = 0; i < 2; i++) {
    free_keys(party, encryptor[i], decryptor[i], evaluator[i], encoder[i],
              gal_keys[i], zero[i]);
  }
}

void ConvField::configure() {
  data.slot_count = this->slot_count;
  // If using Output packing we pad image_size to the nearest power of 2
  data.image_size = next_pow2(data.image_h * data.image_w);
  data.filter_size = data.filter_h * data.filter_w;

  assert(data.out_chans > 0 && data.inp_chans > 0);
  // Doesn't currently support a channel being larger than a half ciphertext
  assert(data.image_size <= (slot_count / 2));

  data.pack_num = slot_count / 2;
  data.chans_per_half = data.pack_num / data.image_size;
  data.out_ct = ceil((float)data.out_chans / (2 * data.chans_per_half));
  data.inp_ct = ceil((float)data.inp_chans / (2 * data.chans_per_half));

  data.inp_halves = ceil((float)data.inp_chans / data.chans_per_half);
  data.out_halves = ceil((float)data.out_chans / data.chans_per_half);

  // The modulo is calculated per ciphertext instead of per half since we
  // should never have the last out_half wrap around to the first in the
  // same ciphertext
  data.out_mod = data.out_ct * 2 * data.chans_per_half;

  data.half_perms = (data.out_halves % 2 != 0 && data.out_halves > 1)
                        ? data.out_halves + 1
                        : data.out_halves;
  data.half_rots =
      (data.inp_halves > 1 || data.out_halves > 1)
          ? data.chans_per_half
          : max(data.chans_per_half, max(data.out_chans, data.inp_chans));
  data.convs = data.half_perms * data.half_rots;

  data.output_h = 1 + (data.image_h + data.pad_t + data.pad_b - data.filter_h) /
                          data.stride_h;
  data.output_w = 1 + (data.image_w + data.pad_l + data.pad_r - data.filter_w) /
                          data.stride_w;
}

Image ConvField::ideal_functionality(Image &image, Filters &filters) {
  int channels = data.inp_chans;
  int filter_h = data.filter_h;
  int filter_w = data.filter_w;
  int output_h = data.output_h;
  int output_w = data.output_w;

  auto p_image = pad_image(data, image);
  const int col_height = filter_h * filter_w * channels;
  const int col_width = output_h * output_w;
  Channel image_col(col_height, col_width);
  i2c(p_image, image_col, data.filter_h, data.filter_w, data.stride_h,
      data.stride_w, data.output_h, data.output_w);

  // For each filter, flatten it into and multiply with image_col
  Image result;
  for (auto &filter : filters) {
    Channel filter_col(1, col_height);
    // Use im2col with a filter size 1x1 to translate
    i2c(filter, filter_col, 1, 1, 1, 1, filter_h, filter_w);
    Channel tmp = filter_col * image_col;

    // Reshape result of multiplication to the right size
    // SEAL stores matrices in RowMajor form
    result.push_back(Eigen::Map<Eigen::Matrix<uint64_t, Eigen::Dynamic,
                                              Eigen::Dynamic, Eigen::RowMajor>>(
        tmp.data(), output_h, output_w));
  }
  return result;
}

void ConvField::non_strided_conv(int32_t H, int32_t W, int32_t CI, int32_t FH,
                                 int32_t FW, int32_t CO, Image *image,
                                 Filters *filters,
                                 vector<vector<vector<uint64_t>>> &outArr,
                                 bool verbose) {
  data.image_h = H;
  data.image_w = W;
  data.inp_chans = CI;
  data.out_chans = CO;
  data.filter_h = FH;
  data.filter_w = FW;
  data.pad_t = 0;
  data.pad_b = 0;
  data.pad_l = 0;
  data.pad_r = 0;
  data.stride_h = 1;
  data.stride_w = 1;
  this->slot_count =
      min(SEAL_POLY_MOD_DEGREE_MAX, max(8192, 2 * next_pow2(H * W)));
  configure();

  shared_ptr<SEALContext> context_;
  Encryptor *encryptor_;
  Decryptor *decryptor_;
  Evaluator *evaluator_;
  BatchEncoder *encoder_;
  GaloisKeys *gal_keys_;
  Ciphertext *zero_;
  if (slot_count == POLY_MOD_DEGREE) {
    context_ = this->context[0];
    encryptor_ = this->encryptor[0];
    decryptor_ = this->decryptor[0];
    evaluator_ = this->evaluator[0];
    encoder_ = this->encoder[0];
    gal_keys_ = this->gal_keys[0];
    zero_ = this->zero[0];
  } else if (slot_count == POLY_MOD_DEGREE_LARGE) {
    context_ = this->context[1];
    encryptor_ = this->encryptor[1];
    decryptor_ = this->decryptor[1];
    evaluator_ = this->evaluator[1];
    encoder_ = this->encoder[1];
    gal_keys_ = this->gal_keys[1];
    zero_ = this->zero[1];
  } else {
    generate_new_keys(party, io, slot_count, context_, encryptor_, decryptor_,
                      evaluator_, encoder_, gal_keys_, zero_, verbose);
  }

  if (party == BOB) {
    auto pt = preprocess_image_OP(*image, data);
    if (verbose)
      cout << "[Client] Image preprocessed" << endl;

    auto ct = HE_encrypt(pt, data, *encryptor_, *encoder_);
    send_encrypted_vector(io, ct);
    if (verbose)
      cout << "[Client] Image encrypted and sent" << endl;

    vector<Ciphertext> enc_result(data.out_ct);
    recv_encrypted_vector(io,context_, enc_result);
    auto HE_result = HE_decrypt(enc_result, data, *decryptor_, *encoder_);

    if (verbose)
      cout << "[Client] Result received and decrypted" << endl;

    for (int idx = 0; idx < data.output_h * data.output_w; idx++) {
      for (int chan = 0; chan < CO; chan++) {
        outArr[idx / data.output_w][idx % data.output_w][chan] +=
            HE_result[chan][idx];
      }
    }
  } else // party == ALICE
  {
    PRG128 prg;
    uint64_t **secret_share = new uint64_t *[CO];
    for (int chan = 0; chan < CO; chan++) {
      secret_share[chan] = new uint64_t[data.output_h * data.output_w];
      prg.random_mod_p<uint64_t>(secret_share[chan],
                                 data.output_h * data.output_w, prime_mod);
    }
    vector<Ciphertext> noise_ct = HE_preprocess_noise(
        secret_share, data, *encryptor_, *encoder_, *evaluator_);

    if (verbose)
      cout << "[Server] Noise processed" << endl;

    vector<vector<vector<Plaintext>>> masks_OP;
    masks_OP = HE_preprocess_filters_OP(*filters, data, *encoder_);

    if (verbose)
      cout << "[Server] Filters processed" << endl;

    vector<Ciphertext> result;
    vector<Ciphertext> ct(data.inp_ct);
    vector<vector<Ciphertext>> rotations(data.inp_ct);
    for (int i = 0; i < data.inp_ct; i++) {
      rotations[i].resize(data.filter_size);
    }
    recv_encrypted_vector(io,context_, ct);
    rotations = filter_rotations(ct, data, evaluator_, gal_keys_);
    if (verbose)
      cout << "[Server] Filter Rotations done" << endl;

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, rotations[0][0],
                       "before homomorphic convolution");
#endif

    auto conv_result =
        HE_conv_OP(masks_OP, rotations, data, *evaluator_, *zero_);
    if (verbose)
      cout << "[Server] Convolution done" << endl;

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, conv_result[0],
                       "after homomorphic convolution");
#endif

    result = HE_output_rotations(conv_result, data, *evaluator_, *gal_keys_,
                                 *zero_, noise_ct);
    if (verbose)
      cout << "[Server] Output Rotations done" << endl;

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, result[0], "after output rotations");
#endif

    parms_id_type parms_id = result[0].parms_id();
    shared_ptr<const SEALContext::ContextData> context_data =
        context_->get_context_data(parms_id);
    for (size_t ct_idx = 0; ct_idx < result.size(); ct_idx++) {
      flood_ciphertext(result[ct_idx], context_data, SMUDGING_BITLEN);
    }

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, result[0], "after noise flooding");
#endif

    for (size_t ct_idx = 0; ct_idx < result.size(); ct_idx++) {
      evaluator_->mod_switch_to_next_inplace(result[ct_idx]);
    }

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, result[0], "after mod-switch");
#endif

    send_encrypted_vector(io, result);
    if (verbose)
      cout << "[Server] Result computed and sent" << endl;

    for (int idx = 0; idx < data.output_h * data.output_w; idx++) {
      for (int chan = 0; chan < CO; chan++) {
        outArr[idx / data.output_w][idx % data.output_w][chan] +=
            (prime_mod - secret_share[chan][idx]);
      }
    }
    for (int i = 0; i < data.out_chans; i++)
      delete[] secret_share[i];
    delete[] secret_share;
  }
  if (slot_count > POLY_MOD_DEGREE && slot_count < POLY_MOD_DEGREE_LARGE) {
    free_keys(party, encryptor_, decryptor_, evaluator_, encoder_, gal_keys_,
              zero_);
  }
}

void ConvField::convolution(int32_t N, int32_t H, int32_t W, int32_t CI,
                            int32_t FH, int32_t FW, int32_t CO,
                            int32_t zPadHLeft, int32_t zPadHRight,
                            int32_t zPadWLeft, int32_t zPadWRight,
                            int32_t strideH, int32_t strideW,
                            vector<vector<vector<vector<uint64_t>>>> &inputArr,
                            vector<vector<vector<vector<uint64_t>>>> &filterArr,
                            vector<vector<vector<vector<uint64_t>>>> &outArr,
                            bool verify_output, bool verbose) {
  int paddedH = H + zPadHLeft + zPadHRight;
  int paddedW = W + zPadWLeft + zPadWRight;
  int newH = 1 + (paddedH - FH) / strideH;
  int newW = 1 + (paddedW - FW) / strideW;
  int limitH = FH + ((paddedH - FH) / strideH) * strideH;
  int limitW = FW + ((paddedW - FW) / strideW) * strideW;

  for (int i = 0; i < newH; i++) {
    for (int j = 0; j < newW; j++) {
      for (int k = 0; k < CO; k++) {
        outArr[0][i][j][k] = 0;
      }
    }
  }

  Image image;
  Filters filters;

  image.resize(CI);
  for (int chan = 0; chan < CI; chan++) {
    Channel tmp_chan(H, W);
    for (int h = 0; h < H; h++) {
      for (int w = 0; w < W; w++) {
        tmp_chan(h, w) =
            neg_mod((int64_t)inputArr[0][h][w][chan], (int64_t)prime_mod);
      }
    }
    image[chan] = tmp_chan;
  }
  if (party == BOB) {
    for (int s_row = 0; s_row < strideH; s_row++) {
      for (int s_col = 0; s_col < strideW; s_col++) {
        int lH = ((limitH - s_row + strideH - 1) / strideH);
        int lW = ((limitW - s_col + strideW - 1) / strideW);
        int lFH = ((FH - s_row + strideH - 1) / strideH);
        int lFW = ((FW - s_col + strideW - 1) / strideW);
        Image lImage(CI);
        for (int chan = 0; chan < CI; chan++) {
          Channel tmp_chan(lH, lW);
          // lImage[chan] = new uint64_t[lH*lW];
          for (int row = 0; row < lH; row++) {
            for (int col = 0; col < lW; col++) {
              int idxH = row * strideH + s_row - zPadHLeft;
              int idxW = col * strideW + s_col - zPadWLeft;
              if ((idxH < 0 || idxH >= H) || (idxW < 0 || idxW >= W)) {
                tmp_chan(row, col) = 0;
              } else {
                tmp_chan(row, col) =
                    neg_mod(inputArr[0][idxH][idxW][chan], (int64_t)prime_mod);
              }
            }
          }
          lImage[chan] = tmp_chan;
        }
        if (lFH > 0 && lFW > 0) {
          non_strided_conv(lH, lW, CI, lFH, lFW, CO, &lImage, nullptr,
                           outArr[0], verbose);
        }
      }
    }
    for (int idx = 0; idx < newH * newW; idx++) {
      for (int chan = 0; chan < CO; chan++) {
        outArr[0][idx / newW][idx % newW][chan] = neg_mod(
            (int64_t)outArr[0][idx / newW][idx % newW][chan], prime_mod);
      }
    }
    if (verify_output)
      verify(H, W, CI, CO, image, nullptr, outArr);
  } else // party == ALICE
  {
    filters.resize(CO);
    for (int out_c = 0; out_c < CO; out_c++) {
      Image tmp_img(CI);
      for (int inp_c = 0; inp_c < CI; inp_c++) {
        Channel tmp_chan(FH, FW);
        for (int idx = 0; idx < FH * FW; idx++) {
          int64_t val = (int64_t)filterArr[idx / FW][idx % FW][inp_c][out_c];
          if (val > int64_t(prime_mod/2)) {
            val = val - prime_mod;
          }
          tmp_chan(idx / FW, idx % FW) = val;
        }
        tmp_img[inp_c] = tmp_chan;
      }
      filters[out_c] = tmp_img;
    }

    for (int s_row = 0; s_row < strideH; s_row++) {
      for (int s_col = 0; s_col < strideW; s_col++) {
        int lH = ((limitH - s_row + strideH - 1) / strideH);
        int lW = ((limitW - s_col + strideW - 1) / strideW);
        int lFH = ((FH - s_row + strideH - 1) / strideH);
        int lFW = ((FW - s_col + strideW - 1) / strideW);
        Filters lFilters(CO);
        for (int out_c = 0; out_c < CO; out_c++) {
          Image tmp_img(CI);
          for (int inp_c = 0; inp_c < CI; inp_c++) {
            Channel tmp_chan(lFH, lFW);
            for (int row = 0; row < lFH; row++) {
              for (int col = 0; col < lFW; col++) {
                int idxFH = row * strideH + s_row;
                int idxFW = col * strideW + s_col;
                tmp_chan(row, col) = neg_mod(
                    filterArr[idxFH][idxFW][inp_c][out_c], (int64_t)prime_mod);
              }
            }
            tmp_img[inp_c] = tmp_chan;
          }
          lFilters[out_c] = tmp_img;
        }
        if (lFH > 0 && lFW > 0) {
          non_strided_conv(lH, lW, CI, lFH, lFW, CO, nullptr, &lFilters,
                           outArr[0], verbose);
        }
      }
    }
    data.image_h = H;
    data.image_w = W;
    data.inp_chans = CI;
    data.out_chans = CO;
    data.filter_h = FH;
    data.filter_w = FW;
    data.pad_t = zPadHLeft;
    data.pad_b = zPadHRight;
    data.pad_l = zPadWLeft;
    data.pad_r = zPadWRight;
    data.stride_h = strideH;
    data.stride_w = strideW;

    // The filter values should be small enough to not overflow uint64_t
    Image local_result = ideal_functionality(image, filters);

    for (int idx = 0; idx < newH * newW; idx++) {
      for (int chan = 0; chan < CO; chan++) {
        outArr[0][idx / newW][idx % newW][chan] =
            neg_mod((int64_t)local_result[chan](idx / newW, idx % newW) +
                        (int64_t)outArr[0][idx / newW][idx % newW][chan],
                    prime_mod);
      }
    }
    if (verify_output)
      verify(H, W, CI, CO, image, &filters, outArr);
  }
}

void ConvField::verify(int H, int W, int CI, int CO, Image &image,
                       Filters *filters,
                       vector<vector<vector<vector<uint64_t>>>> &outArr) {
  int newH = outArr[0].size();
  int newW = outArr[0][0].size();
  if (party == BOB) {
    for (int i = 0; i < CI; i++) {
      io->send_data(image[i].data(), H * W * sizeof(uint64_t));
    }
    for (int i = 0; i < newH; i++) {
      for (int j = 0; j < newW; j++) {
        io->send_data(outArr[0][i][j].data(),
                      sizeof(uint64_t) * data.out_chans);
      }
    }
  } else // party == ALICE
  {
    Image image_0(CI); // = new Channel[CI];
    for (int i = 0; i < CI; i++) {
      // image_0[i] = new uint64_t[H*W];
      image_0[i].resize(H, W);
      io->recv_data(image_0[i].data(), H * W * sizeof(uint64_t));
    }
    for (int i = 0; i < CI; i++) {
      for (int h = 0; h < H; h++) {
        for (int w = 0; w < W; w++) {
          image[i](h, w) = (image[i](h, w) + image_0[i](h, w)) % prime_mod;
        }
      }
    }
    Image result = ideal_functionality(image, *filters);

    vector<vector<vector<vector<uint64_t>>>> outArr_0;
    outArr_0.resize(1);
    outArr_0[0].resize(newH);
    for (int i = 0; i < newH; i++) {
      outArr_0[0][i].resize(newW);
      for (int j = 0; j < newW; j++) {
        outArr_0[0][i][j].resize(CO);
        io->recv_data(outArr_0[0][i][j].data(), sizeof(uint64_t) * CO);
      }
    }
    for (int i = 0; i < newH; i++) {
      for (int j = 0; j < newW; j++) {
        for (int k = 0; k < CO; k++) {
          outArr_0[0][i][j][k] =
              (outArr_0[0][i][j][k] + outArr[0][i][j][k]) % prime_mod;
        }
      }
    }
    bool pass = true;
    for (int i = 0; i < CO; i++) {
      for (int j = 0; j < newH; j++) {
        for (int k = 0; k < newW; k++) {
          if ((int64_t)outArr_0[0][j][k][i] !=
              neg_mod(result[i](j, k), (int64_t)prime_mod)) {
            pass = false;
          }
        }
      }
    }
    if (pass)
      cout << GREEN << "[Server] Successful Operation" << RESET << endl;
    else {
      cout << RED << "[Server] Failed Operation" << RESET << endl;
      cout << RED << "WARNING: The implementation assumes that the computation"
           << endl;
      cout << "performed locally by the server (on the model and its input "
              "share)"
           << endl;
      cout << "fits in a 64-bit integer. The failed operation could be a result"
           << endl;
      cout << "of overflowing the bound." << RESET << endl;
    }
  }
}
