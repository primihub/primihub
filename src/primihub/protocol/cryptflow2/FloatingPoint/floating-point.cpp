/*
Authors: Deevashwer Rathee
Copyright:
Copyright (c) 2021 Microsoft Research
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

#include "FloatingPoint/floating-point.h"
#include <omp.h>

#define CHUNK_SIZE (1 << 20)

using namespace std;
using namespace sci;

FPArray FPArray::subset(int i, int j) {
  assert(i >= 0 && j <= size && i < j);
  int sz = j - i;
  FPArray ret(this->party, sz, this->m_bits, this->e_bits);
  memcpy(ret.s, this->s + i, sz * sizeof(uint8_t));
  memcpy(ret.z, this->z + i, sz * sizeof(uint8_t));
  memcpy(ret.m, this->m + i, sz * sizeof(uint64_t));
  memcpy(ret.e, this->e + i, sz * sizeof(uint64_t));

  return ret;
}

BoolArray concat(const vector<BoolArray>& x) {
  int N = x.size();
  int sz = x[0].size;
  int party = x[0].party;
  for (int i = 1; i < N; i++) {
    sz += x[i].size;
    assert(party == x[i].party);
  }
  BoolArray ret(party, sz);
  int offset = 0;
  for (int i = 0; i < N; i++) {
    int n = x[i].size;
    memcpy(ret.data + offset, x[i].data, n * sizeof(uint8_t));
    offset += n;
  }
  return ret;
}

FPArray concat(const vector<FPArray>& x) {
  int N = x.size();
  int sz = x[0].size;
  int m_bits = x[0].m_bits;
  int e_bits = x[0].e_bits;
  int party = x[0].party;
  for (int i = 1; i < N; i++) {
    sz += x[i].size;
    assert(m_bits == x[i].m_bits);
    assert(e_bits == x[i].e_bits);
    assert(party == x[i].party);
  }
  FPArray ret(party, sz, m_bits, e_bits);
  int offset = 0;
  for (int i = 0; i < N; i++) {
    int n = x[i].size;
    memcpy(ret.s + offset, x[i].s, n * sizeof(uint8_t));
    memcpy(ret.z + offset, x[i].z, n * sizeof(uint8_t));
    memcpy(ret.m + offset, x[i].m, n * sizeof(uint64_t));
    memcpy(ret.e + offset, x[i].e, n * sizeof(uint64_t));
    offset += n;
  }
  return ret;
}

uint64_t RoundTiesToEven(uint64_t x, int32_t shift_amt) {
  assert(shift_amt > 0);
  bool a = (x >> (shift_amt - 1)) & 1;
  bool b = (x & ((1ULL << (shift_amt - 1)) - 1)) != 0;
  bool c = (x >> shift_amt) & 1;
  uint64_t y = x >> shift_amt;
  if (a && (b || c)) {
    y += 1;
  }
  return y;
}

template <class T> vector<T> FPArray::get_native_type() {
  assert(this->party == PUBLIC);
  vector<T> ret(this->size);
  uint8_t T_m_bits, T_e_bits;
  if constexpr (is_same_v<T, float>) {
    T_m_bits = 23;
    T_e_bits = 8;
  }
  if constexpr (is_same_v<T, double>) {
    T_m_bits = 52;
    T_e_bits = 11;
  }
  uint64_t T_m_mask = (1ULL << T_m_bits) - 1;
  uint64_t T_e_mask = (uint64_t(1) << T_e_bits) - 1;
  uint64_t T_e_bias = (uint64_t(1) << (T_e_bits - 1)) - 1;
  uint64_t *m_ = new uint64_t[this->size];
  uint64_t *e_ = new uint64_t[this->size];
  uint64_t m_mask_ = (this->m_mask() >> 1);
  uint64_t e_bias_ = this->e_bias();
  for (int i = 0; i < this->size; i++) {
    uint64_t ret_int = 0;
    uint64_t m_ = this->m[i];
    int64_t e_ = this->e[i] - e_bias_;
    if (m_bits <= T_m_bits) {
      m_ <<= (T_m_bits - m_bits);
    } else {
      // m_ >>= (m_bits - T_m_bits);
      m_ = RoundTiesToEven(m_, m_bits - T_m_bits);
    }
    if (e_ == (e_bias_ + 1)) {
      e_ = T_e_bias + 1;
    }
    if (e_ == (-1 * int64_t(e_bias_))) {
      e_ = -1 * T_e_bias;
      m_ = 0;
    }
    if (m_ == (1ULL << (T_m_bits + 1))) {
      e_ = e_ + 1;
      m_ >>= 1;
    }
    m_ &= T_m_mask;
    if (e_ > int64_t(T_e_bias)) {
      e_ = T_e_bias + 1;
      m_ = 0;
    }
    if ((z[i] & 1) || e_ < 1 - int64_t(T_e_bias)) {
      e_ = -1 * T_e_bias;
      m_ = 0;
    }
    e_ = e_ + T_e_bias;
    assert(e_ >= 0);
    ret_int |= m_;
    ret_int |= (uint64_t(e_) << T_m_bits);
    if constexpr (is_same_v<T, float>) {
      ret_int |= (uint64_t(this->s[i] & 1) << 31);
      uint32_t tmp = ret_int;
      ret[i] = *((T *)&tmp);
    }
    if constexpr (is_same_v<T, double>) {
      ret_int |= (uint64_t(this->s[i] & 1) << 63);
      ret[i] = *((T *)&ret_int);
    }
  }
  return ret;
}

FPMatrix FPMatrix::transpose() {
  FPMatrix ret(this->party, dim2, dim1, m_bits, e_bits);
  for (int i = 0; i < dim2; i++) {
    for (int j = 0; j < dim1; j++) {
      ret.m[i*dim1 + j] = this->m[j*dim2 + i];
      ret.e[i*dim1 + j] = this->e[j*dim2 + i];
      ret.s[i*dim1 + j] = this->s[j*dim2 + i];
      ret.z[i*dim1 + j] = this->z[j*dim2 + i];
    }
  }
  return ret;
}

std::ostream &operator<<(std::ostream &os, FPArray &other) {
  assert(other.party == PUBLIC);
  uint64_t m_mask_ = (other.m_mask() >> 1);
  os << "[";
  auto dbl_other = other.get_native_type<double>();
  for (int i = 0; i < other.size; i++) {
    uint64_t m_ = other.m[i] & m_mask_;
    uint64_t e_ = other.e[i];
    std::string tmp_m = std::bitset<64>(m_).to_string();
    std::string tmp_e = std::bitset<16>(e_).to_string();
    os << dbl_other[i] << " s=" << uint16_t(other.s[i]) << " e=" << signed_val(e_, other.e_bits + 2) << " ("
       << tmp_e.substr(16 - other.e_bits, 16) << ") m=" << m_ << " ("
       << tmp_m.substr(64 - other.m_bits, 64) << ")\t";
  }
  os << "]";

  return os;
}

std::ostream &operator<<(std::ostream &os, FPMatrix &other) {
  assert(other.party == PUBLIC);
  uint64_t m_mask_ = (other.m_mask() >> 1);
  os << "[";
  auto dbl_other = other.get_native_type<double>();
  for (int i = 0; i < other.dim1; i++) {
    os << "[";
    for (int j = 0; j < other.dim2; j++) {
    uint64_t m_ = other.m[i*other.dim2+j] & m_mask_;
    uint64_t e_ = other.e[i*other.dim2+j];
    std::string tmp_m = std::bitset<64>(m_).to_string();
    std::string tmp_e = std::bitset<16>(e_).to_string();
    os << dbl_other[i*other.dim2+j] << " s=" << uint16_t(other.s[i*other.dim2+j]) << " e=" << signed_val(e_, other.e_bits + 2) << " ("
       << tmp_e.substr(16 - other.e_bits, 16) << ") m=" << m_ << " ("
       << tmp_m.substr(64 - other.m_bits, 64) << ")\t";
    }
    os << "]\n";
  }
  os << "]";

  return os;
}

template vector<float> FPArray::get_native_type();
template vector<double> FPArray::get_native_type();

FPArray FPOp::input(int party_, int sz, uint8_t* s_, uint8_t* z_, uint64_t* m_,
        uint64_t* e_, uint8_t m_bits_, uint8_t e_bits_) {
  FPArray ret((party_ == PUBLIC ? party_ : this->party), sz, m_bits_, e_bits_);
  uint64_t m_mask_ = ret.m_mask();
  uint64_t e_mask_ = ret.e_mask();
  if ((this->party == party_) || (party_ == PUBLIC)) {
    memcpy(ret.s, s_, sz * sizeof(uint8_t));
    memcpy(ret.z, z_, sz * sizeof(uint8_t));
    memcpy(ret.m, m_, sz * sizeof(uint64_t));
    memcpy(ret.e, e_, sz * sizeof(uint64_t));
    for (int i = 0; i < sz; i++) {
      ret.s[i] &= 1;
      ret.z[i] &= 1;
      ret.m[i] &= m_mask_;
      ret.e[i] &= e_mask_;
    }
  } else {
    memset(ret.s, 0, sz * sizeof(uint8_t));
    memset(ret.z, 0, sz * sizeof(uint8_t));
    for (int i = 0; i < sz; i++) {
      ret.m[i] = 0;
      ret.e[i] = 0;
    }
  }
  return ret;
}

template <class T>
FPArray FPOp::input(int party_, int sz, T* data_, uint8_t m_bits_,
        uint8_t e_bits_, bool check_params) {
  FPArray ret((party_ == PUBLIC ? party_ : this->party), sz, m_bits_, e_bits_);
  if ((this->party != party_) && (party_ != PUBLIC)) {
    memset(ret.s, 0, sz * sizeof(uint8_t));
    memset(ret.z, 0, sz * sizeof(uint8_t));
    for (int i = 0; i < sz; i++) {
      ret.m[i] = 0;
      ret.e[i] = 0;
    }
    return ret;
  }
  uint64_t *data_int = new uint64_t[sz];
  uint8_t T_m_bits, T_e_bits;
  if constexpr (is_same_v<T, float>) {
    for (int i = 0; i < sz; i++) {
      uint32_t tmp = ((uint32_t *)data_)[i];
      data_int[i] = tmp;
    }
    T_m_bits = 23;
    T_e_bits = 8;
    if (m_bits_ == 0) {
      m_bits_ = 23;
    }
    if (e_bits_ == 0) {
      e_bits_ = 8;
    }
  }
  if constexpr (is_same_v<T, double>) {
    for (int i = 0; i < sz; i++) {
      data_int[i] = ((uint64_t *)data_)[i];
    }
    T_m_bits = 52;
    T_e_bits = 11;
    if (m_bits_ == 0) {
      m_bits_ = 52;
    }
    if (e_bits_ == 0) {
      e_bits_ = 11;
    }
  }

  uint64_t T_m_mask = (1ULL << T_m_bits) - 1;
  uint64_t T_e_mask = (uint64_t(1) << T_e_bits) - 1;
  uint64_t T_e_bias = (uint64_t(1) << (T_e_bits - 1)) - 1;
  uint64_t e_mask = (uint64_t(1) << e_bits_) - 1;

  for (int i = 0; i < sz; i++) {
    ret.z[i] = (data_int[i] == 0 || data_int[i] == (1ULL << (sizeof(T) * 8 - 1)));
    ret.s[i] = data_int[i] >= (1ULL << (sizeof(T) * 8 - 1));
    ret.m[i] = (data_int[i] & T_m_mask) | (ret.z[i] ? 0 : 1ULL << T_m_bits);
    if (m_bits_ >= T_m_bits) {
      ret.m[i] <<= (m_bits_ - T_m_bits);
    } else {
      ret.m[i] = RoundTiesToEven(ret.m[i], T_m_bits - m_bits_);
    }
    ret.e[i] = ((data_int[i] >> T_m_bits) & T_e_mask) - T_e_bias;
    if (ret.e[i] == (T_e_bias + 1)) {
      ret.e[i] = ret.e_bias() + 1;
    }
    ret.e[i] += ret.e_bias();
    if (ret.m[i] == (1ULL << (m_bits_ + 1))) {
      ret.m[i] >>= 1;
      ret.e[i] += 1;
    }
  }
  if (check_params) ret.check_bounds();
  delete[] data_int;

  return ret;
}

template <class T>
FPArray FPOp::input(int party_, int sz, T data_, uint8_t m_bits_,
        uint8_t e_bits_, bool check_params) {
  vector<T> data_vec(sz);
  for (int i = 0; i < sz; i++) {
    data_vec[i] = data_;
  }
  FPArray ret = this->input<T>(party_, sz, data_vec.data(), m_bits_, e_bits_, check_params);
  return ret;
}

FPArray FPOp::output(int party_, const FPArray& x) {
  if (x.party == PUBLIC) {
    return x;
  }
  int sz = x.size;
  int ret_party = (party_ == PUBLIC || party_ == x.party ? PUBLIC : x.party);
  FPArray ret(ret_party, sz, x.m_bits, x.e_bits);
#pragma omp parallel num_threads(2)
  {
    if (omp_get_thread_num() == 1 && party_ != BOB) {
      if (party == sci::ALICE) {
        iopack->io_rev->recv_data(ret.s, sz * sizeof(uint8_t));
        iopack->io_rev->recv_data(ret.z, sz * sizeof(uint8_t));
        iopack->io_rev->recv_data(ret.m, sz * sizeof(uint64_t));
        iopack->io_rev->recv_data(ret.e, sz * sizeof(uint64_t));
      } else { // party == sci::BOB
        iopack->io_rev->send_data(x.s, sz * sizeof(uint8_t));
        iopack->io_rev->send_data(x.z, sz * sizeof(uint8_t));
        iopack->io_rev->send_data(x.m, sz * sizeof(uint64_t));
        iopack->io_rev->send_data(x.e, sz * sizeof(uint64_t));
      }
    } else if (omp_get_thread_num() == 0 && party_ != ALICE) {
      if (party == sci::ALICE) {
        iopack->io->send_data(x.s, sz * sizeof(uint8_t));
        iopack->io->send_data(x.z, sz * sizeof(uint8_t));
        iopack->io->send_data(x.m, sz * sizeof(uint64_t));
        iopack->io->send_data(x.e, sz * sizeof(uint64_t));
      } else { // party == sci::BOB
        iopack->io->recv_data(ret.s, sz * sizeof(uint8_t));
        iopack->io->recv_data(ret.z, sz * sizeof(uint8_t));
        iopack->io->recv_data(ret.m, sz * sizeof(uint64_t));
        iopack->io->recv_data(ret.e, sz * sizeof(uint64_t));
      }
    }
  }
  uint64_t m_mask_ = x.m_mask();
  uint64_t e_mask_ = x.e_mask();
  for (int i = 0; i < sz; i++) {
    ret.s[i] = ret.s[i] ^ x.s[i];
    ret.z[i] = ret.z[i] ^ x.z[i];
    ret.m[i] = (ret.m[i] + x.m[i]) & m_mask_;
    ret.e[i] = (ret.e[i] + x.e[i]) & e_mask_;
  }
  return ret;
}

// To avoid linkage failure
template FPArray FPOp::input(int party_, int sz, float* data_,
        uint8_t m_bits_, uint8_t e_bits_, bool check_params);
template FPArray FPOp::input(int party_, int sz, float data_,
        uint8_t m_bits_, uint8_t e_bits_, bool check_params);
template FPArray FPOp::input(int party_, int sz, double* data_,
        uint8_t m_bits_, uint8_t e_bits_, bool check_params);
template FPArray FPOp::input(int party_, int sz, double data_,
        uint8_t m_bits_, uint8_t e_bits_, bool check_params);

tuple<BoolArray,BoolArray,FixArray,FixArray> FPOp::get_components(const FPArray &x) {
  BoolArray x_s = bool_op->input(x.party, x.size, x.s);
  BoolArray x_z = bool_op->input(x.party, x.size, x.z);
  FixArray x_m = fix->input(x.party, x.size, x.m, false, x.m_bits + 1, x.m_bits);
  FixArray x_e = fix->input(x.party, x.size, x.e, true, x.e_bits + 2, 0);
  return make_tuple(x_s, x_z, x_m, x_e);
}

FPArray FPOp::check_bounds(const FPArray &x) {
  assert(x.party != PUBLIC);
  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);

  BoolArray overflow_check = fix->GT(x_e, x.e_max());
  BoolArray underflow_check = fix->LT(x_e, x.e_min());
  underflow_check = bool_op->OR(underflow_check, x_z);

  uint64_t m_inf = (1ULL << x.m_bits);
  uint64_t e_inf = x.e_max() + 1;
  FixArray ret_m_if = fix->input(PUBLIC, x.size, m_inf, x_m.signed_, x_m.ell, x_m.s);
  FixArray ret_m_else = x_m;
  FixArray ret_e_if = fix->input(PUBLIC, x.size, e_inf, x_e.signed_, x_e.ell, x_e.s);
  FixArray ret_e_else = x_e;

  FixArray ret_m = fix->if_else(overflow_check, ret_m_if, ret_m_else);
  FixArray ret_e = fix->if_else(overflow_check, ret_e_if, ret_e_else);

  uint64_t m_0 = 0;
  uint64_t e_0 = x.e_min() - 1;
  ret_m_if = fix->input(PUBLIC, x.size, m_0, x_m.signed_, x_m.ell, x_m.s);
  ret_m_else = ret_m;
  ret_e_if = fix->input(PUBLIC, x.size, e_0, x_e.signed_, x_e.ell, x_e.s);
  ret_e_else = ret_e;

  ret_m = fix->if_else(underflow_check, ret_m_if, ret_m_else);
  ret_e = fix->if_else(underflow_check, ret_e_if, ret_e_else);
  BoolArray ret_z = underflow_check;

  FPArray ret = this->input(x.party, x.size, x_s.data, ret_z.data,
          ret_m.data, ret_e.data, x.m_bits, x.e_bits);

  return ret;
}

BoolArray FPOp::LT(const FPArray &x, const FPArray &y, bool equal_sign) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.m_bits == y.m_bits);
  assert(x.e_bits == y.e_bits);

  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);
  BoolArray y_s, y_z;
  FixArray y_m, y_e;
  tie(y_s, y_z, y_m, y_e) = get_components(y);

  BoolArray all_1 = bool_op->input(ALICE, x.size, 1);

  FixArray x_em = fix->extend(x_m, x_m.ell + x_e.ell - 1, all_1.data);
  x_e.signed_ = false;
  x_em = fix->add(x_em, fix->scale_up(x_e, x_m.ell + x_e.ell - 1, x.m_bits));

  FixArray y_em = fix->extend(y_m, y_m.ell + y_e.ell - 1, all_1.data);
  y_e.signed_ = false;
  y_em = fix->add(y_em, fix->scale_up(y_e, y_m.ell + y_e.ell - 1, y.m_bits));

  BoolArray em_lt, em_eq;
  tie(em_lt, em_eq) = fix->LT_and_EQ(x_em, y_em);

  BoolArray cond1 = bool_op->XOR(x_s, em_lt);
  BoolArray cond2 = bool_op->NOT(em_eq);
  BoolArray ret_if = bool_op->AND(cond1, cond2);

  if (equal_sign)
    return ret_if;

  BoolArray cond3 = bool_op->NOT(bool_op->AND(x_z, y_z));
  BoolArray ret_else = bool_op->AND(x_s, cond3);

  // s_eq = 1 ^ x_s ^ y_s
  BoolArray s_eq = bool_op->XOR(x_s, bool_op->NOT(y_s));

  BoolArray ret = bool_op->if_else(s_eq, ret_if, ret_else);

  return ret;
}

BoolArray FPOp::GT(const FPArray &x, const FPArray &y, bool equal_sign) {
  return this->LT(y, x, equal_sign);
}

BoolArray FPOp::LE(const FPArray &x, const FPArray &y, bool equal_sign) {
  return bool_op->NOT(this->LT(y, x, equal_sign));
}

BoolArray FPOp::GE(const FPArray &x, const FPArray &y, bool equal_sign) {
  return bool_op->NOT(this->LT(x, y, equal_sign));
}

template <class T> BoolArray FPOp::LT(const FPArray &x, T y, bool equal_sign) {
  assert(x.party != PUBLIC);
  FPArray fp_y = this->input<T>(PUBLIC, x.size, y, x.m_bits, x.e_bits);
  return this->LT(x, fp_y, equal_sign);
}

template <class T> BoolArray FPOp::GT(const FPArray &x, T y, bool equal_sign) {
  assert(x.party != PUBLIC);
  FPArray fp_y = this->input<T>(PUBLIC, x.size, y, x.m_bits, x.e_bits);
  return this->GT(x, fp_y, equal_sign);
}

template <class T> BoolArray FPOp::LE(const FPArray &x, T y, bool equal_sign) {
  assert(x.party != PUBLIC);
  FPArray fp_y = this->input<T>(PUBLIC, x.size, y, x.m_bits, x.e_bits);
  return this->LE(x, fp_y, equal_sign);
}

template <class T> BoolArray FPOp::GE(const FPArray &x, T y, bool equal_sign) {
  assert(x.party != PUBLIC);
  FPArray fp_y = this->input<T>(PUBLIC, x.size, y, x.m_bits, x.e_bits);
  return this->GE(x, fp_y, equal_sign);
}

// To avoid linkage failure
template BoolArray FPOp::LT(const FPArray &x, float y, bool equal_sign);
template BoolArray FPOp::GT(const FPArray &x, float y, bool equal_sign);
template BoolArray FPOp::LE(const FPArray &x, float y, bool equal_sign);
template BoolArray FPOp::GE(const FPArray &x, float y, bool equal_sign);
template BoolArray FPOp::LT(const FPArray &x, double y, bool equal_sign);
template BoolArray FPOp::GT(const FPArray &x, double y, bool equal_sign);
template BoolArray FPOp::LE(const FPArray &x, double y, bool equal_sign);
template BoolArray FPOp::GE(const FPArray &x, double y, bool equal_sign);

FPArray FPOp::if_else(const BoolArray &cond, const FPArray &x,
                      const FPArray &y) {
  assert(cond.party != PUBLIC);
  assert(x.party != PUBLIC && y.party != PUBLIC);
  assert(cond.size == x.size && cond.size == y.size);
  assert(x.m_bits == y.m_bits);
  assert(x.e_bits == y.e_bits);
  int ret_ell = x.m_bits + x.e_bits + 5;
  assert(ret_ell <= 64);

  uint64_t m_mask = x.m_mask();
  uint64_t e_mask = x.e_mask();
  uint64_t ret_mask = (1ULL << ret_ell) - 1;
  uint64_t **send_data = new uint64_t *[x.size];
  uint64_t *r = new uint64_t[x.size];
  uint64_t *recv_data = new uint64_t[x.size];
  PRG128 prg;
  prg.random_data(r, x.size * sizeof(uint64_t));
  for (int i = 0; i < x.size; i++) {
    r[i] &= ret_mask;
    uint64_t r_data = r[i];
    uint64_t ret_x = (x.m[i] - r_data) & m_mask;
    r_data >>= (x.m_bits + 1);
    ret_x |= ((x.e[i] - r_data) & e_mask) << (x.m_bits + 1);
    r_data >>= (x.e_bits + 2);
    ret_x |= ((x.s[i] - r_data) & 1) << (x.m_bits + x.e_bits + 3);
    r_data >>= 1;
    ret_x |= ((x.z[i] - r_data) & 1) << (x.m_bits + x.e_bits + 4);

    r_data = r[i];
    uint64_t ret_y = (y.m[i] - r_data) & m_mask;
    r_data >>= (x.m_bits + 1);
    ret_y |= ((y.e[i] - r_data) & e_mask) << (x.m_bits + 1);
    r_data >>= (x.e_bits + 2);
    ret_y |= ((y.s[i] - r_data) & 1) << (x.m_bits + x.e_bits + 3);
    r_data >>= 1;
    ret_y |= ((y.z[i] - r_data) & 1) << (x.m_bits + x.e_bits + 4);
    send_data[i] = new uint64_t[2];
    if (cond.data[i]) {
      send_data[i][0] = ret_x;
      send_data[i][1] = ret_y;
    } else {
      send_data[i][0] = ret_y;
      send_data[i][1] = ret_x;
    }
  }
#pragma omp parallel num_threads(2)
  {
    if (omp_get_thread_num() == 1) {
      if (party == sci::ALICE) {
        otpack->iknp_reversed->recv_impl(recv_data, cond.data, x.size, ret_ell);
      } else { // party == sci::BOB
        otpack->iknp_reversed->send_impl(send_data, x.size, ret_ell);
      }
    } else {
      if (party == sci::ALICE) {
        otpack->iknp_straight->send_impl(send_data, x.size, ret_ell);
      } else { // party == sci::BOB
        otpack->iknp_straight->recv_impl(recv_data, cond.data, x.size, ret_ell);
      }
    }
  }
  FPArray ret(this->party, x.size, x.m_bits, x.e_bits);
  for (int i = 0; i < x.size; i++) {
    uint64_t r_data = r[i];
    uint64_t rcv_data = recv_data[i];
    ret.m[i] = (r_data + rcv_data) & m_mask;
    r_data >>= (x.m_bits + 1);
    rcv_data >>= (x.m_bits + 1);
    ret.e[i] = (r_data + rcv_data) & e_mask;
    r_data >>= (x.e_bits + 2);
    rcv_data >>= (x.e_bits + 2);
    ret.s[i] = (r_data + rcv_data) & 1;
    r_data >>= 1;
    rcv_data >>= 1;
    ret.z[i] = (r_data + rcv_data) & 1;
  }

  delete[] r;
  delete[] recv_data;
  for (int i = 0; i < x.size; i++) {
    delete[] send_data[i];
  }
  delete[] send_data;

  return ret;
}

FPArray FPOp::if_else(const BoolArray &cond, const FPArray &x, float y) {
  assert(cond.party != PUBLIC && x.party != PUBLIC);
  assert(cond.size == x.size);
  // Setting input party to ALICE so that fp_y is interpreted as a secret-shared value
  // This is required as the core if_else function doesn't support PUBLIC FPArrays
  FPArray fp_y = this->input<float>(ALICE, x.size, y, x.m_bits, x.e_bits, false);
  return this->if_else(cond, x, fp_y);
}

FPArray FPOp::if_else(const BoolArray &cond, const FPArray &x, double y) {
  assert(cond.party != PUBLIC && x.party != PUBLIC);
  assert(cond.size == x.size);
  // Setting input party to ALICE so that fp_y is interpreted as a secret-shared value
  // This is required as the core if_else function doesn't support PUBLIC FPArrays
  FPArray fp_y = this->input<double>(ALICE, x.size, y, x.m_bits, x.e_bits, false);
  return this->if_else(cond, x, fp_y);
}

FPArray FPOp::LUT(const std::vector<uint64_t> &spec_vec, const FixArray &x,
                  uint8_t m_bits, uint8_t e_bits) {
  assert(spec_vec.size() <= (1 << x.ell));
  assert(x.party != PUBLIC);
  assert(x.ell <= 8);
  vector<uint64_t> spec_pow2(1 << x.ell, 0);
  memcpy(spec_pow2.data(), spec_vec.data(), sizeof(uint64_t) * spec_vec.size());
  uint64_t x_mask = x.ell_mask();
  int ret_ell = (m_bits + 1 + e_bits + 2 + 2);
  uint64_t ret_mask = (1ULL << ret_ell) - 1;
  uint64_t m_mask = (1ULL << (m_bits + 1)) - 1;
  uint64_t e_mask = (1ULL << (e_bits + 2)) - 1;
  uint64_t *ret_int64 = new uint64_t[x.size];
  if (party == ALICE) {
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(ret_int64, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[spec_pow2.size()];
      ret_int64[i] &= ret_mask;
      for (int j = 0; j < spec_pow2.size(); j++) {
        int idx = (x.data[i] + j) & x_mask;
        uint64_t spec_data = spec_pow2[idx];
        uint64_t ret_data = ret_int64[i];
        uint64_t spec_data_m = spec_data & m_mask;
        uint64_t ret_data_m = ret_data & m_mask;
        spec_data >>= (m_bits + 1);
        ret_data >>= (m_bits + 1);
        uint64_t spec_data_e = spec_data & e_mask;
        uint64_t ret_data_e = ret_data & e_mask;
        spec_data >>= (e_bits + 2);
        ret_data >>= (e_bits + 2);
        uint64_t spec_data_s = spec_data & 1;
        uint64_t ret_data_s = ret_data & 1;
        spec_data >>= 1;
        ret_data >>= 1;
        uint64_t spec_data_z = spec_data & 1;
        uint64_t ret_data_z = ret_data & 1;
        spec[i][j] =
            (((spec_data_z + ret_data_z) & 1) << (m_bits + e_bits + 4)) |
            (((spec_data_s + ret_data_s) & 1) << (m_bits + e_bits + 3)) |
            (((spec_data_e - ret_data_e) & e_mask) << (m_bits + 1)) |
            ((spec_data_m - ret_data_m) & m_mask);
      }
    }
    fix->aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, x.ell,
                                     ret_ell);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    fix->aux->lookup_table<uint64_t>(nullptr, x.data, ret_int64, x.size, x.ell,
                                     ret_ell);
  }
  uint8_t *ret_s = new uint8_t[x.size];
  uint8_t *ret_z = new uint8_t[x.size];
  uint64_t *ret_e = new uint64_t[x.size];
  uint64_t *ret_m = new uint64_t[x.size];
  for (int i = 0; i < x.size; i++) {
    uint64_t ret_data = ret_int64[i];
    ret_m[i] = ret_data & m_mask;
    ret_data >>= (m_bits + 1);
    ret_e[i] = ret_data & e_mask;
    ret_data >>= (e_bits + 2);
    ret_s[i] = ret_data & 1;
    ret_data >>= 1;
    ret_z[i] = ret_data & 1;
  }

  FPArray ret = this->input(this->party, x.size, ret_s, ret_z, ret_m, ret_e, m_bits, e_bits);
  delete[] ret_s;
  delete[] ret_z;
  delete[] ret_m;
  delete[] ret_e;
  delete[] ret_int64;

  return ret;
}

vector<FPArray> FPOp::GetCoeffs(const vector<vector<uint64_t>> &spec_coeff,
                                const vector<uint64_t> &knots_bits,
                                const FixArray &x, int n, uint8_t m_bits,
                                uint8_t e_bits) {
  assert(x.party != PUBLIC);
  assert(x.ell <= 8);
  assert(n <= (1 << x.ell));
  assert((knots_bits.size() == n - 1) || (knots_bits.size() == n) || (knots_bits.size() == n + 1));
  for (int i = 0; i < spec_coeff.size(); i++) {
    assert(spec_coeff[i].size() == n);
  }
  if (n > 64) {
    int d = spec_coeff.size();
    int num_iters = ceil(n/double(64));
    vector<vector<uint64_t>> lspec_coeff(d);
    vector<uint64_t> lknots_bits;
    vector<vector<FPArray>> lret(num_iters);
    vector<FPArray> ret(d);
    for (int i = 0; i < n; i += 64) {
      int ln = std::min(64, n-i);
      for (int j = 0; j < d; j++) {
        lspec_coeff[j].resize(ln);
        for (int k = 0; k < ln; k++) {
          lspec_coeff[j][k] = spec_coeff[j][i+k];
        }
      }
      int lln = (i/64 == (num_iters - 1))? ln: ln + 1;
      lknots_bits.resize(lln);
      if (i) {
        lknots_bits[0] = knots_bits[i-1];
      } else {
        lknots_bits[0] = 0;
      }
      for (int k = 0; k < ln && i+k < n; k++) {
        lknots_bits[k+1] = knots_bits[i+k];
      }
      lret[i/64] = this->GetCoeffs(lspec_coeff, lknots_bits, x, ln, m_bits, e_bits);
    }
    uint64_t m_mask = (1ULL << (m_bits + 1)) - 1;
    uint64_t e_mask = (1ULL << (e_bits + 2)) - 1;
    for (int k = 0; k < d; k++) {
      ret[k] = lret[0][k];
      for (int i = 1; i < num_iters; i++) {
        for (int j = 0; j < x.size; j++) {
          ret[k].s[j] ^= lret[i][k].s[j];
          ret[k].z[j] ^= lret[i][k].z[j];
          ret[k].m[j] = (ret[k].m[j] + lret[i][k].m[j]) & m_mask;
          ret[k].e[j] = (ret[k].e[j] + lret[i][k].e[j]) & e_mask;
        }
      }
    }
    return ret;
  }
  assert(n <= 64);
  uint64_t x_mask = x.ell_mask();
  uint64_t m_mask = (1ULL << (m_bits + 1)) - 1;
  uint64_t e_mask = (1ULL << (e_bits + 2)) - 1;
  uint64_t LUT_out_mask = (1ULL << n) - 1;
  uint64_t *LUT_out = new uint64_t[x.size];
  if (party == ALICE) {
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(LUT_out, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[1 << x.ell];
      LUT_out[i] &= LUT_out_mask;
      for (int j = 0; j < (1 << x.ell); j++) {
        int idx = (x.data[i] + j) & x_mask;
        vector<uint8_t> spec_active_interval(n, 0);
        vector<uint8_t> u(n, 0);
        // n-1 knots specified
        if (knots_bits.size() == n-1) {
          spec_active_interval[0] = u[0] = (idx < knots_bits[0]);
          for (int k = 1; k < n - 1; k++) {
            u[k] = (idx < knots_bits[k]);
            spec_active_interval[k] = u[k] ^ u[k - 1];
          }
          spec_active_interval[n - 1] = u[n - 2] ^ 1;
        // n knots specified (lb specified)
        } else if (knots_bits.size() == n) {
          u[0] = (idx < knots_bits[1]);
          spec_active_interval[0] = u[0] && (idx >= knots_bits[0]);
          for (int k = 1; k < n - 1; k++) {
            u[k] = (idx < knots_bits[k+1]);
            spec_active_interval[k] = u[k] ^ u[k - 1];
          }
          spec_active_interval[n - 1] = u[n - 2] ^ 1;
        // n+1 knots specified (both lb and ub specified)
        } else if (knots_bits.size() == n+1) {
          u[0] = (idx < knots_bits[1]);
          spec_active_interval[0] = u[0] && (idx >= knots_bits[0]);
          for (int k = 1; k < n; k++) {
            u[k] = (idx < knots_bits[k+1]);
            spec_active_interval[k] = u[k] ^ u[k - 1];
          }
        }
        uint64_t spec_data = 0;
        uint64_t LUT_out_data = LUT_out[i];
        for (int k = 0; k < n; k++) {
          spec_data |= (((spec_active_interval[k] ^ LUT_out_data) & 1) << k);
          LUT_out_data >>= 1;
        }
        spec[i][j] = spec_data;
      }
    }
    fix->aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, x.ell, n);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    fix->aux->lookup_table<uint64_t>(nullptr, x.data, LUT_out, x.size, x.ell,
                                     n);
  }
  uint8_t *v = new uint8_t[x.size * n];
  uint64_t *V = new uint64_t[x.size * n];
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < x.size; j++) {
      v[i * x.size + j] = LUT_out[j] & 1;
      LUT_out[j] >>= 1;
    }
  }
  fix->aux->B2A(v, V, x.size * n, m_bits + 1);

  uint8_t *s = new uint8_t[x.size];
  uint8_t *z = new uint8_t[x.size];
  uint64_t *m = new uint64_t[x.size];
  uint64_t *e = new uint64_t[x.size];
  int d = spec_coeff.size();
  uint8_t theta_s[d][n];
  uint8_t theta_z[d][n];
  uint64_t theta_m[d][n];
  uint64_t theta_e[d][n];
  vector<FPArray> ret(d);
  for (int h = 0; h < d; h++) {
    for (int k = 0; k < n; k++) {
      uint64_t coeff = spec_coeff[h][k];
      theta_m[h][k] = coeff & m_mask;
      coeff >>= (m_bits + 1);
      theta_e[h][k] = coeff & e_mask;
      coeff >>= (e_bits + 2);
      theta_s[h][k] = coeff & 1;
      coeff >>= 1;
      theta_z[h][k] = coeff & 1;
    }
    for (int i = 0; i < x.size; i++) {
      s[i] = 0;
      z[i] = 0;
      m[i] = 0;
      e[i] = 0;
      for (int k = 0; k < n; k++) {
        s[i] ^= (theta_s[h][k] & v[k * x.size + i]);
        z[i] ^= (theta_z[h][k] & v[k * x.size + i]);
        m[i] += (theta_m[h][k] * V[k * x.size + i]);
        e[i] += (theta_e[h][k] * V[k * x.size + i]);
      }
      m[i] &= m_mask;
      e[i] &= e_mask;
    }
    ret[h] = this->input(this->party, x.size, s, z, m, e, m_bits, e_bits);
  }

  delete[] s;
  delete[] z;
  delete[] m;
  delete[] e;
  delete[] v;
  delete[] V;
  delete[] LUT_out;

  return ret;
}

FPArray FPOp::mulpow2(const FPArray &x, const FixArray &exp,
                      bool check_bounds) {
  assert(x.party != PUBLIC);
  assert(x.size == exp.size);
  assert(exp.ell <= x.e_bits + 2);
  assert(exp.signed_ == true);

  FixArray exp_ext = exp;
  if (exp_ext.ell < x.e_bits + 2) {
    fix->extend(exp_ext, x.e_bits + 2, nullptr);
  }
  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);

  FixArray ret_e = fix->add(x_e, exp_ext);

  FPArray ret = this->input(x.party, x.size, x_s.data, x_z.data, x_m.data,
          ret_e.data, x.m_bits, x.e_bits);

  if (check_bounds) {
    ret = this->check_bounds(ret);
  }

  return ret;
}

FPArray FPOp::mulpow2(const FPArray &x, int exp, bool check_bounds) {
  assert(x.party != PUBLIC);
  FixArray fp_exp = fix->input(PUBLIC, x.size, int64_t(exp), true, x.e_bits + 2, 0);
  return this->mulpow2(x, fp_exp, check_bounds);
}

FPArray FPOp::mul(const FPArray &x, const FPArray &y, bool check_bounds) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.m_bits == y.m_bits);
  assert(x.e_bits == y.e_bits);

  int sz = x.size;

  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);
  BoolArray y_s, y_z;
  FixArray y_m, y_e;
  tie(y_s, y_z, y_m, y_e) = get_components(y);

  BoolArray msb_x_m = bool_op->NOT(x_z);
  BoolArray msb_y_m = bool_op->NOT(y_z);

  /* FixArray ret_m = fix->mul(x_m, y_m, 2 * x.m_bits + 2, all_1.data, all_1.data); */
  FixArray ret_m = fix->mul(x_m, y_m, 2 * x.m_bits + 2, msb_x_m.data, msb_y_m.data);
  ret_m = fix->truncate_with_sticky_bit(ret_m, x.m_bits - 2);

  FixArray ret_e = fix->add(x_e, y_e);
  ret_e = fix->sub(ret_e, x.e_bias());
  BoolArray denormal_m = fix->GE(ret_m, (1ULL << (x.m_bits + 3)) - 2);

  FixArray ret_m_if = fix->round_ties_to_even(ret_m, 3);
  ret_m_if.s += 1;
  FixArray ret_m_else = fix->round_ties_to_even(ret_m, 2);
  ret_m_else = fix->reduce(ret_m_else, x.m_bits + 1);
  FixArray ret_e_if = fix->add(ret_e, 1);
  FixArray ret_e_else = ret_e;

  ret_m = fix->if_else(denormal_m, ret_m_if, ret_m_else);
  ret_e = fix->if_else(denormal_m, ret_e_if, ret_e_else);

  BoolArray ret_s = bool_op->XOR(x_s, y_s);
  BoolArray ret_z = bool_op->OR(x_z, y_z);

  FPArray ret = this->input(this->party, sz, ret_s.data, ret_z.data,
          ret_m.data, ret_e.data, x.m_bits, x.e_bits);

  if (check_bounds) {
    ret = this->check_bounds(ret);
  }

  return ret;
}

