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

#include "FloatingPoint/fixed-point.h"

using namespace std;
using namespace sci;

FixArray FixArray::subset(int i, int j) {
  assert(i >= 0 && j <= size && i < j);
  int sz = j - i;
  FixArray ret(this->party, sz, this->signed_, this->ell, this->s);
  memcpy(ret.data, this->data + i, sz * sizeof(uint64_t));
  return ret;
}


FixArray concat(const vector<FixArray>& x) {
  int N = x.size();
  int sz = x[0].size;
  bool signed_ = x[0].signed_;
  int ell = x[0].ell;
  int s = x[0].s;
  int party = x[0].party;
  for (int i = 1; i < N; i++) {
    sz += x[i].size;
    assert(signed_ == x[i].signed_);
    assert(ell == x[i].ell);
    assert(s == x[i].s);
    assert(party == x[i].party);
  }
  FixArray ret(party, sz, signed_, ell, s);
  int offset = 0;
  for (int i = 0; i < N; i++) {
    int n = x[i].size;
    memcpy(ret.data + offset, x[i].data, n * sizeof(uint64_t));
    offset += n;
  }
  return ret;
}

template <class T> std::vector<T> FixArray::get_native_type() {
  assert(this->party == PUBLIC);
  if constexpr (is_same_v<T, uint32_t> || is_same_v<T, uint64_t>) {
    assert(this->signed_ == false);
  }
  vector<T> ret(this->size);
  double den = pow(2.0, this->s);
  for (int i = 0; i < this->size; i++) {
    int64_t data_ = (this->signed_ ? signed_val(this->data[i], this->ell) : this->data[i]);
    ret[i] = T(data_ / den);
  }
  return ret;
}

std::ostream &operator<<(std::ostream &os, FixArray &other) {
  assert(other.party == PUBLIC);
  vector<double> dbl_other = other.get_native_type<double>();
  os << "(ell: " << other.ell << ", s: " << other.s << ") \t[";
  for (int i = 0; i < other.size; i++) {
    int64_t data_ =
        (other.signed_ ? signed_val(other.data[i], other.ell) : other.data[i]);
    std::string tmp_data = std::bitset<64>(data_).to_string();
    os << dbl_other[i] << " int=" << data_ << " ("
       << tmp_data.substr(64 - other.ell, 64) << ")\t";
  }
  os << "]";
  return os;
}

template vector<uint32_t> FixArray::get_native_type();
template vector<int32_t> FixArray::get_native_type();
template vector<uint64_t> FixArray::get_native_type();
template vector<int64_t> FixArray::get_native_type();
template vector<float> FixArray::get_native_type();
template vector<double> FixArray::get_native_type();

FixArray FixOp::input(int party_, int sz, uint64_t* data_, bool signed__, int ell_, int s_) {
  FixArray ret((party_ == PUBLIC ? party_ : this->party), sz, signed__, ell_, s_);
  uint64_t ell_mask_ = ret.ell_mask();
  if ((this->party == party_) || (party_ == PUBLIC)) {
    memcpy(ret.data, data_, sz * sizeof(uint64_t));
    for (int i = 0; i < sz; i++) {
      ret.data[i] &= ell_mask_;
    }
  } else {
    for (int i = 0; i < sz; i++) {
      ret.data[i] = 0;
    }
  }
  return ret;
}

FixArray FixOp::input(int party_, int sz, uint64_t data_, bool signed__, int ell_, int s_) {
  FixArray ret((party_ == PUBLIC ? party_ : this->party), sz, signed__, ell_, s_);
  uint64_t ell_mask_ = ret.ell_mask();
  if ((this->party == party_) || (party_ == PUBLIC)) {
    for (int i = 0; i < sz; i++) {
      ret.data[i] = data_ & ell_mask_;
    }
  } else {
    for (int i = 0; i < sz; i++) {
      ret.data[i] = 0;
    }
  }
  return ret;
}

FixArray FixOp::output(int party_, const FixArray& x) {
  if (x.party == PUBLIC) {
    return x;
  }
  int sz = x.size;
  int ret_party = (party_ == PUBLIC || party_ == x.party ? PUBLIC : x.party);
  FixArray ret(ret_party, sz, x.signed_, x.ell, x.s);
#pragma omp parallel num_threads(2)
  {
    if (omp_get_thread_num() == 1 && party_ != BOB) {
      if (party == sci::ALICE) {
        iopack->io_rev->recv_data(ret.data, sz * sizeof(uint64_t));
      } else { // party == sci::BOB
        iopack->io_rev->send_data(x.data, sz * sizeof(uint64_t));
      }
    } else if (omp_get_thread_num() == 0 && party_ != ALICE) {
      if (party == sci::ALICE) {
        iopack->io->send_data(x.data, sz * sizeof(uint64_t));
      } else { // party == sci::BOB
        iopack->io->recv_data(ret.data, sz * sizeof(uint64_t));
      }
    }
  }
  uint64_t ell_mask_ = x.ell_mask();
  for (int i = 0; i < sz; i++) {
    ret.data[i] = (ret.data[i] + x.data[i]) & ell_mask_;
  }
  return ret;
}

FixArray FixOp::if_else(const BoolArray &cond, const FixArray &x,
                        const FixArray &y) {
  assert(cond.party != PUBLIC);
  assert(cond.size == x.size && cond.size == y.size);
  assert(x.signed_ == y.signed_);
  assert(x.ell == y.ell);
  assert(x.s == y.s);
  FixArray ret(this->party, x.size, x.signed_, x.ell, x.s);
  FixArray diff = this->sub(x, y);
  if (diff.party == PUBLIC) {
    FixArray cond_fix = this->B2A(cond, x.signed_, x.ell);
    cond_fix.s = x.s;
    ret = this->mul(cond_fix, diff, x.ell);
  } else {
    aux->multiplexer(cond.data, diff.data, ret.data, x.size, x.ell, x.ell);
  }
  return this->add(ret, y);
}

