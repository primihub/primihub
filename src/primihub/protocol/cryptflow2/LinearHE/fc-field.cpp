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

#include "LinearHE/fc-field.h"

using namespace std;
using namespace seal;
using namespace primihub::sci;

Ciphertext preprocess_vec(const uint64_t *input, const FCMetadata &data,
                          Encryptor &encryptor, BatchEncoder &batch_encoder) {
  // Create copies of the input vector to fill the ciphertext appropiately.
  // Pack using powers of two for easy rotations later
  vector<uint64_t> pod_matrix(data.slot_count, 0ULL);
  uint64_t size_pow2 = next_pow2(data.image_size);
  for (int col = 0; col < data.image_size; col++) {
    for (int idx = 0; idx < data.pack_num; idx++) {
      pod_matrix[col + size_pow2 * idx] = input[col];
    }
  }

  Ciphertext ciphertext;
  Plaintext tmp;
  batch_encoder.encode(pod_matrix, tmp);
  encryptor.encrypt(tmp, ciphertext);
  return ciphertext;
}

vector<Plaintext> preprocess_matrix(const uint64_t *const *matrix,
                                    const FCMetadata &data,
                                    BatchEncoder &batch_encoder) {
  // Pack the filter in alternating order of needed ciphertexts. This way we
  // rotate the input once per ciphertext
  vector<vector<uint64_t>> mat_pack(data.inp_ct,
                                    vector<uint64_t>(data.slot_count, 0ULL));
  for (int row = 0; row < data.filter_h; row++) {
    int ct_idx = row / data.inp_ct;
    for (int col = 0; col < data.filter_w; col++) {
      mat_pack[row % data.inp_ct][col + next_pow2(data.filter_w) * ct_idx] =
          matrix[row][col];
    }
  }

  // Take the packed ciphertexts above and repack them in a diagonal ordering.
  int mod_mask = (data.inp_ct - 1);
  int wrap_thresh = min(data.slot_count >> 1, next_pow2(data.filter_w));
  int wrap_mask = wrap_thresh - 1;
  vector<vector<uint64_t>> mat_diag(data.inp_ct,
                                    vector<uint64_t>(data.slot_count, 0ULL));
  for (int ct = 0; ct < data.inp_ct; ct++) {
    for (int col = 0; col < data.slot_count; col++) {
      int ct_diag_l = (col - ct) & wrap_mask & mod_mask;
      int ct_diag_h = (col ^ ct) & (data.slot_count / 2) & mod_mask;
      int ct_diag = (ct_diag_h + ct_diag_l);

      int col_diag_l = (col - ct_diag_l) & wrap_mask;
      int col_diag_h = wrap_thresh * (col / wrap_thresh) ^ ct_diag_h;
      int col_diag = col_diag_h + col_diag_l;

      mat_diag[ct_diag][col_diag] = mat_pack[ct][col];
    }
  }

  vector<Plaintext> enc_mat(data.inp_ct);
  for (int ct = 0; ct < data.inp_ct; ct++) {
    batch_encoder.encode(mat_diag[ct], enc_mat[ct]);
  }
  return enc_mat;
}

/* Generates a masking vector of random noise that will be applied to parts of
 * the ciphertext that contain leakage */
Ciphertext fc_preprocess_noise(const uint64_t *secret_share,
                               const FCMetadata &data, Encryptor &encryptor,
                               BatchEncoder &batch_encoder) {
  // Sample randomness into vector
  vector<uint64_t> noise(data.slot_count, 0ULL);
  PRG128 prg;
  prg.random_mod_p<uint64_t>(noise.data(), data.slot_count, prime_mod);

  // Puncture the vector with secret shares where an actual fc result value
  // lives
  for (int row = 0; row < data.filter_h; row++) {
    int curr_set = row / data.inp_ct;
    noise[(row % data.inp_ct) + next_pow2(data.image_size) * curr_set] =
        secret_share[row];
  }

  Ciphertext enc_noise;
  Plaintext tmp;
  batch_encoder.encode(noise, tmp);
  encryptor.encrypt(tmp, enc_noise);

  return enc_noise;
}

