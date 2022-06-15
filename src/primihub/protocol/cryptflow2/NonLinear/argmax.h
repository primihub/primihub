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

#ifndef ARGMAX_H__
#define ARGMAX_H__

#include "src/primihub/protocol/cryptflow2/NonLinear/relu-field.h"
#include "src/primihub/protocol/cryptflow2/NonLinear/relu-ring.h"
namespace primihub::cryptflow2
{

  template <typename type>
  class ArgMaxProtocol
  {
  public:
    primihub::sci::IOPack *iopack = nullptr;
    primihub::sci::OTPack *otpack = nullptr;
    ReLURingProtocol<type> *relu_oracle = nullptr;
    ReLUFieldProtocol<type> *relu_field_oracle = nullptr;
    int party;
    int algeb_str;
    int l, b;
    int num_cmps;
    uint64_t prime_mod;
    uint8_t zero_small = 0;
    uint64_t mask32_lower = (1ULL << 32) - 1ULL;
    uint64_t mask32_upper = -1ULL - mask32_lower;
    uint64_t mask_upper, mask_lower;
    bool createdReluObj = false;
    type mask_l;

    // Constructor
    ArgMaxProtocol(int party, int algeb_str, primihub::sci::IOPack *iopack, int l, int b,
                   uint64_t prime, primihub::sci::OTPack *otpack)
    {
      this->party = party;
      this->algeb_str = algeb_str;
      this->iopack = iopack;
      this->l = l;
      mask_lower = (1ULL << this->l) - 1;
      mask_upper = (1ULL << (2 * this->l)) - 1 - mask_lower;
      this->b = b;
      this->prime_mod = prime;
      this->otpack = otpack;
      if (algeb_str == RING)
      {
        this->relu_oracle =
            new ReLURingProtocol<type>(party, RING, iopack, l, b, otpack);
      }
      else
      {
        this->relu_field_oracle = new ReLUFieldProtocol<type>(
            party, FIELD, iopack, l, b, this->prime_mod, otpack);
      }
      configure();
    }

    // Destructor
    ~ArgMaxProtocol()
    {
      if (algeb_str == RING)
        delete relu_oracle;
      else
        delete relu_field_oracle;
    }

    void configure()
    {
      if (this->l != 32 && this->l != 64)
      {
        mask_l = (type)((1ULL << l) - 1);
      }
      else if (this->l == 32)
      {
        mask_l = -1;
      }
      else
      { // l = 64
        mask_l = -1ULL;
      }
    }

    int next_eight_multiple(int val)
    {
      for (int i = 0; i < 8; i++)
      {
        if ((val + i) % 8 == 0)
        {
          return (val + i);
        }
      }
      return 0;
    }