FixArray FixOp::if_else(const BoolArray &cond, const FixArray &x, uint64_t y) {
  assert(cond.party != PUBLIC);
  assert(cond.size == x.size);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->if_else(cond, x, y_fix);
}

FixArray FixOp::if_else(const BoolArray &cond, uint64_t x, const FixArray &y) {
  assert(cond.party != PUBLIC);
  assert(cond.size == y.size);
  FixArray x_fix = this->input(PUBLIC, y.size, x, y.signed_, y.ell, y.s);
  return this->if_else(cond, x_fix, y);
}

FixArray FixOp::add(const FixArray &x, const FixArray &y) {
  assert(x.size == y.size);
  assert(x.signed_ == y.signed_);
  assert(x.ell == y.ell);
  assert(x.s == y.s);

  bool x_cond, y_cond;
  int party_;
  if (x.party == PUBLIC && y.party == PUBLIC) {
    x_cond = false; y_cond = false;
    party_ = PUBLIC;
  } else {
    x_cond = (x.party == PUBLIC) && (this->party == BOB);
    y_cond = (y.party == PUBLIC) && (this->party == BOB);
    party_ = this->party;
  }
  FixArray ret(party_, x.size, x.signed_, x.ell, x.s);
  uint64_t ell_mask_ = x.ell_mask();
  for (int i = 0; i < x.size; i++) {
    ret.data[i] = ((x_cond ? 0 : x.data[i]) + (y_cond ? 0 : y.data[i])) & ell_mask_;
  }
  return ret;
}

FixArray FixOp::add(const FixArray &x, uint64_t y) {
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->add(x, y_fix);
}

FixArray FixOp::sub(const FixArray &x, const FixArray &y) {
  FixArray neg_y = this->mul(y, uint64_t(-1));
  return this->add(x, neg_y);
}

FixArray FixOp::sub(const FixArray &x, uint64_t y) {
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->sub(x, y_fix);
}

FixArray FixOp::sub(uint64_t x, const FixArray &y) {
  FixArray x_fix = this->input(PUBLIC, y.size, x, y.signed_, y.ell, y.s);
  return this->sub(x_fix, y);
}

FixArray FixOp::extend(const FixArray &x, int ell, uint8_t *msb_x) {
  assert(ell >= x.ell);
  FixArray ret(x.party, x.size, x.signed_, ell, x.s);
  if (x.signed_) {
    if (x.party == PUBLIC) {
      uint64_t ret_mask = ret.ell_mask();
      for (int i = 0; i < x.size; i++) {
        ret.data[i] = uint64_t(signed_val(x.data[i], x.ell)) & ret_mask;
      }
    } else {
      xt->s_extend(x.size, x.data, ret.data, x.ell, ell, msb_x);
    }
  } else {
    if (x.party == PUBLIC) {
      memcpy(ret.data, x.data, x.size*sizeof(uint64_t));
    } else {
      xt->z_extend(x.size, x.data, ret.data, x.ell, ell, msb_x);
    }
  }
  return ret;
}

FixArray FixOp::B2A(const BoolArray &x, bool signed_, int ell) {
  assert(ell >= 1);
  FixArray ret(x.party, x.size, signed_, ell, 0);
  if (x.party == PUBLIC) {
    for (int i = 0; i < x.size; i++) {
        ret.data[i] = uint64_t(x.data[i] & 1);
    }
  } else {
    aux->B2A(x.data, ret.data, x.size, ret.ell);
  }
  return ret;
}

FixArray FixOp::mul(const FixArray &x, const FixArray &y, int ell,
        uint8_t *msb_x, uint8_t *msb_y) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.signed_ || (x.signed_ == y.signed_));
  assert(ell >= x.ell && ell >= y.ell && ell <= x.ell + y.ell);
  FixArray ret(this->party, x.size, x.signed_, ell, x.s + y.s);
  if (x.party == PUBLIC || y.party == PUBLIC) {
    FixArray x_ext = this->extend(x, ell, msb_x);
    FixArray y_ext = this->extend(y, ell, msb_y);
    uint64_t ret_mask = ret.ell_mask();
    for (int i = 0; i < x.size; i++) {
      ret.data[i] = (x_ext.data[i] * y_ext.data[i]) & ret_mask;
    }
  } else {
    mult->hadamard_product(x.size, x.data, y.data, ret.data, x.ell, y.ell, ell,
                           x.signed_, y.signed_, MultMode::None, msb_x, msb_y);
  }
  return ret;
}

FixArray FixOp::mul(const FixArray &x, uint64_t y, int ell, uint8_t *msb_x) {
  assert(ell >= x.ell);
  FixArray ret;
  if (ell > x.ell) {
    ret = this->extend(x, ell, msb_x);
  } else {
    ret = x;
  }
  uint64_t ell_mask_ = ret.ell_mask();
  for (int i = 0; i < x.size; i++) {
    ret.data[i] = (y * ret.data[i]) & ell_mask_;
  }
  return ret;
}