vector<FPArray> FPOp::mul(const vector<FPArray> &x, const vector<FPArray> &y) {
  FPArray concat_x = concat(x) ;
  FPArray concat_y = concat(y) ;
  FPArray concat_mul = this->mul(concat_x, concat_y) ;

  int N = x.size() ;
  int m_bits = x[0].m_bits ;
  int e_bits = x[0].e_bits ; 
  vector<FPArray> ret ;
  int offset = 0 ;

  for (int i = 0 ; i < N ; i++) {
    int n = x[i].size ;
    FPArray retvec(party, n, m_bits, e_bits) ;
    
    memcpy(retvec.s, concat_mul.s + offset, n*sizeof(uint8_t)) ;
    memcpy(retvec.z, concat_mul.z + offset, n*sizeof(uint8_t)) ;
    memcpy(retvec.m, concat_mul.m + offset, n*sizeof(uint64_t)) ;
    memcpy(retvec.e, concat_mul.e + offset, n*sizeof(uint64_t)) ;

    offset += n ;
    ret.push_back(retvec) ;
  }

  return ret ;
}

void FPOp::normalize(FixArray &m, FixArray &e, int e_offset, int ell) {
  assert(m.party != PUBLIC);
  assert(m.size == e.size);
  assert(ell <= m.ell);
  uint8_t *m_one_hot = new uint8_t[m.size * m.ell];
  uint64_t *m_one_hot_64 = new uint64_t[m.size * m.ell];
#ifdef MSNZB_GC
  fix->aux->msnzb_GC(m.data, m_one_hot, m.ell, m.size);
#else  // MSNZB_GC
  fix->aux->msnzb_one_hot(m.data, m_one_hot, m.ell, m.size);
#endif // MSNZB_GC
  int l = (m.ell > e.ell ? m.ell : e.ell);
  fix->aux->B2A(m_one_hot, m_one_hot_64, m.size * m.ell, l);
  FixArray k(m.party, m.size, m.signed_, m.ell, ell - 1 - m.s);
  FixArray e_adj(m.party, m.size, e.signed_, e.ell, e.s);
  uint64_t m_mask_ = m.ell_mask();
  uint64_t e_mask_ = e.ell_mask();
  for (int i = 0; i < m.size; i++) {
    k.data[i] = 0;
    e_adj.data[i] = 0;
    for (int j = 0; j < m.ell; j++) {
      k.data[i] += (1ULL << (ell - 1 - j)) * m_one_hot_64[i * m.ell + j];
      e_adj.data[i] += (j - e_offset) * m_one_hot_64[i * m.ell + j];
    }
    k.data[i] &= m_mask_;
    e_adj.data[i] &= e_mask_;
  }
  e = fix->add(e, e_adj);
  m = fix->mul(m, k, m.ell);

  delete[] m_one_hot;
  delete[] m_one_hot_64;

  return;
}

