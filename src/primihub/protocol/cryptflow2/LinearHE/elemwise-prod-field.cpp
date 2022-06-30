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

#include "LinearHE/elemwise-prod-field.h"

using namespace std;
using namespace seal;
using namespace primihub::sci;

ElemWiseProdField::ElemWiseProdField(int party, NetIO *io)
{
  this->party = party;
  this->io = io;
  this->slot_count = POLY_MOD_DEGREE;
  generate_new_keys(party, io, slot_count, context, encryptor, decryptor,
                    evaluator, encoder, gal_keys, zero);
}

ElemWiseProdField::~ElemWiseProdField()
{
  free_keys(party, encryptor, decryptor, evaluator, encoder, gal_keys, zero);
}

vector<uint64_t>
ElemWiseProdField::ideal_functionality(vector<uint64_t> &inArr,
                                       vector<uint64_t> &multArr)
{
  vector<uint64_t> result(inArr.size(), 0ULL);

  for (size_t i = 0; i < inArr.size(); i++)
  {
    result[i] = multArr[i] * inArr[i];
  }
  return result;
}

void ElemWiseProdField::elemwise_product(int32_t size, vector<uint64_t> &inArr,
                                         vector<uint64_t> &multArr,
                                         vector<uint64_t> &outputArr,
                                         bool verify_output, bool verbose)
{
  int num_ct = ceil(float(size) / slot_count);

  if (party == BOB)
  {
    vector<Ciphertext> ct(num_ct);
    for (int i = 0; i < num_ct; i++)
    {
      int offset = i * slot_count;
      vector<uint64_t> tmp_vec(slot_count, 0);
      Plaintext tmp_pt;
      for (int j = 0; j < slot_count && j + offset < size; j++)
      {
        tmp_vec[j] = neg_mod((int64_t)inArr[j + offset], (int64_t)prime_mod);
      }
      encoder->encode(tmp_vec, tmp_pt);
      encryptor->encrypt(tmp_pt, ct[i]);
    }
    send_encrypted_vector(io, ct);

    vector<Ciphertext> enc_result(num_ct);
    recv_encrypted_vector(io, context, enc_result);
    for (int i = 0; i < num_ct; i++)
    {
      int offset = i * slot_count;
      vector<uint64_t> tmp_vec(slot_count, 0);
      Plaintext tmp_pt;
      decryptor->decrypt(enc_result[i], tmp_pt);
      encoder->decode(tmp_pt, tmp_vec);
      for (int j = 0; j < slot_count && j + offset < size; j++)
      {
        outputArr[j + offset] = tmp_vec[j];
      }
    }
    if (verify_output)
      verify(inArr, nullptr, outputArr);
  }
  else // party == ALICE
  {
    vector<Plaintext> multArr_pt(num_ct);
    for (int i = 0; i < num_ct; i++)
    {
      int offset = i * slot_count;
      vector<uint64_t> tmp_vec(slot_count, 0);
      for (int j = 0; j < slot_count && j + offset < size; j++)
      {
        tmp_vec[j] = neg_mod((int64_t)multArr[j + offset], (int64_t)prime_mod);
      }
      encoder->encode(tmp_vec, multArr_pt[i]);
    }

    PRG128 prg;
    vector<Ciphertext> enc_noise(num_ct);
    vector<vector<uint64_t>> secret_share(num_ct,
                                          vector<uint64_t>(slot_count, 0));
    for (int i = 0; i < num_ct; i++)
    {
      Plaintext tmp_pt;
      prg.random_mod_p<uint64_t>(secret_share[i].data(), slot_count, prime_mod);
      encoder->encode(secret_share[i], tmp_pt);
      encryptor->encrypt(tmp_pt, enc_noise[i]);
    }

    vector<Ciphertext> ct(num_ct);
    recv_encrypted_vector(io, context, ct);

    vector<Ciphertext> enc_result(num_ct);
    for (int i = 0; i < num_ct; i++)
    {
#ifdef HE_DEBUG
      if (!i)
        PRINT_NOISE_BUDGET(decryptor, ct[i], "before product");
#endif

      if (multArr_pt[i].is_zero())
      {
        enc_result[i] = *zero;
      }
      else
      {
        evaluator->multiply_plain(ct[i], multArr_pt[i], enc_result[i]);
      }
      evaluator->add_inplace(enc_result[i], enc_noise[i]);

#ifdef HE_DEBUG
      if (!i)
        PRINT_NOISE_BUDGET(decryptor, enc_result[i], "after product");
#endif

      evaluator->mod_switch_to_next_inplace(enc_result[i]);

#ifdef HE_DEBUG
      if (!i)
        PRINT_NOISE_BUDGET(decryptor, enc_result[i], "after mod-switch");
#endif

      parms_id_type parms_id = enc_result[i].parms_id();
      shared_ptr<const SEALContext::ContextData> context_data =
          context->get_context_data(parms_id);
      flood_ciphertext(enc_result[i], context_data, SMUDGING_BITLEN);

#ifdef HE_DEBUG
      if (!i)
        PRINT_NOISE_BUDGET(decryptor, enc_result[i], "after noise flooding");
#endif

      evaluator->mod_switch_to_next_inplace(enc_result[i]);

#ifdef HE_DEBUG
      if (!i)
        PRINT_NOISE_BUDGET(decryptor, enc_result[i], "after mod-switch");
#endif
    }
    send_encrypted_vector(io, enc_result);

    vector<uint64_t> multArr_lifted(size, 0);
    for (int i = 0; i < size; i++)
    {
      int64_t val = (int64_t)multArr[i];
      if (val > int64_t(prime_mod / 2))
      {
        val = val - prime_mod;
      }
      multArr_lifted[i] = val;
    }
    auto result = ideal_functionality(inArr, multArr_lifted);

    for (int i = 0; i < num_ct; i++)
    {
      int offset = i * slot_count;
      for (int j = 0; j < slot_count && j + offset < size; j++)
      {
        outputArr[j + offset] =
            neg_mod((int64_t)result[j + offset] - (int64_t)secret_share[i][j],
                    (int64_t)prime_mod);
      }
    }
    if (verify_output)
      verify(inArr, &multArr_lifted, outputArr);
  }
}

void ElemWiseProdField::verify(vector<uint64_t> &inArr,
                               vector<uint64_t> *multArr,
                               vector<uint64_t> &outArr)
{
  if (party == BOB)
  {
    io->send_data(inArr.data(), inArr.size() * sizeof(uint64_t));
    io->send_data(outArr.data(), outArr.size() * sizeof(uint64_t));
  }
  else // party == ALICE
  {
    vector<uint64_t> inArr_0(inArr.size());
    io->recv_data(inArr_0.data(), inArr.size() * sizeof(uint64_t));
    for (size_t i = 0; i < inArr.size(); i++)
    {
      inArr_0[i] = (inArr[i] + inArr_0[i]) % prime_mod;
    }

    auto result = ideal_functionality(inArr_0, *multArr);

    vector<uint64_t> outArr_0(outArr.size());
    io->recv_data(outArr_0.data(), outArr.size() * sizeof(uint64_t));
    for (size_t i = 0; i < outArr.size(); i++)
    {
      outArr_0[i] = (outArr[i] + outArr_0[i]) % prime_mod;
    }
    bool pass = true;
    for (size_t i = 0; i < outArr.size(); i++)
    {
      if (neg_mod(result[i], (int64_t)prime_mod) != (int64_t)outArr_0[i])
      {
        pass = false;
      }
    }
    if (pass)
      cout << GREEN << "[Server] Successful Operation" << RESET << endl;
    else
    {
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