FixArray FixOp::left_shift(const FixArray &x, const FixArray &s, int ell,
                           int bound, uint8_t *msb_x) {
  assert(x.party != PUBLIC && s.party != PUBLIC);
  assert(x.size == s.size);
  assert(ell <= x.ell + bound && ell >= x.ell && ell >= bound);
  int m = ceil(log2(bound + 1));
  assert(s.signed_ == false && s.s == 0 && s.ell >= m);
  int pow2_s_ell = bound + 2;
  if (pow2_s_ell > ell) pow2_s_ell = ell;
  FixArray pow2_s(x.party, x.size, x.signed_, pow2_s_ell, 0);
  int M = 1 << m;
  uint64_t pow2_s_mask = pow2_s.ell_mask();
  uint64_t s_mask = M - 1;
  if (party == ALICE) {
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(pow2_s.data, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[M];
      pow2_s.data[i] &= pow2_s_mask;
      for (int j = 0; j < M; j++) {
        int idx = (s.data[i] + j) & s_mask;
        if (idx > pow2_s_ell - 2) {
          spec[i][j] = (0 - pow2_s.data[i]) & pow2_s_mask;
        } else {
          spec[i][j] = ((1ULL << idx) - pow2_s.data[i]) & pow2_s_mask;
        }
      }
    }
    aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, m, pow2_s_ell);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    aux->lookup_table<uint64_t>(nullptr, s.data, pow2_s.data, x.size, m,
                                pow2_s_ell);
  }
  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  FixArray ret = this->mul(x, pow2_s, ell, msb_x, all_0.data);

  return ret;
}

FixArray FixOp::right_shift(const FixArray &x, const FixArray &s, int bound,
                            uint8_t *msb_x) {
  assert(x.party != PUBLIC && s.party != PUBLIC);
  assert(x.size == s.size);
  assert(bound <= x.ell && bound + x.ell < 64);
  int m = ceil(log2(bound));
  assert(s.signed_ == false && s.s == 0 && s.ell >= m);
  FixArray pow2_neg_s(x.party, x.size, x.signed_, bound + 2, bound);
  int M = 1 << m;
  uint64_t pow2_neg_s_mask = pow2_neg_s.ell_mask();
  uint64_t s_mask = M - 1;
  if (party == ALICE) {
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(pow2_neg_s.data, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[M];
      pow2_neg_s.data[i] &= pow2_neg_s_mask;
      for (int j = 0; j < M; j++) {
        int idx = (s.data[i] + j) & s_mask;
        int exp = bound - idx;
        if (exp < 0)
          exp += M;
        spec[i][j] = ((1ULL << exp) - pow2_neg_s.data[i]) & pow2_neg_s_mask;
      }
    }
    aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, m, bound + 2);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    aux->lookup_table<uint64_t>(nullptr, s.data, pow2_neg_s.data, x.size, m,
                                bound + 2);
  }
  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  FixArray ret = this->mul(x, pow2_neg_s, x.ell + bound, msb_x, all_0.data);
  ret = this->truncate_reduce(ret, bound);

  return ret;
}

FixArray FixOp::right_shift(const FixArray &x, int s, uint8_t *msb_x) {
  assert(x.party != PUBLIC);
  assert(s <= x.ell && s >= 0);
  FixArray ret(x.party, x.size, x.signed_, x.ell, x.s - s);
  trunc->truncate(x.size, x.data, ret.data, s, x.ell, x.signed_, msb_x);
  return ret;
}

FixArray FixOp::truncate_reduce(const FixArray &x, int s, uint8_t *wrap_x_s) {
  assert(x.party != PUBLIC);
  assert(s < x.ell && s >= 0);
  FixArray ret(x.party, x.size, x.signed_, x.ell - s, x.s - s);
  if (wrap_x_s != nullptr) {
    aux->B2A(wrap_x_s, ret.data, x.size, x.ell - s);
    uint64_t ret_mask = ret.ell_mask();
    for (int i = 0; i < x.size; i++) {
      ret.data[i] = (ret.data[i] + (x.data[i] >> s)) & ret_mask;
    }
  } else {
    trunc->truncate_and_reduce(x.size, x.data, ret.data, s, x.ell);
  }
  return ret;
}

FixArray FixOp::truncate_with_sticky_bit(const FixArray &x, int s) {
  assert(x.party != PUBLIC);
  assert(s < x.ell && s >= 0);
  if (s == 0) return x;
  FixArray x_s = reduce(x, s);
  BoolArray wrap, zero_test;
  tie(wrap, zero_test) = wrap_and_zero_test(x_s);
  FixArray ret = truncate_reduce(x, s, wrap.data);
  uint8_t *not_lsb_x = new uint8_t[x.size];
  uint8_t *corr_bits = new uint8_t[x.size];
  for (int i = 0; i < x.size; i++) {
    not_lsb_x[i] = (ret.data[i] & 1) ^ (party == ALICE ? 1 : 0);
  }
  BoolArray nonzero_test = bool_op->NOT(zero_test);
  FixArray correction(x.party, x.size, x.signed_, x.ell - s, x.s - s);
  aux->AND(not_lsb_x, nonzero_test.data, corr_bits, x.size);
  aux->B2A(corr_bits, correction.data, x.size, ret.ell);
  ret = add(ret, correction);

  delete[] not_lsb_x;
  delete[] corr_bits;
  return ret;
}

