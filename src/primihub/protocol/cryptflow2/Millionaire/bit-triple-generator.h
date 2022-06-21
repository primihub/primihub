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

#ifndef TRIPLE_GENERATOR_H__
#define TRIPLE_GENERATOR_H__
#include "src/primihub/protocol/cryptflow2/OT/ot_pack.h"

enum TripleGenMethod {
  Ideal,          // (Insecure) Ideal Functionality
  _2ROT,          // 1 Bit Triple from 2 ROT
  _16KKOT_to_4OT, // 2 Bit Triples from 1oo16 KKOT to 1oo4 OT
  _8KKOT,         // 2 Correlated Bit Triples from 1oo8 KKOT
};

class Triple {
public:
  bool packed;
  uint8_t *ai;
  uint8_t *bi;
  uint8_t *ci;
  int num_triples, num_bytes, offset;

  Triple(int num_triples, bool packed = false, int offset = 0) {
    assert((offset < num_triples) || (num_triples == 0));
    this->num_triples = num_triples;
    this->packed = packed;
    if (packed) {
      assert(num_triples % 8 == 0);
      assert(offset % 8 == 0);
      this->num_bytes = num_triples / 8;
    } else
      this->num_bytes = num_triples;
    if (offset == 0)
      this->offset = 1;
    else
      this->offset = offset;
    assert((num_triples % this->offset) == 0);
    this->ai = new uint8_t[num_bytes];
    this->bi = new uint8_t[num_bytes];
    this->ci = new uint8_t[num_bytes];
  }

  ~Triple() {
    delete[] ai;
    delete[] bi;
    delete[] ci;
  }
};

class TripleGenerator {
public:
  primihub::sci::IOPack *iopack;
  primihub::sci::OTPack *otpack;
  primihub::sci::PRG128 *prg;
  int party;

  TripleGenerator(int party, primihub::sci::IOPack *iopack, primihub::sci::OTPack *otpack) {
    this->iopack = iopack;
    this->otpack = otpack;
    this->prg = new primihub::sci::PRG128;
  }

  ~TripleGenerator() { delete prg; }