    void ArgMaxMPC(int size, type *inpArr, type *maxi, bool get_max_too = false,
                   type *max_val = nullptr)
    {
      type *input_temp = new type[size + 16];
      type *input_argmax_temp = new type[size + 16];
      for (int i = 0; i < size; i++)
      {
        input_temp[i] = inpArr[i];
        input_argmax_temp[i] = 0;
      }
      if (party == primihub::sci::ALICE)
      {
        for (type i = 0; i < (type)size; i++)
        {
          input_argmax_temp[i] = i;
        }
      }
      if (size & 1)
      {
        input_temp[size] = input_temp[size - 1];
        input_argmax_temp[size] = input_argmax_temp[size - 1];
        size += 1;
      }
      type *compare_with = new type[size + 16];
      type *compare_with_argmax = new type[size + 16];
      type *relu_res = new type[size + 16];
      type *argmax_res = new type[size + 16];
      int no_of_nodes = size;
      int no_of_nodes_child;
      int pad1, pad2;
      int times_stuck_on_8 = 0;

      while (no_of_nodes > 1)
      {
        // std::cout<<"#nodes = "<<no_of_nodes<<std::endl;
        no_of_nodes_child = (int)ceil((float)no_of_nodes / (float)2);
        pad1 = next_eight_multiple(no_of_nodes_child);
        pad1 = pad1 - no_of_nodes_child;
        pad2 = no_of_nodes + 2 * pad1;
        no_of_nodes_child += pad1;
        if (no_of_nodes_child == 8)
        {
          times_stuck_on_8++;
        }
        if (times_stuck_on_8 >= 5)
        {
          // The backend code only supports a minimum batch size of 8
          // So, whenever we have less than 8 child nodes, we pad it to get 8
          // nodes. The child nodes are = 8 in the following cases: #parentnodes =
          // 16, 8, 4, 2, 1. We will get the argmax result when parent nodes = 1.
          // So, times_stuck_on_8 >= 5.
          break;
        }
        for (int i = no_of_nodes; i < pad2; i++)
        {
          input_temp[i] = input_temp[no_of_nodes - 1];
          input_argmax_temp[i] = input_argmax_temp[no_of_nodes - 1];
        }
        if (this->algeb_str == FIELD)
        {
          for (int i = 0; i < (pad2); i += 2)
          {
            compare_with[i / 2] = primihub::sci::neg_mod(
                (int64_t)(input_temp[i] - input_temp[i + 1]), this->prime_mod);
            compare_with_argmax[i / 2] = primihub::sci::neg_mod(
                (int64_t)(input_argmax_temp[i] - input_argmax_temp[i + 1]),
                this->prime_mod);
          }
        }
        else
        { // RING
          for (int i = 0; i < (pad2); i += 2)
          {
            compare_with[i / 2] = (input_temp[i] - input_temp[i + 1]);
            compare_with_argmax[i / 2] =
                (input_argmax_temp[i] - input_argmax_temp[i + 1]);
          }
        }
        if (this->l > 32)
        {
          argmax_this_level_super_32(argmax_res, relu_res, compare_with_argmax,
                                     compare_with, no_of_nodes_child);
        }
        else
        {
          argmax_this_level_sub_32(argmax_res, relu_res, compare_with_argmax,
                                   compare_with, no_of_nodes_child);
        }
        if (this->algeb_str == FIELD)
        {
          for (int i = 0; i < (no_of_nodes_child); i++)
          {
            input_temp[i] =
                (relu_res[i] + input_temp[2 * i + 1]) % this->prime_mod;
            input_argmax_temp[i] =
                (argmax_res[i] + input_argmax_temp[2 * i + 1]) % this->prime_mod;
          }
        }
        else
        { // RING
          for (int i = 0; i < (no_of_nodes_child); i++)
          {
            input_temp[i] = (relu_res[i] + input_temp[2 * i + 1]) & mask_l;
            input_argmax_temp[i] =
                (argmax_res[i] + input_argmax_temp[2 * i + 1]) & mask_l;
          }
        }
        no_of_nodes = no_of_nodes_child;
      }
      maxi[0] = input_argmax_temp[0];
      if (get_max_too)
      {
        max_val[0] = input_temp[0];
      }
      if (this->algeb_str == RING)
      {
        maxi[0] &= mask_l;
        if (get_max_too)
        {
          max_val[0] &= mask_l;
        }
      }

      delete[] argmax_res;
      delete[] relu_res;
      delete[] compare_with_argmax;
      delete[] compare_with;
      delete[] input_temp;
      delete[] input_argmax_temp;
    }

    /**************************************************************************************************
     *                           Compute ArgMax for a tree level
     **************************************************************************************************/