Ciphertext fc_online(Ciphertext &ct, vector<Plaintext> &enc_mat,
                     const FCMetadata &data, Evaluator &evaluator,
                     GaloisKeys &gal_keys, Ciphertext &zero,
                     Ciphertext &enc_noise) {
  Ciphertext result = zero;
  // For each matrix ciphertext, rotate the input vector once and multiply + add
  Ciphertext tmp;
  for (int ct_idx = 0; ct_idx < data.inp_ct; ct_idx++) {
    if (!enc_mat[ct_idx].is_zero()) {
      evaluator.rotate_rows(ct, ct_idx, gal_keys, tmp);
      evaluator.multiply_plain_inplace(tmp, enc_mat[ct_idx]);
      evaluator.add_inplace(result, tmp);
    }
  }
  evaluator.mod_switch_to_next_inplace(result);
  evaluator.mod_switch_to_next_inplace(enc_noise);

  // Rotate all partial sums together
  for (int rot = data.inp_ct; rot < next_pow2(data.image_size); rot *= 2) {
    Ciphertext tmp;
    if (rot == data.slot_count / 2) {
      evaluator.rotate_columns(result, gal_keys, tmp);
    } else {
      evaluator.rotate_rows(result, rot, gal_keys, tmp);
    }
    evaluator.add_inplace(result, tmp);
  }
  // Add noise to cover leakage
  evaluator.add_inplace(result, enc_noise);
  return result;
}

uint64_t *fc_postprocess(Ciphertext &ct, const FCMetadata &data,
                         BatchEncoder &batch_encoder, Decryptor &decryptor) {
  vector<uint64_t> plain(data.slot_count, 0ULL);
  Plaintext tmp;
  decryptor.decrypt(ct, tmp);
  batch_encoder.decode(tmp, plain);

  uint64_t *result = new uint64_t[data.filter_h];
  for (int row = 0; row < data.filter_h; row++) {
    int curr_set = row / data.inp_ct;
    result[row] =
        plain[(row % data.inp_ct) + next_pow2(data.image_size) * curr_set];
  }
  return result;
}

FCField::FCField(int party, NetIO *io) {
  this->party = party;
  this->io = io;
  this->slot_count = POLY_MOD_DEGREE;
  generate_new_keys(party, io, slot_count, context, encryptor, decryptor,
                    evaluator, encoder, gal_keys, zero);
}

FCField::~FCField() {
  free_keys(party, encryptor, decryptor, evaluator, encoder, gal_keys, zero);
}

void FCField::configure() {
  data.slot_count = slot_count;
  // Only works with a ciphertext that fits in a single ciphertext
  assert(data.slot_count >= data.image_size);

  data.filter_size = data.filter_h * data.filter_w;
  // How many columns of matrix we can fit in a single ciphertext
  data.pack_num = slot_count / next_pow2(data.filter_w);
  // How many total ciphertexts we'll need
  data.inp_ct = ceil((float)next_pow2(data.filter_h) / data.pack_num);
}

vector<uint64_t> FCField::ideal_functionality(uint64_t *vec,
                                              uint64_t **matrix) {
  vector<uint64_t> result(data.filter_h, 0ULL);
  for (int row = 0; row < data.filter_h; row++) {
    for (int idx = 0; idx < data.filter_w; idx++) {
      uint64_t partial = vec[idx] * matrix[row][idx];
      result[row] = result[row] + partial;
    }
  }
  return result;
}