void FPOp::round_and_check(FixArray &m, FixArray &e, int s) {
  assert(m.party != PUBLIC && e.party != PUBLIC);
  assert(m.size == e.size);
  assert(m.s == m.ell - 1);
  int m_bits = m.ell - 1;
  uint64_t oflow_threshold = (1ULL << (m_bits + 1)) - (1ULL << (s - 1));
  BoolArray rnd_no_oflow = fix->LT(m, oflow_threshold);
  FixArray m_if = fix->round_ties_to_even(m, s);
  m = fix->if_else(rnd_no_oflow, m_if, (1ULL << (m_bits - s)));
  e = fix->if_else(rnd_no_oflow, e, fix->add(e, 1));

  return;
}

FPArray FPOp::flip_sign(const FPArray &x) {
  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);
  BoolArray not_x_s = bool_op->NOT(x_s);
  FPArray neg_x = this->input(x.party, x.size, not_x_s.data, x_z.data,
          x_m.data, x_e.data, x.m_bits, x.e_bits);
  return neg_x;
}

FPArray FPOp::add(const FPArray &x, const FPArray &y, bool cheap_variant,
                  bool compare_and_swap, bool check_bounds, int output_m_bits) {
  assert(x.party != PUBLIC && y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.m_bits == y.m_bits);
  assert(x.e_bits == y.e_bits);
  if (output_m_bits == -1) output_m_bits = x.m_bits;
  if (cheap_variant) assert(x.m_bits == output_m_bits);

  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);
  BoolArray y_s, y_z;
  FixArray y_m, y_e;
  tie(y_s, y_z, y_m, y_e) = get_components(y);

  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  BoolArray all_1 = bool_op->input(ALICE, x.size, 1);
  BoolArray msb_x_m = bool_op->NOT(x_z);
  BoolArray msb_y_m = bool_op->NOT(y_z);

  FPArray a = x;
  FPArray b = y;
  FixArray e_diff = fix->sub(x_e, y_e);
  if (compare_and_swap) {
    /* FixArray x_em = fix->extend(x_m, x_m.ell + x_e.ell - 1, all_1.data); */
    FixArray x_em = fix->extend(x_m, x_m.ell + x_e.ell - 1, msb_x_m.data);
    x_e.signed_ = false;
    x_em = fix->add(x_em, fix->scale_up(x_e, x_m.ell + x_e.ell - 1, x.m_bits));

    /* FixArray y_em = fix->extend(y_m, y_m.ell + y_e.ell - 1, all_1.data); */
    FixArray y_em = fix->extend(y_m, y_m.ell + y_e.ell - 1, msb_y_m.data);
    y_e.signed_ = false;
    y_em = fix->add(y_em, fix->scale_up(y_e, y_m.ell + y_e.ell - 1, y.m_bits));

    BoolArray em_gt = fix->GT(x_em, y_em);

    FixArray e_diff_if = e_diff;
    FixArray e_diff_else = fix->mul(e_diff, -1);

    a = this->if_else(em_gt, x, y);
    b = this->if_else(em_gt, y, x);
    e_diff = fix->if_else(em_gt, e_diff_if, e_diff_else);
  }
  e_diff.signed_ = false; // e_diff is unsigned now
  BoolArray a_s, a_z;
  FixArray a_m, a_e;
  tie(a_s, a_z, a_m, a_e) = get_components(a);
  BoolArray b_s, b_z;
  FixArray b_m, b_e;
  tie(b_s, b_z, b_m, b_e) = get_components(b);

  BoolArray msb_a_m = bool_op->NOT(a_z);
  BoolArray msb_b_m = bool_op->NOT(b_z);
  if (!cheap_variant) {
    a_m = fix->extend(a_m, 2 * x.m_bits + 3, msb_a_m.data);
    b_m = fix->extend(b_m, 2 * x.m_bits + 3, msb_b_m.data);
  } else {
    a_m = fix->extend(a_m, x.m_bits + 2, msb_a_m.data);
    b_m = fix->extend(b_m, x.m_bits + 2, msb_b_m.data);
  }
  BoolArray cond = fix->GT(e_diff, x.m_bits + 1);
  BoolArray s_neq = bool_op->XOR(a_s, b_s);

  FixArray ret_m_if = a_m;
  FixArray ret_e_if = a_e;

  FixArray a_m_prime, b_m_prime;
  if (cheap_variant) {
    a_m_prime = a_m;
    b_m_prime = fix->right_shift(b_m, e_diff, x.m_bits + 2, all_0.data);
  } else {
    a_m_prime = fix->left_shift(a_m, e_diff, 2 * x.m_bits + 3, x.m_bits + 1);
    b_m_prime = b_m;
  }
  b_m_prime = fix->if_else(s_neq, fix->mul(b_m_prime, -1), b_m_prime);
  FixArray ret_m_else = fix->add(a_m_prime, b_m_prime);
  FixArray ret_e_else = (cheap_variant ? a_e : b_e);

  FixArray ret_m = fix->if_else(cond, ret_m_if, ret_m_else);
  FixArray ret_e;
  if (cheap_variant) {
    ret_e = a_e;
  } else {
    ret_e = fix->if_else(cond, ret_e_if, ret_e_else);
  }

  if (!cheap_variant) {
    this->normalize(ret_m, ret_e, x.m_bits, 2 * x.m_bits + 2);
  } else {
    this->normalize(ret_m, ret_e, x.m_bits);
  }

  if (!cheap_variant) {
    ret_m = fix->reduce(ret_m, 2 * x.m_bits + 2);
    // fp_op->round_and_check(ret_m, ret_e, x.m_bits + 1);
    fp_op->round_and_check(ret_m, ret_e, 2 * x.m_bits + 1 - output_m_bits);
  } else {
    ret_m = fix->truncate_reduce(ret_m, 1);
  }

  BoolArray ret_z = fix->EQ(ret_m, 0);
  BoolArray ret_s = a_s;

  FPArray ret = this->input(this->party, x.size, ret_s.data, ret_z.data,
          ret_m.data, ret_e.data, output_m_bits, x.e_bits);

  // ret = this->if_else(x_z, y, ret);
  // ret = this->if_else(y_z, x, ret);

  if (check_bounds) {
    ret = this->check_bounds(ret);
  }

  return ret;
}