    void argmax_this_level_super_32(type *argmax, type *result, type *indexshare,
                                    type *share, int num_relu)
    {
      uint8_t *drelu_ans = new uint8_t[num_relu];
      if (this->algeb_str == FIELD)
      {
        relu_field_oracle->relu(result, share, num_relu, drelu_ans, true);
      }
      else
      { // RING
        relu_oracle->relu(result, share, num_relu, drelu_ans, true);
      }

      // Now perform x.msb(x)
      // 2 OTs required with reversed roles
      primihub::sci::block128 *ot_messages_0 = new primihub::sci::block128[num_relu];
      primihub::sci::block128 *ot_messages_1 = new primihub::sci::block128[num_relu];

      uint64_t *additive_masks = new uint64_t[num_relu * 2];
      primihub::sci::block128 *received_shares = new primihub::sci::block128[num_relu];
      uint64_t *received_shares_0 = new uint64_t[num_relu];
      uint64_t *received_shares_1 = new uint64_t[num_relu];

      if (this->algeb_str == FIELD)
      {
        this->relu_field_oracle->triple_gen->prg->template random_mod_p<type>(
            (type *)additive_masks, 2 * num_relu, this->prime_mod);
        for (int i = 0; i < 2 * num_relu; i++)
        {
          additive_masks[i] %= this->prime_mod;
        }
      }
      else
      { // RING
        this->relu_oracle->triple_gen->prg->random_data(
            additive_masks, 2 * num_relu * sizeof(type));
      }
      for (int i = 0; i < num_relu; i++)
      {
        set_argmax_end_ot_messages_super_32(
            ot_messages_0 + i, ot_messages_1 + i, share + i, indexshare + i,
            drelu_ans + i, ((type *)additive_masks) + i, num_relu);
      }
#pragma omp parallel num_threads(2)
      {
        if (omp_get_thread_num() == 1)
        {
          if (party == primihub::sci::ALICE)
          {
            if (this->algeb_str == FIELD)
            {
              relu_field_oracle->otpack->iknp_reversed->recv(
                  received_shares, (bool *)drelu_ans, num_relu);
            }
            else
            {
              relu_oracle->otpack->iknp_reversed->recv(
                  received_shares, (bool *)drelu_ans, num_relu);
            }
          }
          else
          { // party == primihub::sci::BOB
            if (this->algeb_str == FIELD)
            {
              relu_field_oracle->otpack->iknp_reversed->send(
                  ot_messages_0, ot_messages_1, num_relu);
            }
            else
            {
              relu_oracle->otpack->iknp_reversed->send(ot_messages_0,
                                                       ot_messages_1, num_relu);
            }
          }
        }
        else
        {
          if (party == primihub::sci::ALICE)
          {
            if (this->algeb_str == FIELD)
            {
              relu_field_oracle->otpack->iknp_straight->send(
                  ot_messages_0, ot_messages_1, num_relu);
            }
            else
            {
              relu_oracle->otpack->iknp_straight->send(ot_messages_0,
                                                       ot_messages_1, num_relu);
            }
          }
          else
          { // party == primihub::sci::BOB
            if (this->algeb_str == FIELD)
            {
              relu_field_oracle->otpack->iknp_straight->recv(
                  received_shares, (bool *)drelu_ans, num_relu);
            }
            else
            {
              relu_oracle->otpack->iknp_straight->recv(
                  received_shares, (bool *)drelu_ans, num_relu);
            }
          }
        }
      }
      for (int i = 0; i < num_relu; i++)
      {
        received_shares_0[i] = _mm_extract_epi64(received_shares[i], 1);
        received_shares_1[i] = _mm_extract_epi64(received_shares[i], 0);
      }
      for (int i = 0; i < num_relu; i++)
      {
        result[i] = ((type *)additive_masks)[i] +
                    ((type *)received_shares_0)[(8 / sizeof(type)) * i];
        argmax[i] = ((type *)additive_masks)[i + num_relu] +
                    ((type *)received_shares_1)[(8 / sizeof(type)) * i];
        if (this->algeb_str == FIELD)
        {
          result[i] %= this->prime_mod;
          argmax[i] %= this->prime_mod;
        }
      }

      delete[] additive_masks;
      delete[] received_shares;
      delete[] received_shares_0;
      delete[] received_shares_1;
      delete[] ot_messages_0;
      delete[] ot_messages_1;
      delete[] drelu_ans;
    }

