/*
Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)

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
*/

#ifndef EMP_GROUP_H__
#define EMP_GROUP_H__

#include "src/primihub/protocol/cryptflow2/utils/utils.h"
#include <cstring>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <string>

//#ifdef ECC_USE_OPENSSL
//#else
//#include "group_relic.h"
//#endif
namespace primihub::emp {
class BigInt {
public:
  BIGNUM *n = nullptr;
  BigInt();
  BigInt(const BigInt &oth);
  BigInt &operator=(BigInt oth);
  ~BigInt();

  int size();
  void to_bin(unsigned char *in);
  void from_bin(const unsigned char *in, int length);

  BigInt add(const BigInt &oth);
  BigInt mul(const BigInt &oth, BN_CTX *ctx = nullptr);
  BigInt mod(const BigInt &oth, BN_CTX *ctx = nullptr);
  BigInt add_mod(const BigInt &b, const BigInt &m, BN_CTX *ctx = nullptr);
  BigInt mul_mod(const BigInt &b, const BigInt &m, BN_CTX *ctx = nullptr);
};
class Group;
class Point {
public:
  EC_POINT *point = nullptr;
  Group *group = nullptr;
  Point(Group *g = nullptr);
  ~Point();
  Point(const Point &p);
  Point &operator=(Point p);

  void to_bin(unsigned char *buf, size_t buf_len);
  size_t size();
  void from_bin(Group *g, const unsigned char *buf, size_t buf_len);

  Point add(Point &rhs);
  //		Point sub(Point & rhs);
  //		bool is_at_infinity();
  //		bool is_on_curve();
  Point mul(const BigInt &m);
  Point inv();
  bool operator==(Point &rhs);
};

class Group {
public:
  EC_GROUP *ec_group = nullptr;
  BN_CTX *bn_ctx = nullptr;
  BigInt order;
  unsigned char *scratch;
  size_t scratch_size = 256;
  Group();
  ~Group();
  void resize_scratch(size_t size);
  void get_rand_bn(BigInt &n);
  Point get_generator();
  Point mul_gen(const BigInt &m);
};

} // namespace primihub::emp
#include "group_openssl.h"

#endif
