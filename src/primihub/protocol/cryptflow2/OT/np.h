/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

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

Enquiries about further applications and development opportunities are welcome.

Modified by Deevashwer Rathee
*/

#ifndef OT_NP_H__
#define OT_NP_H__
#include "src/primihub/protocol/cryptflow2/OT/ot.h"
/** @addtogroup OT
        @{
*/
namespace primihub::sci {
template <typename IO> class OTNP : public OT<OTNP<IO>> {
public:
  IO *io;
  primihub::emp::Group *G = nullptr;
  bool delete_G = true;
  OTNP(IO *io, primihub::emp::Group *_G = nullptr) {
    this->io = io;
    if (_G == nullptr)
      G = new primihub::emp::Group();
    else {
      G = _G;
      delete_G = false;
    }
  }
  ~OTNP() {
    if (delete_G)
      delete G;
  }

  void send_impl(const block128 *data0, const block128 *data1, int length) {
    primihub::emp::BigInt d;
    G->get_rand_bn(d);
    primihub::emp::Point C = G->mul_gen(d);
    io->send_pt(&C);
    io->flush();

    primihub::emp::BigInt *r = new primihub::emp::BigInt[length];
    primihub::emp::BigInt *rc = new primihub::emp::BigInt[length];
    primihub::emp::Point *pk0 = new primihub::emp::Point[length], pk1, *gr = new primihub::emp::Point[length],
               *Cr = new primihub::emp::Point[length];
    for (int i = 0; i < length; ++i) {
      G->get_rand_bn(r[i]);
      gr[i] = G->mul_gen(r[i]);
      rc[i] = r[i].mul(d, G->bn_ctx);
      rc[i] = rc[i].mod(G->order, G->bn_ctx);
      Cr[i] = G->mul_gen(rc[i]);
    }

    for (int i = 0; i < length; ++i) {
      io->recv_pt(G, &pk0[i]);
    }
    for (int i = 0; i < length; ++i) {
      io->send_pt(&gr[i]);
    }
    io->flush();

    block128 m[2];
    for (int i = 0; i < length; ++i) {
      pk0[i] = pk0[i].mul(r[i]);
      primihub::emp::Point inv = pk0[i].inv();
      pk1 = Cr[i].add(inv);
      m[0] = Hash::KDF128(pk0[i]);
      m[0] = xorBlocks(data0[i], m[0]);
      m[1] = Hash::KDF128(pk1);
      m[1] = xorBlocks(data1[i], m[1]);
      io->send_data(m, 2 * sizeof(block128));
    }

    delete[] r;
    delete[] gr;
    delete[] Cr;
    delete[] rc;
    delete[] pk0;
  }

  void send_impl(const block256 *data0, const block256 *data1, int length) {
    primihub::emp::BigInt d;
    G->get_rand_bn(d);
    primihub::emp::Point C = G->mul_gen(d);
    io->send_pt(&C);
    io->flush();

    primihub::emp::BigInt *r = new primihub::emp::BigInt[length];
    primihub::emp::BigInt *rc = new primihub::emp::BigInt[length];
    primihub::emp::Point *pk0 = new primihub::emp::Point[length], pk1, *gr = new primihub::emp::Point[length],
               *Cr = new primihub::emp::Point[length];
    for (int i = 0; i < length; ++i) {
      G->get_rand_bn(r[i]);
      gr[i] = G->mul_gen(r[i]);
      rc[i] = r[i].mul(d, G->bn_ctx);
      rc[i] = rc[i].mod(G->order, G->bn_ctx);
      Cr[i] = G->mul_gen(rc[i]);
    }

    for (int i = 0; i < length; ++i) {
      io->recv_pt(G, &pk0[i]);
    }
    for (int i = 0; i < length; ++i) {
      io->send_pt(&gr[i]);
    }
    io->flush();

    alignas(32) block256 m[2];
    for (int i = 0; i < length; ++i) {
      pk0[i] = pk0[i].mul(r[i]);
      primihub::emp::Point inv = pk0[i].inv();
      pk1 = Cr[i].add(inv);
      m[0] = Hash::KDF256(pk0[i]);
      m[0] = xorBlocks(data0[i], m[0]);
      m[1] = Hash::KDF256(pk1);
      m[1] = xorBlocks(data1[i], m[1]);
      io->send_data(m, 2 * sizeof(block256));
    }

    delete[] r;
    delete[] gr;
    delete[] Cr;
    delete[] rc;
    delete[] pk0;
  }

  void recv_impl(block128 *data, const bool *b, int length) {
    primihub::emp::BigInt *k = new primihub::emp::BigInt[length];
    primihub::emp::Point *gr = new primihub::emp::Point[length];
    primihub::emp::Point pk[2];
    block128 m[2];
    primihub::emp::Point C;
    for (int i = 0; i < length; ++i)
      G->get_rand_bn(k[i]);

    io->recv_pt(G, &C);

    for (int i = 0; i < length; ++i) {
      if (b[i]) {
        pk[1] = G->mul_gen(k[i]);
        primihub::emp::Point inv = pk[1].inv();
        pk[0] = C.add(inv);
      } else {
        pk[0] = G->mul_gen(k[i]);
      }
      io->send_pt(&pk[0]);
    }

    for (int i = 0; i < length; ++i) {
      io->recv_pt(G, &gr[i]);
      gr[i] = gr[i].mul(k[i]);
    }
    for (int i = 0; i < length; ++i) {
      int ind = b[i] ? 1 : 0;
      io->recv_data(m, 2 * sizeof(block128));
      data[i] = xorBlocks(m[ind], Hash::KDF128(gr[i]));
    }
    delete[] k;
    delete[] gr;
  }

  void recv_impl(block256 *data, const bool *b, int length) {
    primihub::emp::BigInt *k = new primihub::emp::BigInt[length];
    primihub::emp::Point *gr = new primihub::emp::Point[length];
    primihub::emp::Point pk[2];
    alignas(32) block256 m[2];
    primihub::emp::Point C;
    for (int i = 0; i < length; ++i)
      G->get_rand_bn(k[i]);

    io->recv_pt(G, &C);

    for (int i = 0; i < length; ++i) {
      if (b[i]) {
        pk[1] = G->mul_gen(k[i]);
        primihub::emp::Point inv = pk[1].inv();
        pk[0] = C.add(inv);
      } else {
        pk[0] = G->mul_gen(k[i]);
      }
      io->send_pt(&pk[0]);
    }

    for (int i = 0; i < length; ++i) {
      io->recv_pt(G, &gr[i]);
      gr[i] = gr[i].mul(k[i]);
    }
    for (int i = 0; i < length; ++i) {
      int ind = b[i] ? 1 : 0;
      io->recv_data(m, 2 * sizeof(block256));
      data[i] = xorBlocks(m[ind], Hash::KDF256(gr[i]));
    }
    delete[] k;
    delete[] gr;
  }
};
/**@}*/
} // namespace primihub::sci
#endif // OT_NP_H__