FPArray FPOp::sub(const FPArray &x, const FPArray &y, bool cheap_variant,
                  bool compare_and_swap, bool check_bounds) {
  assert(x.party != PUBLIC && y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.m_bits == y.m_bits);
  assert(x.e_bits == y.e_bits);
  FPArray neg_y = this->flip_sign(y);
  return this->add(x, neg_y, cheap_variant, compare_and_swap, check_bounds);
}

vector<FixArray> quotient_mantissa(FixOp *fix, const vector<FixArray> &m1, const FixArray &m2,
                           bool cheap_variant) {
  assert(m1[0].party != PUBLIC && m2.party != PUBLIC);
  assert(m1[0].size == m2.size);
  assert(m1[0].ell == (m2.ell + 2));
  int N = m1.size();
  for (int i = 1; i < N; i++) {
    assert(m1[0].party == m1[i].party);
    assert(m1[0].size == m1[i].size);
    assert(m1[0].ell == m1[i].ell);
  }
  int m_bits = m2.ell - 1;
  assert(m_bits <= 27);
  BoolArray all_0 = fix->bool_op->input(ALICE, m2.size, uint8_t(0));
  BoolArray all_1 = fix->bool_op->input(ALICE, m2.size, 1);

  int num_iter = 2;
  int p = ceil((m_bits + 1) / double(1 << num_iter)) + 1;

  // idx = m >> (m_bits-p) mod 2^{p}
  FixArray idx = fix->reduce(fix->truncate_reduce(m2, m_bits - p), p);
  int k = 1 << p;
  vector<uint64_t> spec_vec_r(1 << p);
  int32_t scale_curr = p + 1;
  for (int i = 0; i < (1 << p); i++) {
    double u = (1.0 + (double(i) / double(k)));
    double Y = 1.0 / u;
    spec_vec_r[i] = (Y * (1ULL << scale_curr));
  }
  FixArray r = fix->LUT(spec_vec_r, idx, false, scale_curr + 2, scale_curr, p);
  for (int i = 1; i <= num_iter; i++) {
    int32_t scale_next = (1 << i) * (p - 1) + 3;
    FixArray m2r =
        fix->mul(m2, r, scale_curr + m_bits + 2, all_1.data, all_0.data);
    m2r = fix->truncate_reduce(m2r, m_bits + scale_curr - scale_next);
    FixArray e = fix->sub(1ULL << scale_next, m2r);
    FixArray r_new = fix->scale_up(r, scale_next + 2, scale_next);
    e.signed_ = true;
    r.signed_ = true; // changing signed_ to enable signed mult
    FixArray re =
        fix->mul(r, e, scale_curr + scale_next + 2, all_0.data, nullptr);
    re.signed_ = false; // switching back to unsigned arithmetic
    r = fix->add(r_new, fix->truncate_reduce(re, scale_curr));
    scale_curr = scale_next;
  }

  FixArray r_replicated(r.party, N*r.size, r.signed_, r.ell, r.s);
  FixArray m2_replicated(m2.party, N*m2.size, m2.signed_, m2.ell, m2.s);
  for (int i = 0; i < N; i++) {
    memcpy(r_replicated.data + i*r.size, r.data, r.size * sizeof(uint64_t));
    memcpy(m2_replicated.data + i*m2.size, m2.data, m2.size * sizeof(uint64_t));
  }
  FixArray m2_ext_replicated(m2.party, N*m2.size, m2.signed_, m_bits+3, m2.s);
  if (!cheap_variant) {
      FixArray m2_ext = fix->extend(m2, m_bits + 3, all_1.data);
      for (int i = 0; i < N; i++) {
        memcpy(m2_ext_replicated.data + i*m2.size, m2_ext.data, m2.size * sizeof(uint64_t));
      }
      m2_ext_replicated.s = 2 * m_bits;
  }
  BoolArray all_0_flat = fix->bool_op->input(ALICE, N*m2.size, uint8_t(0));
  BoolArray all_1_flat = fix->bool_op->input(ALICE, N*m2.size, 1);

  FixArray q_flat;
  FixArray m1_flat = concat(m1);
  if (cheap_variant) {
    q_flat = fix->mul(m1_flat, r_replicated, scale_curr + m_bits + 1, all_0_flat.data, all_0_flat.data);
    q_flat = fix->truncate_reduce(q_flat, scale_curr);
  } else {
    q_flat = fix->mul(m1_flat, r_replicated, scale_curr + m_bits + 2, all_0_flat.data, all_0_flat.data);
    q_flat = fix->truncate_reduce(q_flat, scale_curr);
    FixArray m2q = fix->mul(m2_replicated, q_flat, m_bits + 3, all_1_flat.data, all_0_flat.data);
    FixArray m2q_plus_1 = fix->add(m2q, m2_ext_replicated);
    FixArray m1_scaled = fix->scale_up(m1_flat, m_bits + 3, 2 * m_bits + 1);
    m1_scaled.s -= 1;
    BoolArray lt, eq;
    tie(lt, eq) = fix->LT_and_EQ(fix->add(m2q, m2q_plus_1), m1_scaled);
    BoolArray odd_repr = fix->LSB(q_flat);
    BoolArray add_1 = fix->bool_op->XOR(lt, fix->bool_op->AND(odd_repr, eq));
    q_flat = fix->if_else(add_1, fix->add(q_flat, 1), q_flat);
  }
  q_flat = fix->reduce(q_flat, m_bits + 1);

  vector<FixArray> q(N);
  for (int i = 0; i < N; i++) {
    q[i] = q_flat.subset(i*m2.size, (i+1)*m2.size);
  }

  return q;
}