    void set_argmax_end_ot_messages_super_32(primihub::sci::block128 *ot_messages_0,
                                             primihub::sci::block128 *ot_messages_1,
                                             type *value_share, type *index_share,
                                             uint8_t *xor_share,
                                             type *additive_mask, int num_relu)
    {
      type temp0, temp1, temp2, temp3;
      if (this->algeb_str == FIELD)
      {
        temp0 = primihub::sci::neg_mod((int64_t)value_share[0] - (int64_t)additive_mask[0],
                                       this->prime_mod);
        temp1 = primihub::sci::neg_mod((int64_t)0LL - (int64_t)additive_mask[0],
                                       this->prime_mod);
        temp2 = primihub::sci::neg_mod((int64_t)index_share[0] -
                                           (int64_t)additive_mask[0 + num_relu],
                                       this->prime_mod);
        temp3 = primihub::sci::neg_mod((int64_t)0LL - (int64_t)additive_mask[0 + num_relu],
                                       this->prime_mod);
      }
      else
      { // RING
        temp0 = (value_share[0] - additive_mask[0]);
        temp1 = (0 - additive_mask[0]);
        temp2 = (index_share[0] - additive_mask[0 + num_relu]);
        temp3 = (0 - additive_mask[0 + num_relu]);
      }
      if (*xor_share == zero_small)
      {
        ot_messages_0[0] = primihub::sci::makeBlock128(0ULL + temp0, 0ULL + temp2);
        ot_messages_1[0] = primihub::sci::makeBlock128(0ULL + temp1, 0ULL + temp3);
      }
      else
      {
        ot_messages_0[0] = primihub::sci::makeBlock128(0ULL + temp1, 0ULL + temp3);
        ot_messages_1[0] = primihub::sci::makeBlock128(0ULL + temp0, 0ULL + temp2);
      }
    }

    void argmax_this_level_sub_32(type *argmax, type *result, type *indexshare,
                                  type *share, int num_relu)
    {
      uint8_t *drelu_ans = new uint8_t[num_relu];

      if (this->algeb_str == FIELD)
      {
        relu_field_oracle->relu(result, share, num_relu, drelu_ans, true);
      }
      else
      { // RING
        relu_oracle->relu(result, share, num_relu, drelu_ans, true);
      }

      // Now perform x.msb(x)
      // 2 OTs required with reversed roles
      uint64_t **ot_messages = new uint64_t *[num_relu];
      for (int i = 0; i < num_relu; i++)
      {
        ot_messages[i] = new uint64_t[2];
      }
      uint64_t *additive_masks = new uint64_t[2 * num_relu];

      uint64_t *received_shares = new uint64_t[num_relu];
      uint64_t *received_shares_0 = new uint64_t[num_relu];
      uint64_t *received_shares_1 = new uint64_t[num_relu];

      if (this->algeb_str == FIELD)
      {
        this->relu_field_oracle->triple_gen->prg->template random_mod_p<type>(
            (type *)additive_masks, 2 * num_relu, this->prime_mod);
        for (int i = 0; i < 2 * num_relu; i++)
        {
          additive_masks[i] %= this->prime_mod;
        }
      }
      else
      { // RING
        this->relu_oracle->triple_gen->prg->random_data(
            additive_masks, 2 * num_relu * sizeof(type));
      }
      if (party == primihub::sci::ALICE)
      {
        for (int i = 0; i < num_relu; i++)
        {
          set_argmax_end_ot_messages_sub_32(
              ot_messages[i], share + i, indexshare + i, drelu_ans + i,
              ((type *)additive_masks) + i, num_relu);
        }
        if (this->algeb_str == FIELD)
        {
          relu_field_oracle->otpack->iknp_straight->send(ot_messages, num_relu,
                                                         this->l * 2);
          relu_field_oracle->otpack->iknp_reversed->recv(
              received_shares, drelu_ans, num_relu, this->l * 2);
        }
        else
        { // RING
          relu_oracle->otpack->iknp_straight->send(ot_messages, num_relu, 64);
          relu_oracle->otpack->iknp_reversed->recv(received_shares, drelu_ans,
                                                   num_relu, 64);
        }
      }
      else // party = primihub::sci::BOB
      {
        for (int i = 0; i < num_relu; i++)
        {
          set_argmax_end_ot_messages_sub_32(
              ot_messages[i], share + i, indexshare + i, drelu_ans + i,
              ((type *)additive_masks) + i, num_relu);
        }
        if (this->algeb_str == FIELD)
        {
          relu_field_oracle->otpack->iknp_straight->recv(
              received_shares, drelu_ans, num_relu, this->l * 2);
          relu_field_oracle->otpack->iknp_reversed->send(ot_messages, num_relu,
                                                         this->l * 2);
        }
        else
        { // RING
          relu_oracle->otpack->iknp_straight->recv(received_shares, drelu_ans,
                                                   num_relu, 64);
          relu_oracle->otpack->iknp_reversed->send(ot_messages, num_relu, 64);
        }
      }

      for (int i = 0; i < num_relu; i++)
      {
        if (this->algeb_str == FIELD)
        {
          // tightly optimized communication for the field case
          received_shares_0[i] = (received_shares[i] & mask_upper) >> this->l;
          received_shares_1[i] = received_shares[i] & mask_lower;
        }
        else
        { // RING
          received_shares_0[i] = (received_shares[i] & mask32_upper) >> 32;
          received_shares_1[i] = received_shares[i] & mask32_lower;
        }
      }
      for (int i = 0; i < num_relu; i++)
      {
        result[i] = ((type *)additive_masks)[i] +
                    ((type *)received_shares_0)[(8 / sizeof(type)) * i];
        argmax[i] = ((type *)additive_masks)[i + num_relu] +
                    ((type *)received_shares_1)[(8 / sizeof(type)) * i];
        if (this->algeb_str == FIELD)
        {
          result[i] %= this->prime_mod;
          argmax[i] %= this->prime_mod;
        }
      }
      delete[] additive_masks;
      delete[] received_shares;
      delete[] received_shares_0;
      delete[] received_shares_1;
      delete[] drelu_ans;
      for (int i = 0; i < num_relu; i++)
      {
        delete[] ot_messages[i];
      }
      delete[] ot_messages;
    }