FixArray FixOp::round_ties_to_even(const FixArray &x, int s) {
  assert(x.party != PUBLIC);
  assert(s <= x.ell && s >= 2);
  FixArray x_ = truncate_with_sticky_bit(x, s - 2);
  FixArray ret = truncate_reduce(x_, 2);
  uint8_t *corr_bit = new uint8_t[x.size];
  uint8_t *x_lower_3 = new uint8_t[x.size];
  for (int i = 0; i < x.size; i++) {
    x_lower_3[i] = x_.data[i] & 7;
  }
  if (party == ALICE) {
    uint8_t **spec;
    spec = new uint8_t *[x.size];
    PRG128 prg;
    prg.random_data(corr_bit, x.size * sizeof(uint8_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint8_t[8];
      corr_bit[i] &= 1;
      for (int j = 0; j < 8; j++) {
        int idx = (x_lower_3[i] + j) & 7;
        bool a = idx & 1;
        bool b = (idx >> 1) & 1;
        bool c = (idx >> 2) & 1;
        uint8_t out = (b && (a || c));
        spec[i][j] = (out - corr_bit[i]) & 1;
      }
    }
    aux->lookup_table<uint8_t>(spec, nullptr, nullptr, x.size, 3, 1);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    aux->lookup_table<uint8_t>(nullptr, x_lower_3, corr_bit, x.size, 3, 1);
  }
  FixArray correction(x.party, x.size, x.signed_, x.ell - s, x.s - s);
  aux->B2A(corr_bit, correction.data, x.size, correction.ell);

  delete[] corr_bit;
  delete[] x_lower_3;

  return add(ret, correction);
}

FixArray FixOp::scale_up(const FixArray &x, int ell, int s) {
  assert(ell - x.ell <= s - x.s);
  assert(s >= x.s);
  FixArray ret(x.party, x.size, x.signed_, ell, s);
  uint64_t ell_mask_ = ret.ell_mask();
  for (int i = 0; i < x.size; i++) {
    ret.data[i] = (x.data[i] << (s - x.s)) & ell_mask_;
  }
  return ret;
}

FixArray FixOp::reduce(const FixArray &x, int ell) {
  assert(ell <= x.ell && ell > 0);
  FixArray ret(x.party, x.size, x.signed_, ell, x.s);
  uint64_t ell_mask_ = ret.ell_mask();
  for (int i = 0; i < x.size; i++) {
    ret.data[i] = x.data[i] & ell_mask_;
  }
  return ret;
}

BoolArray FixOp::LSB(const FixArray &x) {
  BoolArray ret(x.party, x.size);
  for (int i = 0; i < x.size; i++) {
    ret.data[i] = x.data[i] & 1;
  }
  return ret;
}

tuple<BoolArray,BoolArray> FixOp::wrap_and_zero_test(const FixArray &x) {
  assert(x.party != PUBLIC);
  BoolArray wrap(x.party, x.size);
  BoolArray zero_test(x.party, x.size);
  uint64_t *mill_inp = new uint64_t[x.size];
  for (int i = 0; i < x.size; i++) {
    mill_inp[i] = (party == ALICE ? x.data[i] : (1ULL << x.ell) - x.data[i]);
  }
  mill_eq->compare_with_eq(wrap.data, zero_test.data, mill_inp, x.size, x.ell + 1, true);
  wrap = bool_op->XOR(wrap, zero_test);

  BoolArray eq_corr(this->party, x.size);
  uint8_t *ztest_share_ALICE = new uint8_t[x.size];
  uint8_t *ztest_share_BOB = new uint8_t[x.size];
  for (int i = 0; i < x.size; i++) {
    ztest_share_ALICE[i] = (party == ALICE ? (x.data[i] == 0) : 0);
    ztest_share_BOB[i] = (party == BOB ? (x.data[i] == 0) : 0);
  }
  aux->AND(ztest_share_ALICE, ztest_share_BOB, eq_corr.data, x.size);
  zero_test = bool_op->XOR(zero_test, eq_corr);

  delete[] mill_inp;
  delete[] ztest_share_ALICE;
  delete[] ztest_share_BOB;

  return make_tuple(wrap, zero_test);
}

tuple<BoolArray,BoolArray> FixOp::MSB_and_zero_test(const FixArray &x) {
  assert(x.party != PUBLIC);
  BoolArray msb(x.party, x.size);
  BoolArray sub_wrap, sub_zero_test;
  tie(sub_wrap, sub_zero_test) = wrap_and_zero_test(this->reduce(x, x.ell - 1));
  for (int i = 0; i < x.size; i++) {
    msb.data[i] = (sub_wrap.data[i] ^ (x.data[i] >= (1ULL << (x.ell - 1))));
  }
  BoolArray zero_test = bool_op->AND(bool_op->NOT(msb), sub_zero_test);
  return make_tuple(msb, zero_test);
}

BoolArray FixOp::EQ(const FixArray &x, const FixArray &y) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.signed_ == y.signed_);
  assert(x.ell == y.ell);
  assert(x.s == y.s);

  BoolArray ret(this->party, x.size);
  FixArray diff = this->sub(x, y);

  if (diff.party == BOB) {
    uint64_t ell_mask_ = diff.ell_mask();
    for (int i = 0; i < diff.size; i++) {
      diff.data[i] = (-1 * diff.data[i]) & ell_mask_;
    }
  }
  eq->check_equality(ret.data, diff.data, diff.size, diff.ell);

  return ret;
}

BoolArray FixOp::LT(const FixArray &x, const FixArray &y) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.signed_ == y.signed_);
  assert(x.ell == y.ell);
  assert(x.s == y.s);

  BoolArray ret(this->party, x.size);
  FixArray diff = this->sub(x, y);
  aux->MSB(diff.data, ret.data, x.size, diff.ell);

  return ret;
}

BoolArray FixOp::GT(const FixArray &x, const FixArray &y) {
  return this->LT(y, x);
}

BoolArray FixOp::LE(const FixArray &x, const FixArray &y) {
  return bool_op->NOT(this->LT(y, x));
}

BoolArray FixOp::GE(const FixArray &x, const FixArray &y) {
  return bool_op->NOT(this->LT(x, y));
}

BoolArray FixOp::EQ(const FixArray &x, uint64_t y) {
  assert(x.party != PUBLIC);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->EQ(x, y_fix);
}

