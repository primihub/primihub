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

#ifndef RELU_RING_H__
#define RELU_RING_H__

#include "src/primihub/protocol/cryptflow2/Millionaire/millionaire.h"
#include "src/primihub/protocol/cryptflow2/NonLinear/relu-interface.h"
#include <omp.h>

#define RING 0
#define OFF_PLACE
namespace primihub::cryptflow2
{
  template <typename type>
  class ReLURingProtocol : public ReLUProtocol<type>
  {
  public:
    primihub::sci::IOPack *iopack;
    primihub::sci::OTPack *otpack;
    TripleGenerator *triple_gen;
    MillionaireProtocol *millionaire;
    int party;
    int algeb_str;
    int l, b;
    int num_cmps;
    uint8_t two_small = 1 << 1;
    uint8_t zero_small = 0;
    uint64_t mask_take_32 = -1;
    uint64_t msb_one;
    uint64_t cut_mask;
    uint64_t relu_comparison_rhs;
    type mask_l;
    type relu_comparison_rhs_type;
    type cut_mask_type;
    type msb_one_type;

    // Constructor
    ReLURingProtocol(int party, int algeb_str, primihub::sci::IOPack *iopack, int l, int b,
                     primihub::sci::OTPack *otpack)
    {
      this->party = party;
      this->algeb_str = algeb_str;
      this->iopack = iopack;
      this->l = l;
      this->b = b;
      this->otpack = otpack;
      this->millionaire = new MillionaireProtocol(party, iopack, otpack);
      this->triple_gen = this->millionaire->triple_gen;
      configure();
    }

    // Destructor
    virtual ~ReLURingProtocol() { delete millionaire; }

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
      if (sizeof(type) == sizeof(uint64_t))
      {
        msb_one = (1ULL << (this->l - 1));
        relu_comparison_rhs_type = msb_one - 1ULL;
        relu_comparison_rhs = relu_comparison_rhs_type;
        cut_mask_type = relu_comparison_rhs_type;
        cut_mask = cut_mask_type;
      }
      else
      {
        msb_one_type = (1 << (this->l - 1));
        relu_comparison_rhs_type = msb_one_type - 1;
        relu_comparison_rhs = relu_comparison_rhs_type + 0ULL;
        cut_mask_type = relu_comparison_rhs_type;
        cut_mask = cut_mask_type + 0ULL;
      }
    }

    // Ideal Functionality
    void drelu_ring_ideal_func(uint8_t *result, type *sh1, type *sh2,
                               int num_relu)
    {
      uint8_t *msb1 = new uint8_t[num_relu];
      uint8_t *msb2 = new uint8_t[num_relu];
      type *plain_value = new type[num_relu];
      for (int i = 0; i < num_relu; i++)
      {
        plain_value[i] = sh1[i] + sh2[i];
      }
      uint8_t *actual_drelu = new uint8_t[num_relu];

      uint64_t index_fetch = (sizeof(type) == sizeof(uint64_t)) ? 7 : 3;
      for (int i = 0; i < num_relu; i++)
      {
        msb1[i] = (*((uint8_t *)(&(sh1[i])) + index_fetch)) >> 7;
        msb2[i] = (*((uint8_t *)(&(sh2[i])) + index_fetch)) >> 7;
        actual_drelu[i] = (*((uint8_t *)(&(plain_value[i])) + index_fetch)) >> 7;
      }

      type *sh1_cut = new type[num_relu];
      type *sh2_cut = new type[num_relu];
      uint8_t *wrap = new uint8_t[num_relu];
      uint8_t *wrap_orig = new uint8_t[num_relu];
      uint8_t *relu_comparison_avoid_warning = new uint8_t[sizeof(type)];
      memcpy(relu_comparison_avoid_warning, &relu_comparison_rhs, sizeof(type));
      for (int i = 0; i < num_relu; i++)
      {
        sh1_cut[i] = sh1[i] & cut_mask;
        sh2_cut[i] = sh2[i] & cut_mask;
        wrap_orig[i] =
            ((sh1_cut[i] + sh2_cut[i]) > *(type *)relu_comparison_avoid_warning);
        wrap[i] = wrap_orig[i];
        wrap[i] ^= msb1[i];
        wrap[i] ^= msb2[i];
      }
      memcpy(result, wrap, num_relu);
      for (int i = 0; i < num_relu; i++)
      {
        assert((wrap[i] == actual_drelu[i]) &&
               "The computed DReLU did not match the actual DReLU");
      }
    }