vector<FPArray> FPOp::div(const vector<FPArray> &x, const FPArray &y, bool cheap_variant,
                  bool check_bounds) {
  assert(x[0].party != PUBLIC && y.party != PUBLIC);
  assert(x[0].size == y.size);
  assert(x[0].m_bits == y.m_bits);
  assert(x[0].e_bits == y.e_bits);
  int N = x.size();
  int n = x[0].size;
  for (int i = 1; i < N; i++) {
    assert(x[0].party == x[i].party);
    assert(x[0].size == x[i].size);
    assert(x[0].m_bits == x[i].m_bits);
    assert(x[0].e_bits == x[i].e_bits);
  }

  FPArray x_flat = concat(x);
  BoolArray all_1 = bool_op->input(ALICE, x_flat.size, 1);

  BoolArray x_s_flat, x_z_flat;
  FixArray x_m_flat, x_e_flat;
  tie(x_s_flat, x_z_flat, x_m_flat, x_e_flat) = get_components(x_flat);
  BoolArray y_s, y_z;
  FixArray y_m, y_e;
  tie(y_s, y_z, y_m, y_e) = get_components(y);

  BoolArray msb_x_m_flat = bool_op->NOT(x_z_flat);

  BoolArray y_s_replicated = BoolArray(y_s.party, N*y_s.size);
  BoolArray y_z_replicated = BoolArray(y_z.party, N*y_z.size);
  FixArray y_m_replicated = FixArray(y_m.party, N*y_m.size, y_m.signed_, y_m.ell, y_m.s);
  FixArray y_e_replicated = FixArray(y_e.party, N*y_e.size, y_e.signed_, y_e.ell, y_e.s);
  for (int i = 0; i < N; i++) {
    memcpy(y_s_replicated.data + i*n, y_s.data, n*sizeof(uint8_t));
    memcpy(y_z_replicated.data + i*n, y_z.data, n*sizeof(uint8_t));
    memcpy(y_m_replicated.data + i*n, y_m.data, n*sizeof(uint64_t));
    memcpy(y_e_replicated.data + i*n, y_e.data, n*sizeof(uint64_t));
  }
  FixArray ret_e_flat = fix->sub(x_e_flat, y_e_replicated);
  ret_e_flat = fix->add(ret_e_flat, y.e_bias());
  BoolArray denormal_m_flat = fix->LT(x_m_flat, y_m_replicated);
  /* x_m_flat = fix->extend(x_m_flat, y.m_bits + 3, all_1.data); */
  x_m_flat = fix->extend(x_m_flat, y.m_bits + 3, msb_x_m_flat.data);
  FixArray x_m_if_flat = fix->mul(x_m_flat, 2, y.m_bits + 3);
  x_m_flat = fix->if_else(denormal_m_flat, x_m_if_flat, x_m_flat);
  FixArray ret_e_if_flat = fix->sub(ret_e_flat, 1);
  ret_e_flat = fix->if_else(denormal_m_flat, ret_e_if_flat, ret_e_flat);

  vector<FixArray> x_m(N);
  for (int i = 0; i < N; i++) {
    x_m[i] = x_m_flat.subset(i*n, (i+1)*n);
  }

  vector<FixArray> q = quotient_mantissa(fix, x_m, y_m, cheap_variant);
  FixArray q_flat = concat(q);

  BoolArray ret_s_flat = bool_op->XOR(x_s_flat, y_s_replicated);
  BoolArray ret_z_flat = x_z_flat;

  FPArray ret_flat = this->input(this->party, x_flat.size, ret_s_flat.data, ret_z_flat.data,
          q_flat.data, ret_e_flat.data, y.m_bits, y.e_bits);

  if (check_bounds) {
    ret_flat = this->check_bounds(ret_flat);
  }

  vector<FPArray> ret(N);
  for (int i = 0; i < N; i++) {
    ret[i] = ret_flat.subset(i*n, (i+1)*n);
  }

  return ret;
}

