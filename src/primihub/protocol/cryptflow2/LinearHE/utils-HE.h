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

#ifndef UTILS_HE_H__
#define UTILS_HE_H__

#include "LinearHE/defines-HE.h"
#include "seal/seal.h"
#include "utils/emp-tool.h"

#define PRINT_NOISE_BUDGET(decryptor, ct, print_msg)                       \
    if (verbose)                                                           \
    std::cout << "[Server] Noise Budget " << print_msg << ": " << YELLOW   \
              << decryptor->invariant_noise_budget(ct) << " bits" << RESET \
              << std::endl

void generate_new_keys(int party, primihub::sci::NetIO *io, int slot_count,
                       std::shared_ptr<seal::SEALContext> &context_,
                       seal::Encryptor *&encryptor_,
                       seal::Decryptor *&decryptor_,
                       seal::Evaluator *&evaluator_,
                       seal::BatchEncoder *&encoder_,
                       seal::GaloisKeys *&gal_keys_, seal::Ciphertext *&zero_,
                       bool verbose = false);

void free_keys(int party, seal::Encryptor *&encryptor_,
               seal::Decryptor *&decryptor_, seal::Evaluator *&evaluator_,
               seal::BatchEncoder *&encoder_, seal::GaloisKeys *&gal_keys_,
               seal::Ciphertext *&zero_);

void send_encrypted_vector(primihub::sci::NetIO *io,
                           std::vector<seal::Ciphertext> &ct_vec);

void recv_encrypted_vector(primihub::sci::NetIO *io, std::shared_ptr<seal::SEALContext> context, std::vector<seal::Ciphertext> &ct_vec);

void send_ciphertext(primihub::sci::NetIO *io, seal::Ciphertext &ct);

void recv_ciphertext(primihub::sci::NetIO *io,std::shared_ptr<seal::SEALContext> context, seal::Ciphertext &ct);

void set_poly_coeffs_uniform(
    uint64_t *poly, uint32_t bitlen,
    std::shared_ptr<seal::UniformRandomGenerator> random,
    std::shared_ptr<const seal::SEALContext::ContextData> &context_data);

void flood_ciphertext(
    seal::Ciphertext &ct,
    std::shared_ptr<const seal::SEALContext::ContextData> &context_data,
    uint32_t noise_len,
    seal::MemoryPoolHandle pool = seal::MemoryManager::GetPool());

#endif // UTILS_HE_H__