    void set_argmax_end_ot_messages_sub_32(uint64_t *ot_messages,
                                           type *value_share, type *index_share,
                                           uint8_t *xor_share,
                                           type *additive_mask, int num_relu)
    {
      uint64_t temp0, temp1, temp2, temp3;
      uint64_t mask_upper_general, mask_lower_general;
      if (this->algeb_str == FIELD)
      {
        temp0 = primihub::sci::neg_mod((int64_t)value_share[0] - (int64_t)additive_mask[0],
                                       this->prime_mod);
        temp1 = primihub::sci::neg_mod((int64_t)0LL - (int64_t)additive_mask[0],
                                       this->prime_mod);
        temp2 = primihub::sci::neg_mod((int64_t)index_share[0] -
                                           (int64_t)additive_mask[0 + num_relu],
                                       this->prime_mod);
        temp3 = primihub::sci::neg_mod((int64_t)0LL - (int64_t)additive_mask[0 + num_relu],
                                       this->prime_mod);
        temp0 = temp0 << this->l;
        temp1 = temp1 << this->l;
        mask_upper_general = mask_upper;
        mask_lower_general = mask_lower;
      }
      else
      { // RING
        temp0 = (type)(value_share[0] - additive_mask[0]);
        temp0 = temp0 << 32;
        temp1 = (type)(0 - additive_mask[0]);
        temp1 = temp1 << 32;
        temp2 = (type)(index_share[0] - additive_mask[0 + num_relu]);
        temp3 = (type)(0 - additive_mask[0 + num_relu]);
        mask_upper_general = mask32_upper;
        mask_lower_general = mask32_lower;
      }
      if (*xor_share == zero_small)
      {
        ot_messages[0] = (mask_upper_general & (0ULL + temp0)) ^
                         (mask_lower_general & (0ULL + temp2));
        ot_messages[1] = (mask_upper_general & (0ULL + temp1)) ^
                         (mask_lower_general & (0ULL + temp3));
      }
      else
      {
        ot_messages[0] = (mask_upper_general & (0ULL + temp1)) ^
                         (mask_lower_general & (0ULL + temp3));
        ot_messages[1] = (mask_upper_general & (0ULL + temp0)) ^
                         (mask_lower_general & (0ULL + temp2));
      }
    }
  };
}
#endif // ARGMAX_H__