void FCField::matrix_multiplication(int32_t num_rows, int32_t common_dim,
                                    int32_t num_cols,
                                    vector<vector<uint64_t>> &A,
                                    vector<vector<uint64_t>> &B,
                                    vector<vector<uint64_t>> &C,
                                    bool verify_output, bool verbose) {
  assert(num_cols == 1);
  data.filter_h = num_rows;
  data.filter_w = common_dim;
  data.image_size = common_dim;
  this->slot_count =
      min(max(8192, 2 * next_pow2(common_dim)), SEAL_POLY_MOD_DEGREE_MAX);
  configure();

  shared_ptr<SEALContext> context_;
  Encryptor *encryptor_;
  Decryptor *decryptor_;
  Evaluator *evaluator_;
  BatchEncoder *encoder_;
  GaloisKeys *gal_keys_;
  Ciphertext *zero_;
  if (slot_count > POLY_MOD_DEGREE) {
    generate_new_keys(party, io, slot_count, context_, encryptor_, decryptor_,
                      evaluator_, encoder_, gal_keys_, zero_);
  } else {
    context_ = this->context;
    encryptor_ = this->encryptor;
    decryptor_ = this->decryptor;
    evaluator_ = this->evaluator;
    encoder_ = this->encoder;
    gal_keys_ = this->gal_keys;
    zero_ = this->zero;
  }

  if (party == BOB) {
    vector<uint64_t> vec(common_dim);
    for (int i = 0; i < common_dim; i++) {
      vec[i] = B[i][0];
    }
    if (verbose)
      cout << "[Client] Vector Generated" << endl;

    auto ct = preprocess_vec(vec.data(), data, *encryptor_, *encoder_);
    send_ciphertext(io, ct);
    if (verbose)
      cout << "[Client] Vector processed and sent" << endl;

    Ciphertext enc_result;
    recv_ciphertext(io,context_, enc_result);
    auto HE_result = fc_postprocess(enc_result, data, *encoder_, *decryptor_);
    if (verbose)
      cout << "[Client] Result received and decrypted" << endl;

    for (int i = 0; i < num_rows; i++) {
      C[i][0] = HE_result[i];
    }
    if (verify_output)
      verify(&vec, nullptr, C);

    delete[] HE_result;
  } else // party == ALICE
  {
    vector<uint64_t> vec(common_dim);
    for (int i = 0; i < common_dim; i++) {
      vec[i] = B[i][0];
    }
    if (verbose)
      cout << "[Server] Vector Generated" << endl;
    vector<uint64_t *> matrix_mod_p(num_rows);
    vector<uint64_t *> matrix(num_rows);
    for (int i = 0; i < num_rows; i++) {
      matrix_mod_p[i] = new uint64_t[common_dim];
      matrix[i] = new uint64_t[common_dim];
      for (int j = 0; j < common_dim; j++) {
        matrix_mod_p[i][j] = neg_mod((int64_t)A[i][j], (int64_t)prime_mod);
        int64_t val = (int64_t)A[i][j];
        if (val > int64_t(prime_mod/2)) {
          val = val - prime_mod;
        }
        matrix[i][j] = val;
      }
    }
    if (verbose)
      cout << "[Server] Matrix generated" << endl;

    PRG128 prg;
    uint64_t *secret_share = new uint64_t[num_rows];
    prg.random_mod_p<uint64_t>(secret_share, num_rows, prime_mod);

    Ciphertext enc_noise =
        fc_preprocess_noise(secret_share, data, *encryptor_, *encoder_);
    auto encoded_mat = preprocess_matrix(matrix_mod_p.data(), data, *encoder_);
    if (verbose)
      cout << "[Server] Matrix and noise processed" << endl;

    Ciphertext ct;
    recv_ciphertext(io,context_, ct);

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, ct, "before FC Online");
#endif

    auto HE_result = fc_online(ct, encoded_mat, data, *evaluator_, *gal_keys_,
                               *zero_, enc_noise);

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, HE_result, "after FC Online");
#endif

    parms_id_type parms_id = HE_result.parms_id();
    shared_ptr<const SEALContext::ContextData> context_data =
        context_->get_context_data(parms_id);
    flood_ciphertext(HE_result, context_data, SMUDGING_BITLEN);

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, HE_result, "after noise flooding");
#endif

    evaluator_->mod_switch_to_next_inplace(HE_result);

#ifdef HE_DEBUG
    PRINT_NOISE_BUDGET(decryptor_, HE_result, "after mod-switch");
#endif

    send_ciphertext(io, HE_result);
    if (verbose)
      cout << "[Server] Result computed and sent" << endl;

    auto result = ideal_functionality(vec.data(), matrix.data());

    for (int i = 0; i < num_rows; i++) {
      C[i][0] = neg_mod((int64_t)result[i] - (int64_t)secret_share[i],
                        (int64_t)prime_mod);
    }
    if (verify_output)
      verify(&vec, &matrix, C);

    for (int i = 0; i < num_rows; i++) {
      delete[] matrix_mod_p[i];
      delete[] matrix[i];
    }
    delete[] secret_share;
  }
  if (slot_count > POLY_MOD_DEGREE) {
    free_keys(party, encryptor_, decryptor_, evaluator_, encoder_, gal_keys_,
              zero_);
  }
}

void FCField::verify(vector<uint64_t> *vec, vector<uint64_t *> *matrix,
                     vector<vector<uint64_t>> &C) {
  if (party == BOB) {
    io->send_data(vec->data(), data.filter_w * sizeof(uint64_t));
    io->flush();
    for (int i = 0; i < data.filter_h; i++) {
      io->send_data(C[i].data(), sizeof(uint64_t));
    }
  } else // party == ALICE
  {
    vector<uint64_t> vec_0(data.filter_w);
    io->recv_data(vec_0.data(), data.filter_w * sizeof(uint64_t));
    for (int i = 0; i < data.filter_w; i++) {
      vec_0[i] = (vec_0[i] + (*vec)[i]) % prime_mod;
    }
    auto result = ideal_functionality(vec_0.data(), matrix->data());

    vector<vector<uint64_t>> C_0(data.filter_h);
    for (int i = 0; i < data.filter_h; i++) {
      C_0[i].resize(1);
      io->recv_data(C_0[i].data(), sizeof(uint64_t));
      C_0[i][0] = (C_0[i][0] + C[i][0]) % prime_mod;
    }
    bool pass = true;
    for (int i = 0; i < data.filter_h; i++) {
      if (neg_mod(result[i], (int64_t)prime_mod) != (int64_t)C_0[i][0]) {
        pass = false;
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