    void relu(type *result, type *share, int num_relu,
              uint8_t *drelu_res = nullptr, bool skip_ot = false)
    {
      uint8_t *msb_local_share = new uint8_t[num_relu];
      uint64_t *array64;
      type *array_type;
      array64 = new uint64_t[num_relu];
      array_type = new type[num_relu];

      if (this->algeb_str == RING)
      {
        this->num_cmps = num_relu;
      }
      else
      {
        abort();
      }
      uint8_t *wrap = new uint8_t[num_cmps];
      for (int i = 0; i < num_relu; i++)
      {
        msb_local_share[i] = (uint8_t)(share[i] >> (l - 1));
        array_type[i] = share[i] & cut_mask_type;
      }

      type temp;

      switch (this->party)
      {
      case primihub::sci::ALICE:
      {
        for (int i = 0; i < num_relu; i++)
        {
          array64[i] = array_type[i] + 0ULL;
        }
        break;
      }
      case primihub::sci::BOB:
      {
        for (int i = 0; i < num_relu; i++)
        {
          temp = this->relu_comparison_rhs_type -
                 array_type[i]; // This value is never negative.
          array64[i] = 0ULL + temp;
        }
        break;
      }
      }

      this->millionaire->compare(wrap, array64, num_cmps, l - 1, true, false, b);
      for (int i = 0; i < num_relu; i++)
      {
        msb_local_share[i] = (msb_local_share[i] + wrap[i]) % two_small;
      }

      if (drelu_res != nullptr)
      {
        for (int i = 0; i < num_relu; i++)
        {
          drelu_res[i] = msb_local_share[i];
        }
      }

      if (skip_ot)
      {
        delete[] msb_local_share;
        delete[] array64;
        delete[] array_type;
        return;
      }

      // Now perform x.msb(x)
      uint64_t **ot_messages = new uint64_t *[num_relu];
      for (int i = 0; i < num_relu; i++)
      {
        ot_messages[i] = new uint64_t[2];
      }
      uint64_t *additive_masks = new uint64_t[num_relu];
      uint64_t *received_shares = new uint64_t[num_relu];
      this->triple_gen->prg->random_data(additive_masks, num_relu * sizeof(type));
      for (int i = 0; i < num_relu; i++)
      {
        set_relu_end_ot_messages(ot_messages[i], share + i, msb_local_share + i,
                                 ((type *)additive_masks) + i);
      }
#pragma omp parallel num_threads(2)
      {
        if (omp_get_thread_num() == 1)
        {
          if (party == primihub::sci::ALICE)
          {
            otpack->iknp_reversed->recv(received_shares, msb_local_share,
                                        num_relu, this->l);
          }
          else
          { // party == primihub::sci::BOB
            otpack->iknp_reversed->send(ot_messages, num_relu, this->l);
          }
        }
        else
        {
          if (party == primihub::sci::ALICE)
          {
            otpack->iknp_straight->send(ot_messages, num_relu, this->l);
          }
          else
          { // party == primihub::sci::BOB
            otpack->iknp_straight->recv(received_shares, msb_local_share,
                                        num_relu, this->l);
          }
        }
      }
      for (int i = 0; i < num_relu; i++)
      {
        result[i] = ((type *)additive_masks)[i] +
                    ((type *)received_shares)[(8 / sizeof(type)) * i];
        result[i] &= mask_l;
      }
      delete[] msb_local_share;
      delete[] array64;
      delete[] array_type;
      delete[] additive_masks;
      delete[] received_shares;
      for (int i = 0; i < num_relu; i++)
      {
        delete[] ot_messages[i];
      }
      delete[] ot_messages;
      delete[] wrap;
    }

    void set_relu_end_ot_messages(uint64_t *ot_messages, type *value_share,
                                  uint8_t *xor_share, type *additive_mask)
    {
      type temp0, temp1;
      temp0 = (value_share[0] - additive_mask[0]);
      temp1 = (0 - additive_mask[0]);
      if (*xor_share == zero_small)
      {
        ot_messages[0] = 0ULL + temp0;
        ot_messages[1] = 0ULL + temp1;
      }
      else
      {
        ot_messages[0] = 0ULL + temp1;
        ot_messages[1] = 0ULL + temp0;
      }
    }
  };
}
#endif // RELU_RING_H__