BoolArray FixOp::GT(const FixArray &x, uint64_t y) {
  assert(x.party != PUBLIC);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->GT(x, y_fix);
}

BoolArray FixOp::LT(const FixArray &x, uint64_t y) {
  assert(x.party != PUBLIC);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->LT(x, y_fix);
}

BoolArray FixOp::LE(const FixArray &x, uint64_t y) {
  assert(x.party != PUBLIC);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->LE(x, y_fix);
}

BoolArray FixOp::GE(const FixArray &x, uint64_t y) {
  assert(x.party != PUBLIC);
  FixArray y_fix = this->input(PUBLIC, x.size, y, x.signed_, x.ell, x.s);
  return this->GE(x, y_fix);
}

tuple<BoolArray,BoolArray> FixOp::LT_and_EQ(const FixArray &x, const FixArray &y) {
  assert(x.party != PUBLIC || y.party != PUBLIC);
  assert(x.size == y.size);
  assert(x.signed_ == y.signed_);
  assert(x.ell == y.ell);
  assert(x.s == y.s);

  FixArray diff = this->sub(x, y);
  return MSB_and_zero_test(diff);
}

FixArray FixOp::LUT(const vector<uint64_t> &spec_vec, const FixArray &x,
        bool signed_, int l_out, int s_out, int l_in) {
  assert(x.party != PUBLIC);
  assert(x.signed_ == false);
  assert(l_out < 64);
  assert(spec_vec.size() == (1 << l_in));
  assert(l_in <= x.ell);
  if (l_in > 8) {
    assert(l_in <= 14);
    int l_rem = l_in - 8;
    int n = 1 << l_rem;
    FixArray x_red = this->reduce(x, 8);
    vector<vector<uint64_t>> lspec_vec(n);
    for (int i = 0; i < n; i++) {
      lspec_vec[i].resize(1 << 8);
      for (int j = 0; j < (1 << 8); j++) {
        lspec_vec[i][j] = spec_vec[i*(1 << 8) + j];
      }
    }
    vector<FixArray> lout(1 << l_rem);
    for (int i = 0; i < (1 << l_rem); i++) {
      lout[i] = this->LUT(lspec_vec[i], x_red, signed_, l_out, s_out, 8);
    }
    FixArray x_hi = this->reduce(this->truncate_reduce(x, 8), l_rem);
    uint64_t x_mask = x_hi.ell_mask();
    uint64_t LUT_out_mask = (1ULL << n) - 1;
    uint64_t *LUT_out = new uint64_t[x.size];
    if (party == ALICE) {
      uint64_t **spec;
      spec = new uint64_t *[x.size];
      PRG128 prg;
      prg.random_data(LUT_out, x.size * sizeof(uint64_t));
      for (int i = 0; i < x.size; i++) {
        spec[i] = new uint64_t[1 << l_rem];
        LUT_out[i] &= LUT_out_mask;
        for (int j = 0; j < (1 << l_rem); j++) {
          int idx = (x_hi.data[i] + j) & x_mask;
          vector<uint8_t> spec_active_interval(n, 0);
          spec_active_interval[idx] = 1;
          uint64_t spec_data = 0;
          uint64_t LUT_out_data = LUT_out[i];
          for (int k = 0; k < n; k++) {
            spec_data |= (((spec_active_interval[k] ^ LUT_out_data) & 1) << k);
            LUT_out_data >>= 1;
          }
          spec[i][j] = spec_data;
        }
      }
      fix->aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, l_rem, n);

      for (int i = 0; i < x.size; i++)
        delete[] spec[i];
      delete[] spec;
    } else {
      fix->aux->lookup_table<uint64_t>(nullptr, x_hi.data, LUT_out, x.size, l_rem, n);
    }
    uint8_t *v = new uint8_t[x.size * n];
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < x.size; j++) {
        v[i * x.size + j] = LUT_out[j] & 1;
        LUT_out[j] >>= 1;
      }
    }
    vector<BoolArray> v_bl(n);
    vector<FixArray> lout_v(n);
    FixArray zero = fix->input(PUBLIC, x.size, (uint64_t)0ULL, signed_, l_out, s_out);
    for(int i = 0; i < n; i++) {
      v_bl[i] = bool_op->input(this->party, x.size, v + i*x.size);
      lout_v[i] = fix->if_else(v_bl[i], lout[i], zero);
    }
    FixArray ret(x.party, x.size, signed_, l_out, s_out);
    memset(ret.data, 0, ret.size*sizeof(uint64_t));
    for(int i = 0; i < n; i++) {
      ret = fix->add(ret, lout_v[i]);
    }
    delete[] v;
    delete[] LUT_out;
    return ret;
    /*
    FixArray ret_send(x.party, x.size, signed_, l_out, s_out);
    FixArray ret_recv(x.party, x.size, signed_, l_out, s_out);
    FixArray x_hi = this->reduce(this->truncate_reduce(x, 8), l_rem);
    uint64_t x_mask = x_hi.ell_mask();
    uint64_t ret_mask = ret_send.ell_mask();
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(ret_send.data, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[1 << l_rem];
      ret_send.data[i] &= ret_mask;
      for (int j = 0; j < (1 << l_rem); j++) {
        int idx = (x_hi.data[i] + j) & x_mask;
        spec[i][j] = (lout[idx].data[i] - ret_send.data[i]) & ret_mask;
      }
    }
    if (party == sci::ALICE) {
      aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, l_rem, l_out);
      aux->party = sci::BOB;
      aux->lookup_table<uint64_t>(nullptr, x_hi.data, ret_recv.data, x.size, l_rem, l_out);
      aux->party = sci::ALICE;
    } else { // party == sci::BOB
      aux->lookup_table<uint64_t>(nullptr, x_hi.data, ret_recv.data, x.size, l_rem, l_out);
      aux->party = sci::ALICE;
      aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, l_rem, l_out);
      aux->party = sci::BOB;
    }
    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;

    return fix->add(ret_send, ret_recv);
    */
  }
  assert(l_in <= 8);
  FixArray ret(x.party, x.size, signed_, l_out, s_out);
  FixArray x_red = this->reduce(x, l_in);
  uint64_t x_mask = x_red.ell_mask();
  uint64_t ret_mask = ret.ell_mask();
  if (party == ALICE) {
    uint64_t **spec;
    spec = new uint64_t *[x.size];
    PRG128 prg;
    prg.random_data(ret.data, x.size * sizeof(uint64_t));
    for (int i = 0; i < x.size; i++) {
      spec[i] = new uint64_t[spec_vec.size()];
      ret.data[i] &= ret_mask;
      for (int j = 0; j < spec_vec.size(); j++) {
        int idx = (x_red.data[i] + j) & x_mask;
        spec[i][j] = (spec_vec[idx] - ret.data[i]) & ret_mask;
      }
    }
    aux->lookup_table<uint64_t>(spec, nullptr, nullptr, x.size, l_in, l_out);

    for (int i = 0; i < x.size; i++)
      delete[] spec[i];
    delete[] spec;
  } else {
    aux->lookup_table<uint64_t>(nullptr, x_red.data, ret.data, x.size, l_in,
                                l_out);
  }
  return ret;
}

