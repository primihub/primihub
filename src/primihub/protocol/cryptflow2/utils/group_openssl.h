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

#ifndef EMP_GROUP_OPENSSL_H__
#define EMP_GROUP_OPENSSL_H__

namespace emp {
inline BigInt::BigInt() { n = BN_new(); }
inline BigInt::BigInt(const BigInt &oth) {
  n = BN_new();
  BN_copy(n, oth.n);
}
inline BigInt &BigInt::operator=(BigInt oth) {
  std::swap(n, oth.n);
  return *this;
}
inline BigInt::~BigInt() {
  if (n != nullptr)
    BN_free(n);
}

inline int BigInt::size() { return BN_num_bytes(n); }

inline void BigInt::to_bin(unsigned char *in) { BN_bn2bin(n, in); }

inline void BigInt::from_bin(const unsigned char *in, int length) {
  BN_free(n);
  n = BN_bin2bn(in, length, nullptr);
}

inline BigInt BigInt::add(const BigInt &oth) {
  BigInt ret;
  BN_add(ret.n, n, oth.n);
  return ret;
}

inline BigInt BigInt::mul_mod(const BigInt &b, const BigInt &m, BN_CTX *ctx) {
  BigInt ret;
  BN_mod_mul(ret.n, n, b.n, m.n, ctx);
  return ret;
}

inline BigInt BigInt::add_mod(const BigInt &b, const BigInt &m, BN_CTX *ctx) {
  BigInt ret;
  BN_mod_add(ret.n, n, b.n, m.n, ctx);
  return ret;
}

inline BigInt BigInt::mul(const BigInt &oth, BN_CTX *ctx) {
  BigInt ret;
  BN_mul(ret.n, n, oth.n, ctx);
  return ret;
}

inline BigInt BigInt::mod(const BigInt &oth, BN_CTX *ctx) {
  BigInt ret;
  BN_mod(ret.n, n, oth.n, ctx);
  return ret;
}

inline Point::Point(Group *g) {
  if (g == nullptr)
    return;
  this->group = g;
  point = EC_POINT_new(group->ec_group);
}

inline Point::~Point() {
  if (point != nullptr)
    EC_POINT_free(point);
}

inline Point::Point(const Point &p) {
  if (p.group == nullptr)
    return;
  this->group = p.group;
  point = EC_POINT_new(group->ec_group);
  int ret = EC_POINT_copy(point, p.point);
  if (ret == 0)
    sci::error("ECC COPY");
}

inline Point &Point::operator=(Point p) {
  std::swap(p.point, point);
  std::swap(p.group, group);
  return *this;
}

inline void Point::to_bin(unsigned char *buf, size_t buf_len) {
  int ret =
      EC_POINT_point2oct(group->ec_group, point, POINT_CONVERSION_UNCOMPRESSED,
                         buf, buf_len, group->bn_ctx);
  if (ret == 0)
    sci::error("ECC TO_BIN");
}

inline size_t Point::size() {
  size_t ret =
      EC_POINT_point2oct(group->ec_group, point, POINT_CONVERSION_UNCOMPRESSED,
                         NULL, 0, group->bn_ctx);
  if (ret == 0)
    sci::error("ECC SIZE_BIN");
  return ret;
}

inline void Point::from_bin(Group *g, const unsigned char *buf,
                            size_t buf_len) {
  if (point == nullptr) {
    group = g;
    point = EC_POINT_new(group->ec_group);
  }
  int ret =
      EC_POINT_oct2point(group->ec_group, point, buf, buf_len, group->bn_ctx);
  if (ret == 0)
    sci::error("ECC FROM_BIN");
}

inline Point Point::add(Point &rhs) {
  Point ret(group);
  int res =
      EC_POINT_add(group->ec_group, ret.point, point, rhs.point, group->bn_ctx);
  if (res == 0)
    sci::error("ECC ADD");
  return ret;
}

inline Point Point::mul(const BigInt &m) {
  Point ret(group);
  int res =
      EC_POINT_mul(group->ec_group, ret.point, NULL, point, m.n, group->bn_ctx);
  if (res == 0)
    sci::error("ECC MUL");
  return ret;
}

inline Point Point::inv() {
  Point ret(*this);
  int res = EC_POINT_invert(group->ec_group, ret.point, group->bn_ctx);
  if (res == 0)
    sci::error("ECC INV");
  return ret;
}
inline bool Point::operator==(Point &rhs) {
  int ret = EC_POINT_cmp(group->ec_group, point, rhs.point, group->bn_ctx);
  if (ret == -1)
    sci::error("ECC CMP");
  return (ret == 0);
}

inline Group::Group() {
  ec_group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1); // NIST P-256
  bn_ctx = BN_CTX_new();
  EC_GROUP_precompute_mult(ec_group, bn_ctx);
  EC_GROUP_get_order(ec_group, order.n, bn_ctx);
  scratch = new unsigned char[scratch_size];
}

inline Group::~Group() {
  if (ec_group != nullptr)
    EC_GROUP_free(ec_group);

  if (bn_ctx != nullptr)
    BN_CTX_free(bn_ctx);

  if (scratch != nullptr)
    delete[] scratch;
}

inline void Group::resize_scratch(size_t size) {
  if (size > scratch_size) {
    delete[] scratch;
    scratch_size = size;
    scratch = new unsigned char[scratch_size];
  }
}

inline void Group::get_rand_bn(BigInt &n) { BN_rand_range(n.n, order.n); }

inline Point Group::get_generator() {
  Point res(this);
  int ret = EC_POINT_copy(res.point, EC_GROUP_get0_generator(ec_group));
  if (ret == 0)
    sci::error("ECC GEN");
  return res;
}

inline Point Group::mul_gen(const BigInt &m) {
  Point res(this);
  int ret = EC_POINT_mul(ec_group, res.point, m.n, NULL, NULL, bn_ctx);
  if (ret == 0)
    sci::error("ECC GEN MUL");
  return res;
}
} // namespace emp
#endif