  void generate(int party, uint8_t *ai, uint8_t *bi, uint8_t *ci,
                int num_triples, TripleGenMethod method, bool packed = false,
                int offset = 1) {
    if (!num_triples)
      return;
    switch (method) {
    case Ideal: {
      int num_bytes = ceil((double)num_triples / 8);
      if (party == primihub::sci::ALICE) {
        uint8_t *a = new uint8_t[num_bytes];
        uint8_t *b = new uint8_t[num_bytes];
        uint8_t *c = new uint8_t[num_bytes];
        if (packed) {
          prg->random_data(ai, num_bytes);
          prg->random_data(bi, num_bytes);
          prg->random_data(ci, num_bytes);
        } else {
          prg->random_bool((bool *)ai, num_triples);
          prg->random_bool((bool *)bi, num_triples);
          prg->random_bool((bool *)ci, num_triples);
        }
        prg->random_data(a, num_bytes);
        prg->random_data(b, num_bytes);
        for (int i = 0; i < num_triples; i += 8) {
          c[i / 8] = a[i / 8] & b[i / 8];
          if (packed) {
            a[i / 8] ^= ai[i / 8];
            b[i / 8] ^= bi[i / 8];
            c[i / 8] ^= ci[i / 8];
          } else {
            uint8_t temp_a, temp_b, temp_c;
            if (num_triples - i >= 8) {
              temp_a = primihub::sci::bool_to_uint8(ai + i, 8);
              temp_b = primihub::sci::bool_to_uint8(bi + i, 8);
              temp_c = primihub::sci::bool_to_uint8(ci + i, 8);
            } else {
              temp_a = primihub::sci::bool_to_uint8(ai + i, num_triples - i);
              temp_b = primihub::sci::bool_to_uint8(bi + i, num_triples - i);
              temp_c = primihub::sci::bool_to_uint8(ci + i, num_triples - i);
            }
            a[i / 8] ^= temp_a;
            b[i / 8] ^= temp_b;
            c[i / 8] ^= temp_c;
          }
        }
        iopack->io->send_data(a, num_bytes);
        iopack->io->send_data(b, num_bytes);
        iopack->io->send_data(c, num_bytes);
        delete[] a;
        delete[] b;
        delete[] c;
      } else {
        if (packed) {
          iopack->io->recv_data(ai, num_bytes);
          iopack->io->recv_data(bi, num_bytes);
          iopack->io->recv_data(ci, num_bytes);
        } else {
          uint8_t *a = new uint8_t[num_bytes];
          uint8_t *b = new uint8_t[num_bytes];
          uint8_t *c = new uint8_t[num_bytes];
          iopack->io->recv_data(a, num_bytes);
          iopack->io->recv_data(b, num_bytes);
          iopack->io->recv_data(c, num_bytes);

          for (int i = 0; i < num_triples; i += 8) {
            if (num_triples - i >= 8) {
              primihub::sci::uint8_to_bool(ai + i, a[i / 8], 8);
              primihub::sci::uint8_to_bool(bi + i, b[i / 8], 8);
              primihub::sci::uint8_to_bool(ci + i, c[i / 8], 8);
            } else {
              primihub::sci::uint8_to_bool(ai + i, a[i / 8], num_triples - i);
              primihub::sci::uint8_to_bool(bi + i, b[i / 8], num_triples - i);
              primihub::sci::uint8_to_bool(ci + i, c[i / 8], num_triples - i);
            }
          }
          delete[] a;
          delete[] b;
          delete[] c;
        }
      }
      break;
    }
    case _2ROT: {
      throw std::invalid_argument("To be implemented");
      break;
    }
    case _16KKOT_to_4OT: {
      assert((num_triples & 1) == 0); // num_triples is even
      uint8_t *a, *b, *c;
      if (packed) {
        a = new uint8_t[num_triples];
        b = new uint8_t[num_triples];
        c = new uint8_t[num_triples];
      } else {
        a = ai;
        b = bi;
        c = ci;
      }
      prg->random_bool((bool *)a, num_triples);
      prg->random_bool((bool *)b, num_triples);
      switch (party) {
      case primihub::sci::ALICE: {
        prg->random_bool((bool *)c, num_triples);
        uint8_t **ot_messages; // (num_triples/2) X 16
        ot_messages = new uint8_t *[num_triples / 2];
        for (int i = 0; i < num_triples; i += 2)
          ot_messages[i / 2] = new uint8_t[16];
        for (int j = 0; j < 16; j++) {
          uint8_t bits_j[4]; // a01 || b01 || a11 || b11 (LSB->MSB)
          primihub::sci::uint8_to_bool(bits_j, j, 4);
          for (int i = 0; i < num_triples; i += 2) {
            ot_messages[i / 2][j] =
                ((((a[i + 1] ^ bits_j[2]) & (b[i + 1] ^ bits_j[3])) ^ c[i + 1])
                 << 1) |
                (((a[i] ^ bits_j[0]) & (b[i] ^ bits_j[1])) ^ c[i]);
          }
        }
        // otpack->kkot_16->send(ot_messages, num_triples/2, 2);
        otpack->kkot[3]->send(ot_messages, num_triples / 2, 2);
        for (int i = 0; i < num_triples; i += 2)
          delete[] ot_messages[i / 2];
        delete[] ot_messages;
        break;
      }
      case primihub::sci::BOB: {
        uint8_t *ot_selection = new uint8_t[(size_t)num_triples / 2];
        uint8_t *ot_result = new uint8_t[(size_t)num_triples / 2];
        for (int i = 0; i < num_triples; i += 2) {
          ot_selection[i / 2] =
              (b[i + 1] << 3) | (a[i + 1] << 2) | (b[i] << 1) | a[i];
        }
        // otpack->kkot_16->recv(ot_result, ot_selection, num_triples/2, 2);
        otpack->kkot[3]->recv(ot_result, ot_selection, num_triples / 2, 2);
        for (int i = 0; i < num_triples; i += 2) {
          c[i] = ot_result[i / 2] & 1;
          c[i + 1] = ot_result[i / 2] >> 1;
        }
        delete[] ot_selection;
        delete[] ot_result;
        break;
      }
      }
      if (packed) {
        for (int i = 0; i < num_triples; i += 8) {
          ai[i / 8] = primihub::sci::bool_to_uint8(a + i, 8);
          bi[i / 8] = primihub::sci::bool_to_uint8(b + i, 8);
          ci[i / 8] = primihub::sci::bool_to_uint8(c + i, 8);
        }
        delete[] a;
        delete[] b;
        delete[] c;
      }
      break;
    }
    case _8KKOT: {
      assert((num_triples & 1) == 0); // num_triples is even
      uint8_t *a, *b, *c;
      if (packed) {
        a = new uint8_t[num_triples];
        b = new uint8_t[num_triples];
        c = new uint8_t[num_triples];
      } else {
        a = ai;
        b = bi;
        c = ci;
      }
      for (int i = 0; i < num_triples; i += 2 * offset) {
        prg->random_bool((bool *)a + i, offset);
        memcpy(a + i + offset, a + i, offset);
      }
      prg->random_bool((bool *)b, num_triples);
      switch (party) {
      case primihub::sci::ALICE: {
        prg->random_bool((bool *)c, num_triples);
        uint8_t **ot_messages; // (num_triples/2) X 8
        ot_messages = new uint8_t *[num_triples / 2];
        for (int i = 0; i < num_triples; i += 2)
          ot_messages[i / 2] = new uint8_t[8];
        for (int j = 0; j < 8; j++) {
          uint8_t bits_j[3]; // a01 || b01 || b11 (LSB->MSB)
          primihub::sci::uint8_to_bool(bits_j, j, 3);
          for (int i = 0; i < num_triples; i += 2 * offset) {
            for (int k = 0; k < offset; k++) {
              ot_messages[i / 2 + k][j] =
                  ((((a[i + k] ^ bits_j[0]) & (b[i + offset + k] ^ bits_j[2])) ^
                    c[i + offset + k])
                   << 1) |
                  (((a[i + k] ^ bits_j[0]) & (b[i + k] ^ bits_j[1])) ^
                   c[i + k]);
            }
          }
        }
        // otpack->kkot_8->send(ot_messages, num_triples/2, 2);
        otpack->kkot[2]->send(ot_messages, num_triples / 2, 2);
        for (int i = 0; i < num_triples; i += 2)
          delete[] ot_messages[i / 2];
        delete[] ot_messages;
        break;
      }
      case primihub::sci::BOB: {
        uint8_t *ot_selection = new uint8_t[(size_t)num_triples / 2];
        uint8_t *ot_result = new uint8_t[(size_t)num_triples / 2];
        for (int i = 0; i < num_triples; i += 2 * offset) {
          for (int k = 0; k < offset; k++)
            ot_selection[i / 2 + k] =
                (b[i + offset + k] << 2) | (b[i + k] << 1) | a[i + k];
        }
        // otpack->kkot_8->recv(ot_result, ot_selection, num_triples/2, 2);
        otpack->kkot[2]->recv(ot_result, ot_selection, num_triples / 2, 2);
        for (int i = 0; i < num_triples; i += 2 * offset) {
          for (int k = 0; k < offset; k++) {
            c[i + k] = ot_result[i / 2 + k] & 1;
            c[i + offset + k] = ot_result[i / 2 + k] >> 1;
          }
        }
        delete[] ot_selection;
        delete[] ot_result;
        break;
      }
      }
      if (packed) {
        for (int i = 0; i < num_triples; i += 8) {
          ai[i / 8] = primihub::sci::bool_to_uint8(a + i, 8);
          bi[i / 8] = primihub::sci::bool_to_uint8(b + i, 8);
          ci[i / 8] = primihub::sci::bool_to_uint8(c + i, 8);
        }
        delete[] a;
        delete[] b;
        delete[] c;
      }
      break;
    }
    }
  }

  void generate(int party, Triple *triples, TripleGenMethod method) {
    generate(party, triples->ai, triples->bi, triples->ci, triples->num_triples,
             method, triples->packed, triples->offset);
  }
};
#endif // TRIPLE_GENERATOR_H__
