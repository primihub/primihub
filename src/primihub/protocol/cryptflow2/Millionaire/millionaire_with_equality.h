/*
Authors: Mayank Rathee
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

#ifndef MILLIONAIRE_WITH_EQ_H__
#define MILLIONAIRE_WITH_EQ_H__
#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire.h"
#include "src/primihub/protocol/cryptflow2/OT/emp-ot.h"
#include "src/primihub/protocol/cryptflow2/utils/emp-tool.h"
#include <cmath>

class MillionaireWithEquality {
public:
  primihub::sci::IOPack *iopack;
  primihub::sci::OTPack *otpack;
  TripleGenerator *triple_gen;
  MillionaireProtocol *mill;
  int party;
  int l, r, log_alpha, beta, beta_pow;
  int num_digits, num_triples, log_num_digits;
  uint8_t mask_beta, mask_r;

  MillionaireWithEquality(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack,
                          int bitlength = 32, int radix_base = MILL_PARAM) {
    this->party = party;
    this->iopack = iopack;
    this->otpack = otpack;
    this->mill =
        new MillionaireProtocol(party, iopack, otpack, bitlength, radix_base);
    this->triple_gen = mill->triple_gen;
    configure(bitlength, radix_base);
  }

  void configure(int bitlength, int radix_base = MILL_PARAM) {
    assert(radix_base <= 8);
    assert(bitlength <= 64);
    this->l = bitlength;
    this->beta = radix_base;

    this->num_digits = ceil((double)l / beta);
    this->r = l % beta;
    this->log_alpha = primihub::sci::bitlen(num_digits) - 1;
    this->log_num_digits = log_alpha + 1;
    this->num_triples = 2 * num_digits - 2;
    if (beta == 8)
      this->mask_beta = -1;
    else
      this->mask_beta = (1 << beta) - 1;
    this->mask_r = (1 << r) - 1;
    this->beta_pow = 1 << beta;
  }

  ~MillionaireWithEquality() { delete mill; }

  void bitlen_lt_beta(uint8_t *res_cmp, uint8_t *res_eq, uint64_t *data,
                      int num_cmps, int bitlength, bool greater_than = true,
                      int radix_base = MILL_PARAM) {
    uint8_t N = 1 << bitlength;
    uint8_t mask = N - 1;
    if (party == primihub::sci::ALICE) {
      primihub::sci::PRG128 prg;
      prg.random_data(res_cmp, num_cmps * sizeof(uint8_t));
      prg.random_data(res_eq, num_cmps * sizeof(uint8_t));
      uint8_t **leaf_messages = new uint8_t *[num_cmps];
      for (int i = 0; i < num_cmps; i++) {
        res_cmp[i] &= 1;
        res_eq[i] &= 1;
        leaf_messages[i] = new uint8_t[N];
        this->mill->set_leaf_ot_messages(leaf_messages[i], (data[i] & mask), N,
                                         res_cmp[i], res_eq[i], greater_than,
                                         true);
      }
      if (bitlength > 1) {
        otpack->kkot[bitlength - 1]->send(leaf_messages, num_cmps, 2);
      } else {
        otpack->iknp_straight->send(leaf_messages, num_cmps, 2);
      }

      for (int i = 0; i < num_cmps; i++)
        delete[] leaf_messages[i];
      delete[] leaf_messages;
    } else { // party == BOB
      uint8_t *choice = new uint8_t[num_cmps];
      for (int i = 0; i < num_cmps; i++) {
        choice[i] = data[i] & mask;
      }
      if (bitlength > 1) {
        otpack->kkot[bitlength - 1]->recv(res_cmp, choice, num_cmps, 2);
      } else {
        otpack->iknp_straight->recv(res_cmp, choice, num_cmps, 2);
      }
      for (int i = 0; i < num_cmps; i++) {
        res_eq[i] = res_cmp[i] & 1;
        res_cmp[i] >>= 1;
      }
      delete[] choice;
    }
    return;
  }

  void compare_with_eq(uint8_t *res_cmp, uint8_t *res_eq, uint64_t *data,
                       int num_cmps, int bitlength, bool greater_than = true,
                       int radix_base = MILL_PARAM) {
    configure(bitlength, radix_base);

    if (bitlength <= beta) {
      bitlen_lt_beta(res_cmp, res_eq, data, num_cmps, bitlength, greater_than,
                     radix_base);
      return;
    }

    int old_num_cmps = num_cmps;
    // num_cmps should be a multiple of 8
    num_cmps = ceil(num_cmps / 8.0) * 8;

    // padding with 0s if data dim not multiple of 8
    uint64_t *data_ext;
    if (old_num_cmps == num_cmps)
      data_ext = data;
    else {
      data_ext = new uint64_t[num_cmps];
      memcpy(data_ext, data, old_num_cmps * sizeof(uint64_t));
      memset(data_ext + old_num_cmps, 0,
             (num_cmps - old_num_cmps) * sizeof(uint64_t));
    }

    uint8_t *digits;       // num_digits * num_cmps
    uint8_t *leaf_res_cmp; // num_digits * num_cmps
    uint8_t *leaf_res_eq;  // num_digits * num_cmps

    digits = new uint8_t[num_digits * num_cmps];
    leaf_res_cmp = new uint8_t[num_digits * num_cmps];
    leaf_res_eq = new uint8_t[num_digits * num_cmps];

    // Extract radix-digits from data
    for (int i = 0; i < num_digits; i++) // Stored from LSB to MSB
      for (int j = 0; j < num_cmps; j++)
        if ((i == num_digits - 1) && (r != 0))
          digits[i * num_cmps + j] =
              (uint8_t)(data_ext[j] >> i * beta) & mask_r;
        else
          digits[i * num_cmps + j] =
              (uint8_t)(data_ext[j] >> i * beta) & mask_beta;
    // ======================

    // Set leaf OT messages now
    if (party == primihub::sci::ALICE) {
      uint8_t *
          *leaf_ot_messages; // (num_digits * num_cmps) X beta_pow (=2^beta)
      leaf_ot_messages = new uint8_t *[num_digits * num_cmps];
      for (int i = 0; i < num_digits * num_cmps; i++)
        leaf_ot_messages[i] = new uint8_t[beta_pow];

      // Set Leaf OT messages
      triple_gen->prg->random_bool((bool *)leaf_res_cmp, num_digits * num_cmps);
      triple_gen->prg->random_bool((bool *)leaf_res_eq, num_digits * num_cmps);
      for (int i = 0; i < num_digits; i++) {
        for (int j = 0; j < num_cmps; j++) {
          if (i == (num_digits - 1) && (r > 0)) {
            this->mill->set_leaf_ot_messages(
                leaf_ot_messages[i * num_cmps + j], digits[i * num_cmps + j],
                1ULL << r, leaf_res_cmp[i * num_cmps + j],
                leaf_res_eq[i * num_cmps + j], greater_than);
          } else {
            this->mill->set_leaf_ot_messages(
                leaf_ot_messages[i * num_cmps + j], digits[i * num_cmps + j],
                beta_pow, leaf_res_cmp[i * num_cmps + j],
                leaf_res_eq[i * num_cmps + j], greater_than);
          }
        }
      }

      // Perform Leaf OTs with comparison and equality
      if (r == 1) {
        // All branches except r
        otpack->kkot[beta - 1]->send(leaf_ot_messages,
                                     num_cmps * (num_digits - 1), 2);
        // r branch
        otpack->iknp_straight->send(
            leaf_ot_messages + num_cmps * (num_digits - 1), num_cmps, 2);
      } else if (r != 0) {
        // All branches except r
        otpack->kkot[beta - 1]->send(leaf_ot_messages,
                                     num_cmps * (num_digits - 1), 2);
        // r branch
        otpack->kkot[r - 1]->send(
            leaf_ot_messages + num_cmps * (num_digits - 1), num_cmps, 2);
      } else {
        // All branches including r, r is 0
        otpack->kkot[beta - 1]->send(leaf_ot_messages, num_cmps * (num_digits),
                                     2);
      }

      // Cleanup
      for (int i = 0; i < num_digits * num_cmps; i++)
        delete[] leaf_ot_messages[i];
      delete[] leaf_ot_messages;
    } else // party = primihub::sci::BOB
    {
      // Perform Leaf OTs
      if (r == 1) {
        // All branches except r
        otpack->kkot[beta - 1]->recv(leaf_res_cmp, digits,
                                     num_cmps * (num_digits - 1), 2);
        // r branch
        otpack->iknp_straight->recv(leaf_res_cmp + num_cmps * (num_digits - 1),
                                    digits + num_cmps * (num_digits - 1),
                                    num_cmps, 2);
      } else if (r != 0) {
        // All branches except r
        otpack->kkot[beta - 1]->recv(leaf_res_cmp, digits,
                                     num_cmps * (num_digits - 1), 2);
        // r branch
        otpack->kkot[r - 1]->recv(leaf_res_cmp + num_cmps * (num_digits - 1),
                                  digits + num_cmps * (num_digits - 1),
                                  num_cmps, 2);
      } else {
        // All branches including r, r is 0
        otpack->kkot[beta - 1]->recv(leaf_res_cmp, digits,
                                     num_cmps * (num_digits), 2);
      }

      // Extract equality result from leaf_res_cmp
      for (int i = 0; i < num_digits * num_cmps; i++) {
        leaf_res_eq[i] = leaf_res_cmp[i] & 1;
        leaf_res_cmp[i] >>= 1;
      }
    }

    traverse_and_compute_ANDs(num_cmps, leaf_res_eq, leaf_res_cmp);

    for (int i = 0; i < old_num_cmps; i++) {
      res_cmp[i] = leaf_res_cmp[i];
      res_eq[i] = leaf_res_eq[i];
    }

    // Cleanup
    if (old_num_cmps != num_cmps)
      delete[] data_ext;
    delete[] digits;
    delete[] leaf_res_cmp;
    delete[] leaf_res_eq;
  }

  /**************************************************************************************************
   *                         AND computation related functions
   **************************************************************************************************/

  void traverse_and_compute_ANDs(int num_cmps, uint8_t *leaf_res_eq,
                                 uint8_t *leaf_res_cmp) {
    Triple triples_corr((num_triples)*num_cmps, true, num_cmps);

    // Generate required Bit-Triples
    triple_gen->generate(party, &triples_corr, _8KKOT);

    // Combine leaf OT results in a bottom-up fashion
    int counter_triples_used = 0, old_counter_triples_used = 0;
    uint8_t *ei = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *fi = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *e = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *f = new uint8_t[(num_triples * num_cmps) / 8];

    for (int i = 1; i < num_digits;
         i *= 2) { // i denotes the distance between 2 nodes which should be
                   // ANDed together
      for (int j = 0; j < num_digits and j + i < num_digits;
           j += 2 * i) { // j=0 is LSD and j=num_digits-1 is MSD

        // CMP_j: Use 1 triple for opening e = a + cmp_j and f = b + eq_j+i.
        this->mill->AND_step_1(
            ei + (2 * counter_triples_used * num_cmps) / 8,
            fi + (2 * counter_triples_used * num_cmps) / 8,
            leaf_res_cmp + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
            (triples_corr.ai) + (2 * counter_triples_used * num_cmps) / 8,
            (triples_corr.bi) + (2 * counter_triples_used * num_cmps) / 8,
            num_cmps);
        // EQ_j: Use 1 triple for opening e = a + eq_j and f = b + eq_j+i.
        this->mill->AND_step_1(
            ei + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            fi + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            leaf_res_eq + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
            (triples_corr.ai) + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            (triples_corr.bi) + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            num_cmps);
        counter_triples_used++;
      }
      int offset = (2 * old_counter_triples_used * num_cmps) / 8;
      int size_used =
          (2 * (counter_triples_used - old_counter_triples_used) * num_cmps) /
          8;

#pragma omp parallel num_threads(2)
      {
        if (omp_get_thread_num() == 1) {
          if (party == primihub::sci::ALICE) {
            iopack->io_rev->recv_data(e + offset, size_used);
            iopack->io_rev->recv_data(e + offset, size_used);
            iopack->io_rev->recv_data(f + offset, size_used);
            iopack->io_rev->recv_data(f + offset, size_used);
          } else { // party == primihub::sci::BOB
            iopack->io_rev->send_data(ei + offset, size_used);
            iopack->io_rev->send_data(ei + offset, size_used);
            iopack->io_rev->send_data(fi + offset, size_used);
            iopack->io_rev->send_data(fi + offset, size_used);
          }
        } else {
          if (party == primihub::sci::ALICE) {
            iopack->io->send_data(ei + offset, size_used);
            iopack->io->send_data(ei + offset, size_used);
            iopack->io->send_data(fi + offset, size_used);
            iopack->io->send_data(fi + offset, size_used);
          } else { // party == primihub::sci::BOB
            iopack->io->recv_data(e + offset, size_used);
            iopack->io->recv_data(e + offset, size_used);
            iopack->io->recv_data(f + offset, size_used);
            iopack->io->recv_data(f + offset, size_used);
          }
        }
      }

      // Reconstruct e and f
      for (int i = 0; i < size_used; i++) {
        e[i + offset] ^= ei[i + offset];
        f[i + offset] ^= fi[i + offset];
      }

      counter_triples_used = old_counter_triples_used;

      // Step 2 of AND computation
      for (int j = 0; j < num_digits and j + i < num_digits;
           j += 2 * i) { // j=0 is LSD and j=num_digits-1 is MSD
        // CMP_j: Use 1 triple compute cmp_j AND eq_j+i.
        this->mill->AND_step_2(
            leaf_res_cmp + j * num_cmps,
            e + (2 * counter_triples_used * num_cmps) / 8,
            f + (2 * counter_triples_used * num_cmps) / 8,
            nullptr, // not used in function
            nullptr, // not used in function
            (triples_corr.ai) + (2 * counter_triples_used * num_cmps) / 8,
            (triples_corr.bi) + (2 * counter_triples_used * num_cmps) / 8,
            (triples_corr.ci) + (2 * counter_triples_used * num_cmps) / 8,
            num_cmps);
        // EQ_j: Use 1 triple compute eq_j AND eq_j+i.
        this->mill->AND_step_2(
            leaf_res_eq + j * num_cmps,
            e + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            f + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            nullptr, // not used in function
            nullptr, // not used in function
            (triples_corr.ai) + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            (triples_corr.bi) + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            (triples_corr.ci) + ((2 * counter_triples_used + 1) * num_cmps) / 8,
            num_cmps);
        for (int k = 0; k < num_cmps; k++) {
          leaf_res_cmp[j * num_cmps + k] ^=
              leaf_res_cmp[(j + i) * num_cmps + k];
        }
        counter_triples_used++;
      }

      old_counter_triples_used = counter_triples_used;
    }

    assert(2 * counter_triples_used == num_triples);

    // cleanup
    delete[] ei;
    delete[] fi;
    delete[] e;
    delete[] f;
  }
};

#endif // MILLIONAIRE_WITH_EQ_H__