FixArray sqrt_mantissa(FixOp *fix, const BoolArray &oddExp, const FixArray &m) {
  assert(m.party != PUBLIC && oddExp.party != PUBLIC);
  assert(oddExp.size == m.size);
  int m_bits = m.ell - 1;
  assert(m_bits <= 27);
  BoolArray all_0 = fix->bool_op->input(ALICE, m.size, uint8_t(0));
  BoolArray all_1 = fix->bool_op->input(ALICE, m.size, 1);

  FixArray m_prime = fix->extend(m, m_bits + 3, all_1.data);
  m_prime = fix->if_else(oddExp, fix->mul(m_prime, 2), m_prime);

  int num_iter = 2;
  int p = ceil((m_bits + 1) / double(1 << num_iter)) + 1;

  // idx = oddExp || (m >> (m_bits-(p-1)) mod 2^{p-1})
  FixArray idx =
      fix->sub(fix->truncate_reduce(m, m_bits - p + 1), 1ULL << (p - 1));
  idx = fix->add(idx, fix->scale_up(fix->B2A(oddExp, false, 1), p, p - 1));
  int k = 1 << (p - 1);
  vector<uint64_t> spec_vec_r(1 << p);
  int m_msb_mask = k - 1;
  int32_t scale_curr = p + 1;
  for (int i = 0; i < (1 << p); i++) {
    int exp_parity = i >> (p - 1);
    int m_msb = i & m_msb_mask;
    double u = (1.0 + (double(m_msb) / double(k))) * (1 << exp_parity);
    double Y = 1.0 / sqrt(u);
    spec_vec_r[i] = (Y * (1ULL << scale_curr));
  }
  FixArray r = fix->LUT(spec_vec_r, idx, false, scale_curr + 2, scale_curr, p);
  for (int i = 1; i <= num_iter; i++) {
    int32_t scale_next = (1 << i) * (p - 1) + 5;
    FixArray r_sq = fix->mul(r, r, 2 * scale_curr + 2, all_0.data, all_0.data);
    FixArray mr_sq = fix->mul(m_prime, r_sq, 2 * scale_curr + m_bits + 2,
                              all_0.data, all_0.data);
    mr_sq = fix->truncate_reduce(mr_sq, m_bits + 2 * scale_curr - scale_next);
    FixArray e = fix->sub(1ULL << scale_next, mr_sq);
    FixArray r_new = fix->scale_up(r, scale_next + 2, scale_next);
    e.signed_ = true;
    r.signed_ = true; // changing signed_ to enable signed mult
    FixArray re =
        fix->mul(r, e, scale_curr + scale_next + 3, all_0.data, nullptr);
    re.signed_ = false; // switching back to unsigned arithmetic
    re.s +=
        1; // increasing scale by 1 because we want r_new = (r << scale) + re/2
    r = fix->add(r_new, fix->truncate_reduce(re, scale_curr + 1));
    scale_curr = scale_next;
  }

  FixArray q =
      fix->mul(m_prime, r, scale_curr + m_bits + 2, all_0.data, all_0.data);
  q = fix->truncate_reduce(q, scale_curr);
  FixArray q_sq = fix->mul(q, q, m_bits + 4, all_0.data, all_0.data);
  FixArray two_q = fix->mul(fix->extend(q, m_bits + 4, all_0.data), 2);
  two_q.s = 2 * m_bits;
  FixArray q_plus_1_sq = fix->add(fix->add(q_sq, 1), two_q);
  FixArray m_scaled = fix->scale_up(m_prime, m_bits + 4, 2 * m_bits + 1);
  m_scaled.s -= 1;
  BoolArray lt, eq;
  tie(lt, eq) = fix->LT_and_EQ(fix->add(q_sq, q_plus_1_sq), m_scaled);
  BoolArray odd_repr = fix->LSB(q);
  BoolArray add_1 = fix->bool_op->XOR(lt, fix->bool_op->AND(odd_repr, eq));
  q = fix->if_else(add_1, fix->add(q, 1), q);
  return fix->reduce(q, m_bits + 1);
}