vector<FixArray> FixOp::digit_decomposition(const FixArray& x, int digit_size) {
  assert(x.party != PUBLIC);
  assert(digit_size <= 8);
  int num_digits = ceil(x.ell/double(digit_size));
  vector<FixArray> digits(num_digits);
  for (int i = 0; i < num_digits; i++) {
      int digit_ell = (i == (num_digits - 1) ? x.ell - i*digit_size : digit_size);
      int digit_s = x.s - i*digit_size;
      digits[i] = FixArray(x.party, x.size, false, digit_ell, digit_s);
  }
  uint64_t* digits_data = new uint64_t[num_digits * x.size];
  aux->digit_decomposition_sci(x.size, x.data, digits_data, x.ell, digit_size);
  for (int i = 0; i < num_digits; i++) {
    memcpy(digits[i].data, digits_data + i*x.size, x.size * sizeof(uint64_t));
  }
  delete[] digits_data;
  return digits;
}

vector<FixArray> FixOp::msnzb_one_hot(const FixArray& x, int ell) {
  assert(x.party != PUBLIC);
  assert(ell <= 64);
  uint8_t *x_one_hot = new uint8_t[x.size * x.ell];
  uint64_t *x_one_hot_64 = new uint64_t[x.size * x.ell];
#ifdef MSNZB_GC
  aux->msnzb_GC(x.data, x_one_hot, x.ell, x.size);
#else  // MSNZB_GC
  aux->msnzb_one_hot(x.data, x_one_hot, x.ell, x.size);
#endif // MSNZB_GC
  fix->aux->B2A(x_one_hot, x_one_hot_64, x.size * x.ell, ell);
  vector<FixArray> ret(x.ell);
  for (int i = 0; i < x.ell; i++) {
    ret[i] = FixArray(x.party, x.size, false, ell, 0);
    for (int j = 0; j < x.size; j++) {
      ret[i].data[j] = x_one_hot_64[j*x.ell + i];
    }
  }
  delete[] x_one_hot;
  delete[] x_one_hot_64;
  return ret;
}

FixArray FixOp::exp(const FixArray& x, int l_y, int s_y, int digit_size) {
  assert(x.party != PUBLIC);
  assert(x.signed_ == true);
  assert(l_y >= (s_y + 2));
  assert(digit_size <= 8);

  FixArray pos_x = this->mul(x, -1);
  pos_x.signed_ = false; // pos_x is unsigned

  vector<FixArray> digits = this->digit_decomposition(pos_x, digit_size);
  int num_digits = digits.size();

  vector<FixArray> digits_exp(num_digits);
  for (int i = 0; i < num_digits; i++) {
    vector<uint64_t> spec(1 << digits[i].ell);
    for (int j = 0; j < (1 << digits[i].ell); j++) {
      spec[j] = std::exp(-1.0 * (j / pow(2.0, digits[i].s))) * (1ULL << s_y);
    }
    digits_exp[i] = this->LUT(spec, digits[i], true, s_y + 2, s_y);
  }
  BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
  for (int i = 1; i < num_digits; i *= 2) {
    for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i) {
      digits_exp[j] = this->mul(digits_exp[j+i], digits_exp[j],
              2*s_y + 2, all_0.data, all_0.data);
      digits_exp[j] = this->truncate_reduce(digits_exp[j], s_y);
    }
  }

  return this->extend(digits_exp[0], l_y, all_0.data);
}

