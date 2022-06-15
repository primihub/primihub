/*
Authors: Mayank Rathee, Deevashwer Rathee
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

#ifndef DRELU_FIELD_H__
#define DRELU_FIELD_H__

#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire.h"
#include "src/primihub/protocol/cryptflow2/OT/emp-ot.h"
#include "src/primihub/protocol/cryptflow2/utils/emp-tool.h"
#include <cmath>
namespace primihub::cryptflow2
{

  class DReLUFieldProtocol
  {
  public:
    primihub::sci::IOPack *iopack;
    primihub::sci::OTPack *otpack;
    TripleGenerator *triple_gen;
    MillionaireProtocol *millionaire;
    int party;
    int l, r, log_alpha, beta, beta_pow;
    int num_digits, num_triples_corr, num_triples_std, log_num_digits;
    int num_triples;
    uint8_t mask_beta, mask_r, take_lsb;
    uint64_t p, p_2;

    DReLUFieldProtocol(int party, int bitlength, int log_radix_base,
                       uint64_t prime_mod, primihub::sci::IOPack *iopack,
                       primihub::sci::OTPack *otpack)
    {
      assert(log_radix_base <= 8);
      assert(bitlength <= 64);
      this->party = party;
      this->l = bitlength;
      this->beta = log_radix_base;
      this->iopack = iopack;
      this->p = prime_mod;
      this->otpack = otpack;
      this->millionaire = new MillionaireProtocol(party, iopack, otpack,
                                                  bitlength, log_radix_base);
      this->triple_gen = millionaire->triple_gen;
      configure();
    }

    void configure()
    {
      this->num_digits = ceil((double)l / beta);
      this->r = l % beta;
      this->log_alpha = primihub::sci::bitlen(num_digits) - 1;
      this->log_num_digits = log_alpha + 1;
      this->num_triples_corr = 2 * num_digits - 2 - 2 * log_num_digits;
      this->num_triples_std = log_num_digits;
      this->num_triples = num_triples_std + num_triples_corr;
      if (beta == 8)
        this->mask_beta = -1;
      else
        this->mask_beta = (1 << beta) - 1;
      this->mask_r = (1 << r) - 1;
      this->beta_pow = 1 << beta;
      this->take_lsb = 1;
      this->p_2 = (p - 1) / 2;
    }

    ~DReLUFieldProtocol() { delete millionaire; }

    void compute_drelu(uint8_t *drelu, uint64_t *share, int num_relu)
    {
      int old_num_relu = num_relu;
      // num_relu should be a multiple of 4
      num_relu = ceil(num_relu / 4.0) * 4;

      uint64_t *share_ext;
      uint8_t *drelu_ext;
      if (old_num_relu == num_relu)
      {
        share_ext = share;
        drelu_ext = drelu;
      }
      else
      {
        share_ext = new uint64_t[num_relu];
        drelu_ext = new uint8_t[num_relu];
        memcpy(share_ext, share, old_num_relu * sizeof(uint64_t));
        memset(share_ext + old_num_relu, 0,
               (num_relu - old_num_relu) * sizeof(uint64_t));
      }
      int num_cmps = 2 * num_relu;

      uint8_t *digits;       // num_digits * num_cmps
      uint8_t *leaf_res_cmp; // num_digits * num_cmps
      uint8_t *leaf_res_eq;  // num_digits * num_cmps

      // To save number of rounds in WAN over slight increase in communication
#ifdef WAN_EXEC
      Triple triples_std((num_triples)*num_cmps, true);
#else
      Triple triples_corr(num_triples_corr * num_cmps, true, num_cmps);
      Triple triples_std(num_triples_std * num_cmps, true);
#endif
      digits = new uint8_t[num_digits * num_cmps];
      leaf_res_cmp = new uint8_t[num_digits * num_cmps];
      leaf_res_eq = new uint8_t[num_digits * num_cmps];

      // Extract radix-digits from data
      assert((beta <= 8) && "Base beta > 8 is not implemented");
      if (party == primihub::sci::ALICE)
      {
        for (int j = 0; j < num_relu; j++)
        {
          uint64_t input_wrap_cmp = (p - 1 - share_ext[j]) + p_2;
          uint64_t input_drelu_cmp;
          if (share_ext[j] > p_2)
          {
            input_drelu_cmp = 2 * p - 1 - share_ext[j];
          }
          else
          {
            input_drelu_cmp = p - 1 - share_ext[j];
          }
          for (int i = 0; i < num_digits; i++)
          { // Stored from LSB to MSB
            if ((i == num_digits - 1) &&
                (r != 0))
            { // A partially full last digit
              digits[i * num_relu * 2 + j * 2 + 0] =
                  (uint8_t)(input_wrap_cmp >> i * beta) & mask_r;
              digits[i * num_relu * 2 + j * 2 + 1] =
                  (uint8_t)(input_drelu_cmp >> i * beta) & mask_r;
            }
            else
            {
              digits[i * num_relu * 2 + j * 2 + 0] =
                  (uint8_t)(input_wrap_cmp >> i * beta) & mask_beta;
              digits[i * num_relu * 2 + j * 2 + 1] =
                  (uint8_t)(input_drelu_cmp >> i * beta) & mask_beta;
            }
          }
        }
      }
      else // party = primihub::sci::BOB
      {
        for (int j = 0; j < num_relu; j++)
        {
          uint64_t input_cmp = p_2 + share_ext[j];
          for (int i = 0; i < num_digits; i++)
          { // Stored from LSB to MSB
            if ((i == num_digits - 1) &&
                (r != 0))
            { // A partially full last digit
              digits[i * num_relu + j * 1 + 0] =
                  (uint8_t)(input_cmp >> i * beta) & mask_r;
            }
            else
            {
              digits[i * num_relu + j * 1 + 0] =
                  (uint8_t)(input_cmp >> i * beta) & mask_beta;
            }
          }
        }
      }

      if (party == primihub::sci::ALICE)
      {
        uint8_t *
            *leaf_ot_messages; // (num_digits * num_relu) X beta_pow (=2^beta)
        // Do the two relu_comparisons together in 1 leaf OT.
        leaf_ot_messages = new uint8_t *[num_digits * num_relu];
        for (int i = 0; i < num_digits * num_relu; i++)
          leaf_ot_messages[i] = new uint8_t[beta_pow];

        // Set Leaf OT messages
        triple_gen->prg->random_bool((bool *)leaf_res_cmp,
                                     num_digits * num_relu * 2);
        triple_gen->prg->random_bool((bool *)leaf_res_eq,
                                     num_digits * num_relu * 2);
        for (int i = 0; i < num_digits; i++)
        {
          for (int j = 0; j < num_relu; j++)
          {
            if (i == 0)
            {
              set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                   digits + i * num_relu * 2 + j * 2, beta_pow,
                                   leaf_res_cmp + i * num_relu * 2 + j * 2, 0,
                                   false);
            }
            else if (i == (num_digits - 1) && (r > 0))
            {
#ifdef WAN_EXEC
              set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                   digits + i * num_relu * 2 + j * 2, beta_pow,
                                   leaf_res_cmp + i * num_relu * 2 + j * 2,
                                   leaf_res_eq + i * num_relu * 2 + j * 2);
#else
              set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                   digits + i * num_relu * 2 + j * 2, 1 << r,
                                   leaf_res_cmp + i * num_relu * 2 + j * 2,
                                   leaf_res_eq + i * num_relu * 2 + j * 2);
#endif
            }
            else
            {
              set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                   digits + i * num_relu * 2 + j * 2, beta_pow,
                                   leaf_res_cmp + i * num_relu * 2 + j * 2,
                                   leaf_res_eq + i * num_relu * 2 + j * 2);
            }
          }
        }

        // Perform Leaf OTs

        // Each ReLU has the first digit of which equality is not required.
#ifdef WAN_EXEC
        otpack->kkot[beta - 1]->send(leaf_ot_messages, num_relu * (num_digits),
                                     2 * 2);
#else
        otpack->kkot[beta - 1]->send(leaf_ot_messages, num_relu, 1 * 2);
        if (r == 1)
        {
          // For the last digit (MSB), use IKNP because it is just 1-bit
          otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                       num_relu * (num_digits - 2), 2 * 2);
          otpack->iknp_straight->send(
              leaf_ot_messages + num_relu * (num_digits - 1), num_relu, 2 * 2);
        }
        else if (r != 0)
        {
          otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                       num_relu * (num_digits - 2), 2 * 2);
          otpack->kkot[r - 1]->send(
              leaf_ot_messages + num_relu * (num_digits - 1), num_relu, 2 * 2);
        }
        else
          otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                       num_relu * (num_digits - 1), 2 * 2);
#endif
        // Cleanup
        for (int i = 0; i < num_digits * num_relu; i++)
          delete[] leaf_ot_messages[i];
        delete[] leaf_ot_messages;
        // Alice's shares are: cmp1, cmp2 and cmp3 are at leaf_res_cmp[0+3*j],
        // leaf_res_cmp[1+3*j] and leaf_res_cmp[2+3*j]
      }
      else // party = primihub::sci::BOB
      {
        // Perform Leaf OTs
        uint8_t *leaf_ot_recvd = new uint8_t[num_digits * num_relu];
#ifdef WAN_EXEC
        otpack->kkot[beta - 1]->recv(leaf_ot_recvd, digits,
                                     num_relu * (num_digits), 2 * 2);
#else
        otpack->kkot[beta - 1]->recv(leaf_ot_recvd, digits, num_relu, 1 * 2);
        if (r == 1)
        {
          otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                       digits + num_relu,
                                       num_relu * (num_digits - 2), 2 * 2);
          otpack->iknp_straight->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                      digits + num_relu * (num_digits - 1),
                                      num_relu, 2 * 2);
        }
        else if (r != 0)
        {
          otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                       digits + num_relu,
                                       num_relu * (num_digits - 2), 2 * 2);
          otpack->kkot[r - 1]->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                    digits + num_relu * (num_digits - 1),
                                    num_relu, 2 * 2);
        }
        else
          otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                       digits + num_relu,
                                       num_relu * (num_digits - 1), 2 * 2);
#endif

        // Extract equality result from leaf_res_cmp
        for (int i = 0; i < num_relu; i++)
        {
          leaf_res_cmp[2 * i] = (leaf_ot_recvd[i] & (1 << 1)) >> 1;
          leaf_res_cmp[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 0)) >> 0;
        }
        for (int i = num_relu; i < num_digits * num_relu; i++)
        {
          leaf_res_cmp[2 * i] = (leaf_ot_recvd[i] & (1 << 3)) >> 3;
          leaf_res_cmp[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 2)) >> 2;

          leaf_res_eq[2 * i] = (leaf_ot_recvd[i] & (1 << 1)) >> 1;
          leaf_res_eq[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 0)) >> 0;
        }
        delete[] leaf_ot_recvd;
      }

      // Generate required Bit-Triples and traverse tree to compute the results of
      // comparsions
      millionaire->traverse_and_compute_ANDs(num_cmps, leaf_res_eq, leaf_res_cmp);

      if (party == primihub::sci::ALICE)
      {
        uint8_t **mux_ot_messages = new uint8_t *[num_relu];
        for (int j = 0; j < num_relu; j++)
        {
          mux_ot_messages[j] = new uint8_t[4];
        }
        triple_gen->prg->random_bool((bool *)drelu_ext, num_relu);
        for (int j = 0; j < num_relu; j++)
        {
          // drelu[j] = 0;
          bool neg_share = (share_ext[j] > p_2);
          set_mux_ot_messages(mux_ot_messages[j], leaf_res_cmp + j * 2, drelu_ext[j],
                              neg_share);
        }
        otpack->kkot[1]->send(mux_ot_messages, num_relu, 1);
        for (int j = 0; j < num_relu; j++)
        {
          delete[] mux_ot_messages[j];
        }
        delete[] mux_ot_messages;
      }
      else // primihub::sci::BOB
      {
        uint8_t *mux_ot_selection = new uint8_t[num_relu];
        for (int j = 0; j < num_relu; j++)
        {
          mux_ot_selection[j] =
              (leaf_res_cmp[j * 2 + 1] << 1) | leaf_res_cmp[j * 2];
        }
        otpack->kkot[1]->recv(drelu_ext, mux_ot_selection, num_relu, 1);
        delete[] mux_ot_selection;
      }
      if (old_num_relu != num_relu)
      {
        memcpy(drelu, drelu_ext, old_num_relu * sizeof(uint8_t));
        delete[] share_ext;
        delete[] drelu_ext;
      }

      delete[] digits;
      delete[] leaf_res_cmp;
      delete[] leaf_res_eq;
    }

    void set_leaf_ot_messages(uint8_t *ot_messages, uint8_t *digits, int N,
                              uint8_t *additive_mask_cmp,
                              uint8_t *additive_mask_eq, bool eq = true)
    {
      for (int i = 0; i < N; i++)
      {
        if (eq)
        {
          ot_messages[i] = 0;
          // For comparisons
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[0] < i) ^ additive_mask_cmp[0]);
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[1] < i) ^ additive_mask_cmp[1]);
          // For equality
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[0] == i) ^ additive_mask_eq[0]);
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[1] == i) ^ additive_mask_eq[1]);
        }
        else
        {
          ot_messages[i] = 0;
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[0] < i) ^ additive_mask_cmp[0]);
          ot_messages[i] =
              (ot_messages[i] << 1) | ((digits[1] < i) ^ additive_mask_cmp[1]);
        }
      }
    }

    void set_mux_ot_messages(uint8_t *ot_messages, uint8_t *cmp_results,
                             uint8_t drelu_mask, bool neg_share)
    {
      uint8_t bits_i[2];
      for (int i = 0; i < 4; i++)
      {
        primihub::sci::uint8_to_bool(bits_i, i, 2); // wrap_cmp || !drelu_cmp
        uint8_t drelu_cmp = bits_i[1] ^ cmp_results[1];
        uint8_t wrap = bits_i[0] ^ cmp_results[0];
        if (neg_share)
        {
          ot_messages[i] = ((1 ^ drelu_cmp) & wrap) ^ 1;
        }
        else
        {
          ot_messages[i] = drelu_cmp ^ (drelu_cmp & wrap);
        }
        ot_messages[i] ^= drelu_mask;
      }
    }
  };
}
#endif // DRELU_FIELD_H__