FPArray FPOp::sqrt(const FPArray &x) {
  assert(x.party != PUBLIC);
  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  BoolArray all_1 = bool_op->input(ALICE, x.size, 1);

  BoolArray x_s, x_z;
  FixArray x_m, x_e;
  tie(x_s, x_z, x_m, x_e) = get_components(x);

  FixArray e_plus_1 = fix->add(x_e, 1);
  BoolArray oddExp = fix->LSB(e_plus_1);
  FixArray ret_e = fix->right_shift(e_plus_1, 1);
  ret_e.s += 1; // right_shift decreases scale by 1
  ret_e = fix->add(ret_e, (x.e_bias() - 1) / 2);

  FixArray q = sqrt_mantissa(fix, oddExp, x_m);

  BoolArray ret_z = x_z;
  uint64_t m_0 = 0;
  uint64_t e_0 = x.e_min() - 1;
  FixArray ret_m_if = fix->input(PUBLIC, x.size, m_0, x_m.signed_, x_m.ell, x_m.s);
  FixArray ret_e_if = fix->input(PUBLIC, x.size, e_0, x_e.signed_, x_e.ell, x_e.s);

  FixArray ret_m = fix->if_else(ret_z, ret_m_if, q);
  ret_e = fix->if_else(ret_z, ret_e_if, ret_e);

  FPArray ret = this->input(x.party, x.size, all_0.data, ret_z.data,
          ret_m.data, ret_e.data, x.m_bits, x.e_bits);

  return ret;
}