FixArray FixOp::max(const vector<FixArray>& x) {
  int N = x.size();
  int n = x[0].size;
  int party = x[0].party;
  int signed_ = x[0].signed_;
  int ell = x[0].ell;
  int s = x[0].s;
  for (int i = 1; i < N; i++) {
    assert(x[i].party == party);
    assert(x[i].signed_ == signed_);
    assert(x[i].ell == ell);
    assert(x[i].s == s);
  }

  vector<FixArray> x_tr(n);
  for (int i = 0; i < n; i++) {
    x_tr[i] = FixArray(party, N, signed_, ell, s);
    for (int j = 0; j < N; j++) {
      x_tr[i].data[j] = x[j].data[i];
    }
  }
  int num_cmps_old = n; int num_cmps_curr = n/2;
  uint64_t* lhs = new uint64_t[N*num_cmps_curr];
  uint64_t* rhs = new uint64_t[N*num_cmps_curr];
  while(num_cmps_old > 1) {
    int odd_num_cmps = num_cmps_old & 1;
    for (int j = odd_num_cmps; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      memcpy(lhs + (j/2)*N, x_tr[j].data, N*sizeof(uint64_t));
      memcpy(rhs + (j/2)*N, x_tr[j + 1].data, N*sizeof(uint64_t));
    }
    FixArray lhs_fp = fix->input(this->party, N*num_cmps_curr, lhs, signed_, ell, s);
    FixArray rhs_fp = fix->input(this->party, N*num_cmps_curr, rhs, signed_, ell, s);
    BoolArray cond = fix->GT(lhs_fp, rhs_fp);
    lhs_fp = fix->if_else(cond, lhs_fp, rhs_fp);
    for (int j = 0; j < num_cmps_old && j + 1 < num_cmps_old; j += 2) {
      memcpy(x_tr[odd_num_cmps + (j/2)].data, lhs_fp.data + (j/2)*N, N*sizeof(uint64_t));
    }
    num_cmps_old = num_cmps_curr + odd_num_cmps;
    num_cmps_curr = num_cmps_old/2;
  }
  delete[] lhs;
  delete[] rhs;

  return x_tr[0];
}

// A0 \in (1/4, 1)
inline uint64_t recp_lookup_c0(uint64_t index, int m) {
  uint64_t k = 1ULL << m;
  double p = 1 + (double(index) / double(k));
  double A1 = 1.0 / (p * (p + 1.0 / double(k)));
  int32_t scale = m + 3;
  uint64_t mask = (1ULL << scale) - 1;
  uint64_t val = uint64_t(A1 * (1ULL << scale)) & mask;
  return val;
}

// A1 \in (1/2, 1)
inline uint64_t recp_lookup_c1(uint64_t index, int m) {
  uint64_t k = 1ULL << m;
  double p = 1 + (double(index) / double(k));
  double z = (p * (p + (1.0 / double(k))));
  double A1 = ((1.0 / double(k * 2)) + sqrt(z)) / z;
  int32_t scale = 2 * m + 2;
  uint64_t mask = (1ULL << scale) - 1;
  uint64_t val = uint64_t(A1 * (1ULL << scale)) & mask;
  return val;
}

FixArray FixOp::div(const FixArray& nm, const FixArray& dn, int l_out, int s_out, bool normalized_dn) {
  if (!normalized_dn) assert(dn.signed_ == false);
  assert(nm.party != PUBLIC && dn.party != PUBLIC);
  assert(nm.size == dn.size);
  assert(s_out <= dn.s);
  BoolArray all_0 = bool_op->input(ALICE, dn.size, uint8_t(0));
  BoolArray all_1 = bool_op->input(ALICE, dn.size, uint8_t(1));
  print_fix(dn);

  FixArray nrmlzd_dn;
  FixArray adjust = fix->input(PUBLIC, dn.size, uint64_t(0), false, dn.ell + 1, 0);
  if (!normalized_dn) {
    vector<FixArray> msnzb_one_hot = fix->msnzb_one_hot(dn, dn.ell + 1);
    for (int i = 0; i < dn.ell; i++) {
      adjust = fix->add(adjust, fix->mul(msnzb_one_hot[i], (1ULL << (dn.ell - 1 - i))));
    }
    adjust.s = dn.ell - 1 - dn.s;
    BoolArray msb_dn = fix->LSB(msnzb_one_hot[dn.ell - 1]);
    nrmlzd_dn = fix->mul(dn, adjust, dn.ell + 1, msb_dn.data, all_0.data);
  } else {
    if (dn.ell == dn.s + 1) {
      nrmlzd_dn = fix->extend(dn, dn.s + 2, all_1.data);
    } else {
      nrmlzd_dn = fix->reduce(dn, dn.s + 2);
    }
  }

  int32_t m, iters;
  m = (s_out <= 18 ? ceil((s_out - 2) / 2.0) : ceil((ceil(s_out / 2.0) - 2) / 2.0));
  iters = (s_out <= 18 ? 0 : 1);

  // reciprocal approximation w
  FixArray eps = fix->reduce(nrmlzd_dn, nrmlzd_dn.s - m);
  eps.signed_ = false;
  BoolArray msb_eps = fix->MSB(eps);
  uint8_t *wrap_eps = new uint8_t[dn.size];
  fix->aux->MSB_to_Wrap(eps.data, msb_eps.data, wrap_eps, eps.size, eps.ell);
  FixArray idx = fix->truncate_reduce(fix->reduce(nrmlzd_dn, nrmlzd_dn.s), nrmlzd_dn.s - m, wrap_eps);
  idx.signed_ = false;
  delete[] wrap_eps;
  vector<uint64_t> spec_c0(1 << idx.ell);
  vector<uint64_t> spec_c1(1 << idx.ell);
  for (int j = 0; j < (1 << idx.ell); j++) {
    spec_c0[j] = recp_lookup_c0(j, m);
    spec_c1[j] = recp_lookup_c1(j, m);
  }
  FixArray c0 = fix->LUT(spec_c0, idx, true, m + 4, m + 3);
  FixArray c1 = fix->LUT(spec_c1, idx, true, 2*m + 3, 2*m + 2);
  FixArray w = fix->mul(c0, eps, nrmlzd_dn.s + 4, all_0.data, msb_eps.data);
  print_fix(eps);
  print_fix(w);
  w = fix->sub(fix->scale_up(c1, nrmlzd_dn.s + m + 4, nrmlzd_dn.s + m + 3),
               fix->extend(w, nrmlzd_dn.s + m + 4, all_0.data));
  print_fix(c0);
  print_fix(c1);
  print_fix(w);
  w = fix->truncate_reduce(w, w.s - s_out);

  BoolArray msb_nm;
  uint8_t* msb_nm_data = nullptr;
  if (nm.signed_) {
    msb_nm = fix->MSB(nm);
    msb_nm_data = msb_nm.data;
  }
  FixArray a = fix->mul(nm, w, nm.ell + s_out, msb_nm_data, all_0.data);
  a = fix->truncate_reduce(a, nm.s);
  if ((nm.ell - nm.s) >= (l_out - s_out)) {
    a = fix->reduce(a, l_out);
  } else {
    a = fix->extend(a, l_out, msb_nm_data);
  }
  print_fix(a);

  if (!normalized_dn) {
    a = fix->mul(a, adjust, l_out + adjust.s, msb_nm_data, all_0.data);
    a = fix->truncate_reduce(a, adjust.s);
  }

  if (iters > 0) {
    FixArray d = fix->mul(w, nrmlzd_dn, s_out + nrmlzd_dn.s + 2, all_0.data, all_0.data);
    d = fix->truncate_reduce(d, nrmlzd_dn.s);
    FixArray e = fix->sub(1ULL << d.s, d);
    e.signed_ = true;

    FixArray a_curr, e_curr;
    FixArray a_prev = a, e_prev = e;
    for (int i = 0; i < iters - 1; i++) {
      e_curr = fix->mul(e_prev, e_prev, 2*s_out + 2, all_0.data, all_0.data);
      e_curr = fix->truncate_reduce(e_curr, s_out);
      e_prev = fix->add(e_prev, 1ULL << e_prev.s);
      a_curr = fix->mul(e_prev, a_prev, l_out + s_out, all_0.data, msb_nm_data);
      a_curr = fix->truncate_reduce(a_curr, s_out);
      a_prev = a_curr;
      e_prev = e_curr;
    }
    e_prev = fix->add(e_prev, 1ULL << e_prev.s);
    FixArray out = fix->mul(e_prev, a_prev, l_out + s_out, all_0.data, msb_nm_data);
    out = fix->truncate_reduce(out, s_out);
    return out;
  } else {
    return a;
  }
}