FPArray FPOp::int_to_float(const FixArray &x, int m_bits, int e_bits) {
  assert(x.party != PUBLIC);
  assert(x.s == 0);
  assert(x.ell <= (1 << (e_bits - 1)) - 1);

  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  FixArray f;
  BoolArray msb_x, f_eq_0;
  if (x.signed_ == true) {
    tie(msb_x, f_eq_0) = fix->MSB_and_zero_test(x);
    f = fix->reduce(fix->if_else(msb_x, fix->mul(x, -1), x), x.ell - 1);
    f.signed_ = false;
  } else {
    f = x;
    msb_x = all_0;
    f_eq_0 = fix->EQ(f, 0);
  }
  uint64_t delta_e_bias = (1ULL << (e_bits - 1)) - 1;
  FixArray delta_m = f;
  FixArray delta_e = fix->input(ALICE, x.size, uint64_t(0), true, e_bits + 2, 0);
  this->normalize(delta_m, delta_e, -1 * delta_e_bias);
  round_and_check(delta_m, delta_e, f.ell - 1 - m_bits);
  delta_e = fix->if_else(f_eq_0, 0, delta_e);
  FPArray delta = this->input(x.party, x.size, msb_x.data, f_eq_0.data,
          delta_m.data, delta_e.data, m_bits, e_bits);

  return delta;
}

FPArray FPOp::max(const vector<FPArray>& x) {
  int N = x.size();
  int n = x[0].size;
  int party = x[0].party;
  int m_bits = x[0].m_bits;
  int e_bits = x[0].e_bits;
  for (int i = 1; i < N; i++) {
    assert(x[i].party == party);
    assert(x[i].m_bits == m_bits);
    assert(x[i].e_bits == e_bits);
    assert(x[i].size == n);
  }

  vector<FPArray> x_tr(n);
  for (int i = 0; i < n; i++) {
    x_tr[i] = FPArray(party, N, m_bits, e_bits);
    for (int j = 0; j < N; j++) {
      x_tr[i].s[j] = x[j].s[i];
      x_tr[i].z[j] = x[j].z[i];
      x_tr[i].m[j] = x[j].m[i];
      x_tr[i].e[j] = x[j].e[i];
    }
  }
  int num_cmps_old = n; int num_cmps_curr = n/2;
  while(num_cmps_old > 1) {
    int odd_num_cmps = num_cmps_old & 1;
    vector<FPArray> lhs(num_cmps_curr); vector<FPArray> rhs(num_cmps_curr);
    for (int j = odd_num_cmps; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      lhs[j/2] = x_tr[j]; rhs[j/2] = x_tr[j+1];
    }
    FPArray lhs_concat = concat(lhs);
    FPArray rhs_concat = concat(rhs);
    BoolArray cond = fp_op->GT(lhs_concat, rhs_concat);
    lhs_concat = fp_op->if_else(cond, lhs_concat, rhs_concat);
    for (int j = 0; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      x_tr[odd_num_cmps + (j/2)] = lhs_concat.subset((j/2)*N, (j/2)*N + N);
    }
    num_cmps_old = num_cmps_curr + odd_num_cmps;
    num_cmps_curr = num_cmps_old/2;
  }

  return x_tr[0];
}

FPArray FPOp::treesum(const vector<FPArray> &x) {
  int N = x.size();
  int n = x[0].size;
  int party = x[0].party;
  int m_bits = x[0].m_bits;
  int e_bits = x[0].e_bits;
  for (int i = 1; i < N; i++) {
    assert(x[i].party == party);
    assert(x[i].m_bits == m_bits);
    assert(x[i].e_bits == e_bits);
    assert(x[i].size == n);
  }

  vector<FPArray> x_tr(n);
  for (int i = 0; i < n; i++) {
    x_tr[i] = FPArray(party, N, m_bits, e_bits);
    for (int j = 0; j < N; j++) {
      x_tr[i].s[j] = x[j].s[i];
      x_tr[i].z[j] = x[j].z[i];
      x_tr[i].m[j] = x[j].m[i];
      x_tr[i].e[j] = x[j].e[i];
    }
  }
  int num_cmps_old = n; int num_cmps_curr = n/2;
  while(num_cmps_old > 1) {
    int odd_num_cmps = num_cmps_old & 1;
    vector<FPArray> lhs(num_cmps_curr); vector<FPArray> rhs(num_cmps_curr);
    for (int j = odd_num_cmps; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      lhs[j/2] = x_tr[j]; rhs[j/2] = x_tr[j+1];
    }
    FPArray lhs_concat = concat(lhs);
    FPArray rhs_concat = concat(rhs);
    lhs_concat = this->add(lhs_concat, rhs_concat) ;
    for (int j = 0; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      x_tr[odd_num_cmps + (j/2)] = lhs_concat.subset((j/2)*N, (j/2)*N + N);
    }
    num_cmps_old = num_cmps_curr + odd_num_cmps;
    num_cmps_curr = num_cmps_old/2;
  }

  return x_tr[0];
}

vector<FPArray> enlist_products(FPOp* fp_op, const FPMatrix &x, const FPMatrix &y) {
  assert(x.party != PUBLIC); assert(y.party != PUBLIC);
  assert(x.dim2 == y.dim1);
  assert(x.m_bits == y.m_bits); assert(x.e_bits == y.e_bits);

  int N = x.dim1*y.dim2; int n = x.dim2;
  int m_bits = x.m_bits; int e_bits = x.e_bits;

  vector<FPArray> prod_x(N), prod_y(N) ;
  for (int i = 0 ; i < x.dim1 ; i++) {
    for (int j = 0 ; j < y.dim2 ; j++) {
      int ind = i*y.dim2 + j ;
      prod_x[ind] = FPArray(fp_op->party, n, m_bits, e_bits) ; 
      prod_y[ind] = FPArray(fp_op->party, n, m_bits, e_bits) ;

      for (int k = 0 ; k < n ; k++) {
        prod_x[ind].s[k] = x.s[i*x.dim2+k] ;
        prod_x[ind].z[k] = x.z[i*x.dim2+k] ;
        prod_x[ind].m[k] = x.m[i*x.dim2+k] ;
        prod_x[ind].e[k] = x.e[i*x.dim2+k] ;

        prod_y[ind].s[k] = y.s[k*y.dim2+j] ;
        prod_y[ind].z[k] = y.z[k*y.dim2+j] ;
        prod_y[ind].m[k] = y.m[k*y.dim2+j] ;
        prod_y[ind].e[k] = y.e[k*y.dim2+j] ;
      }
    }
  }

  return fp_op->mul(prod_x, prod_y) ;
}

FPMatrix FPOp::matrix_multiplication(const FPMatrix &x, const FPMatrix &y) {
  assert(x.party != PUBLIC); assert(y.party != PUBLIC);
  assert(x.dim2 == y.dim1);
  assert(x.m_bits == y.m_bits); assert(x.e_bits == y.e_bits);

  int N = x.dim1*y.dim2; int n = x.dim2;
  int m_bits = x.m_bits; int e_bits = x.e_bits;
  
  vector<FPArray> prod = enlist_products(this, x, y) ;
  FPMatrix ret(this->party, x.dim1, y.dim2, m_bits, e_bits) ;
  int rows_per_batch = ceil(CHUNK_SIZE/double(n));
  for (int i = 0; i < N; i += rows_per_batch) {
    int j = std::min(i + rows_per_batch, N);
    vector<FPArray> prod_i = {prod.begin() + i, prod.begin() + j};
    FPArray ret_i = treesum(prod_i);
    memcpy(ret.s + i, ret_i.s, (j-i) * sizeof(uint8_t));
    memcpy(ret.z + i, ret_i.z, (j-i) * sizeof(uint8_t));
    memcpy(ret.m + i, ret_i.m, (j-i) * sizeof(uint64_t));
    memcpy(ret.e + i, ret_i.e, (j-i) * sizeof(uint64_t));
  }
  return ret ;
}