FixArray FixOp::sigmoid(const FixArray& x, int l_y, int s_y) {
  assert(x.party != PUBLIC);
  assert(x.signed_ == true);

  BoolArray msb_x = fix->MSB(x);
  FixArray neg_x = fix->if_else(msb_x, x, fix->mul(x, -1));

  FixArray exp_neg_x = fix->exp(neg_x, s_y + 2, s_y);
  FixArray dn = fix->add(exp_neg_x, 1ULL << exp_neg_x.s);

  // if dn.data == 2 * 2^{dn.s}, subtract 1 from dn.data to ensure normalized form of dn
  BoolArray dn_eq_2 = fix->EQ(dn, 2ULL << dn.s);
  dn = fix->if_else(dn_eq_2, fix->sub(dn, 1), dn);

  // setting one_dn as secret-shared as div expects a secret-share numerator
  FixArray one_dn = fix->input(ALICE, x.size, 1, true, 2, 0);
  FixArray inv_dn = fix->div(one_dn, dn, s_y + 2, s_y, true);

  FixArray one_nm = fix->input(PUBLIC, x.size, 1ULL << exp_neg_x.s, true, exp_neg_x.ell, exp_neg_x.s);
  FixArray nm = fix->if_else(msb_x, exp_neg_x, one_nm);

  BoolArray all_0 = bool_op->input(ALICE, dn.size, uint8_t(0));
  FixArray ret = fix->mul(nm, inv_dn, 2*s_y + 2, all_0.data, all_0.data);
  ret = fix->truncate_reduce(ret, s_y);
  if (l_y >= s_y + 2) {
    ret = fix->extend(ret, l_y, all_0.data);
  } else {
    ret = fix->reduce(ret, l_y);
  }
  return ret;
}

FixArray FixOp::tanh(const FixArray& x, int l_y, int s_y) {
  assert(x.party != PUBLIC);
  assert(x.signed_ == true);

  BoolArray msb_x = fix->MSB(x);
  FixArray neg_x = fix->if_else(msb_x, x, fix->mul(x, -1));
  // get neg_2x from neg_x by reducing scale by 1
  FixArray neg_2x = neg_x;
  neg_2x.s -= 1;

  FixArray exp_neg_2x = fix->exp(neg_2x, s_y + 2, s_y);
  FixArray nm = fix->sub(1ULL << exp_neg_2x.s, exp_neg_2x);
  FixArray dn = fix->add(exp_neg_2x, 1ULL << exp_neg_2x.s);
  // if dn.data == 2 * 2^{dn.s}, subtract 1 from dn.data to ensure normalized form of dn
  BoolArray dn_eq_2 = fix->EQ(dn, 2ULL << dn.s);
  dn = fix->if_else(dn_eq_2, fix->sub(dn, 1), dn);

  FixArray tanh_neg_x = fix->div(nm, dn, s_y + 2, s_y, true);

  FixArray ret = fix->if_else(msb_x, fix->mul(tanh_neg_x, -1), tanh_neg_x);
  if (l_y >= s_y + 2) {
    ret = fix->extend(ret, l_y, msb_x.data);
  } else {
    ret = fix->reduce(ret, l_y);
  }
  return ret;
}

FixArray FixOp::sqrt(const FixArray& x, int l_y, int s_y, bool recp_sqrt) {
  assert(x.party != PUBLIC);
}
