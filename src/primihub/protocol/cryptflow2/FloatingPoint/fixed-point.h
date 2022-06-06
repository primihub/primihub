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

#ifndef FIXED_POINT_H__
#define FIXED_POINT_H__

#include "FloatingPoint/bool-data.h"
#include "Math/math-functions.h"
#include <tuple>

// #define MSNZB_GC

#define print_fix(vec)                                                         \
  {                                                                            \
    auto tmp_pub = fix->output(PUBLIC, vec).subset(I, I + J);                  \
    cout << #vec << "_pub: " << tmp_pub << endl;                               \
  }

// A container to hold an array of fixed-point values
// If party is set as PUBLIC for a FixArray instance, then the underlying array is known publicly and we maintain the invariant that both parties will hold identical data in that instance.
// Else, the underlying array is secret-shared and the class instance will hold the party's share of the secret array. In this case, the party data member denotes which party this share belongs to.
// signed_ denotes the signedness, ell is the bitlength, and s is the scale of the underlying fixed-point array
// If s is set to 0, the FixArray will behave like an IntegerArray
class FixArray {
public:
  int party = sci::PUBLIC;
  int size = 0;             // size of array
  uint64_t *data = nullptr; // data (ell-bit integers)
  bool signed_;             // signed? (1: signed; 0: unsigned)
  int ell;                  // bitlength
  int s;                    // scale

  FixArray(){};

  FixArray(int party_, int sz, bool signed__, int ell_, int s_ = 0) {
    assert(party_ == sci::PUBLIC || party_ == sci::ALICE || party_ == sci::BOB);
    assert(sz > 0);
    assert(ell_ <= 64 && ell_ > 0);
    this->party = party_;
    this->size = sz;
    this->signed_ = signed__;
    this->ell = ell_;
    this->s = s_;
    data = new uint64_t[sz];
  }

  // copy constructor
  FixArray(const FixArray &other) {
    this->party = other.party;
    this->size = other.size;
    this->signed_ = other.signed_;
    this->ell = other.ell;
    this->s = other.s;
    this->data = new uint64_t[size];
    memcpy(this->data, other.data, size * sizeof(uint64_t));
  }

  // move constructor
  FixArray(FixArray &&other) noexcept {
    this->party = other.party;
    this->size = other.size;
    this->signed_ = other.signed_;
    this->ell = other.ell;
    this->s = other.s;
    this->data = other.data;
    other.data = nullptr;
  }

  ~FixArray() { delete[] data; }

  template <class T> std::vector<T> get_native_type();

  // copy assignment
  FixArray &operator=(const FixArray &other) {
    if (this == &other) return *this;

    delete[] this->data;
    this->party = other.party;
    this->size = other.size;
    this->signed_ = other.signed_;
    this->ell = other.ell;
    this->s = other.s;
    this->data = new uint64_t[size];
    memcpy(this->data, other.data, size * sizeof(uint64_t));
    return *this;
  }

  // move assignment
  FixArray &operator=(FixArray &&other) noexcept {
    if (this == &other) return *this;

    delete[] this->data;
    this->party = other.party;
    this->size = other.size;
    this->signed_ = other.signed_;
    this->ell = other.ell;
    this->s = other.s;
    this->data = other.data;
    other.data = nullptr;
    return *this;
  }

  // FixArray[i, j)
  FixArray subset(int i, int j);

  uint64_t ell_mask() const { return (1ULL << (this->ell)) - 1; }
};

std::ostream &operator<<(std::ostream &os, FixArray &other);

FixArray concat(const vector<FixArray>& x);

class FixOp {
public:
  int party;
  sci::IOPack *iopack;
  sci::OTPack *otpack;
  Equality *eq;
  MillionaireWithEquality *mill_eq;
  AuxProtocols *aux;
  XTProtocol *xt;
  Truncation *trunc;
  LinearOT *mult;
  BoolOp *bool_op;
  FixOp *fix;

  FixOp(int party, sci::IOPack *iopack, sci::OTPack *otpack) {
    this->party = party;
    this->iopack = iopack;
    this->otpack = otpack;
    this->aux = new AuxProtocols(party, iopack, otpack);
    this->eq = new Equality(party, iopack, otpack);
    this->mill_eq = new MillionaireWithEquality(party, iopack, otpack);
    this->xt = new XTProtocol(party, iopack, otpack);
    this->trunc = new Truncation(party, iopack, otpack);
    this->mult = new LinearOT(party, iopack, otpack);
    this->bool_op = new BoolOp(party, iopack, otpack);
    this->fix = this;
  }

  ~FixOp() {
    delete aux;
    delete eq;
    delete mill_eq;
    delete xt;
    delete trunc;
    delete mult;
    delete bool_op;
  }

  // input functions: return a FixArray that stores data_
  // party_ denotes which party provides the input data_ and the data_ provided by the other party is ignored. If party_ is PUBLIC, then the data_ provided by both parties must be identical.
  // sz is the size of the returned FixArray and the uint64_t array pointed by data_
  // signed__, ell_, and s_ are the signedness, bitlength and scale of the input, respectively
  FixArray input(int party_, int sz, uint64_t* data_, bool signed__, int ell_, int s_ = 0);
  // same as the above function, except that it replicates data_ in all sz positions of the returned FixArray
  FixArray input(int party_, int sz, uint64_t data_, bool signed__, int ell_, int s_ = 0);

  // output function: returns the secret array underlying x in the form of a PUBLIC FixArray
  // party_ denotes which party will receive the output. If party_ is PUBLIC, both parties receive the output.
  FixArray output(int party_, const FixArray& x);

  // Multiplexers: return x[i] if cond[i] = 1; else return y[i]
  // cond must be a secret-shared BoolArray
  //// Both x and y can be PUBLIC or secret-shared
  //// cond, x, y must have equal size
  //// x, y must have same signedness, bitlength and scale
  FixArray if_else(const BoolArray &cond, const FixArray &x, const FixArray &y);
  //// x can be PUBLIC or secret-shared
  //// cond, x must have equal size
  //// y[i] = y (with same signedness, bitlength and scale as x)
  FixArray if_else(const BoolArray &cond, const FixArray &x, uint64_t y);
  //// y can be PUBLIC or secret-shared
  //// cond, y must have equal size
  //// x[i] = x (with same signedness, bitlength and scale as y)
  FixArray if_else(const BoolArray &cond, uint64_t x, const FixArray &y);

  // Add Operations: return x[i] + y[i]
  // The output has same signedness, bitlength and scale as x, y
  //// Both x and y can be PUBLIC or secret-shared
  //// x and y must have equal size
  //// x, y must have same signedness, bitlength and scale
  FixArray add(const FixArray &x, const FixArray &y);
  //// x can be PUBLIC or secret-shared
  //// y[i] = y (with same signedness, bitlength and scale as x)
  FixArray add(const FixArray &x, uint64_t y);

  // Sub Operations: return x[i] - y[i]
  // The output has same signedness, bitlength and scale as x, y
  //// Both x and y can be PUBLIC or secret-shared
  //// x and y must have equal size
  //// x, y must have same signedness, bitlength and scale
  FixArray sub(const FixArray &x, const FixArray &y);
  //// x can be PUBLIC or secret-shared
  //// y[i] = y (with same signedness, bitlength and scale as x)
  FixArray sub(const FixArray &x, uint64_t y);
  //// y can be PUBLIC or secret-shared
  //// x[i] = x (with same signedness, bitlength and scale as y)
  FixArray sub(uint64_t x, const FixArray &y);

  // Extension: returns a FixArray that holds the same fixed-point values as x, except in larger bitlength ell
  // The signedness and scale of the output are same as x
  // x can be PUBLIC or secret-shared
  // ell should be greater than or equal to bitlength of x
  // msb_x is an optional parameter that points to an array holding boolean shares of most significant bit (MSB) of x[i]'s. If msb_x provided, this operation is cheaper when x is secret-shared.
  FixArray extend(const FixArray &x, int ell, uint8_t *msb_x = nullptr);

  // Boolean-to-Arithmetic Operation: returns a FixArray with bitlength ell that holds the same boolean values as x
  // The signedness of the output is set as signed_ and the scale is set as 0
  // x can be PUBLIC or secret-shared
  // ell should be greater than 1
  FixArray B2A(const BoolArray &x, bool signed_, int ell);

  // Multiplication Operation: return x[i] * y[i] (in ell bits)
  // ell specifies the output bitlength
  // Signedness of the output is the same as x and scale is equal to sum of scales of x and y
  // msb_x is an optional parameter that points to an array holding boolean shares of most significant bit (MSB) of x[i]'s. If msb_x provided, this operation is cheaper when x is secret-shared.
  //// At least one of x and y must be a secret-shared FixArray
  //// x and y must have equal size
  //// Either signedness of x and y is same, or x is signed (x.signed_ = 1)
  //// ell >= bitlengths of x and y and ell <= sum of bitlengths of x and y (x.ell + y.ell)
  //// msb_y is similar to msb_x but for y
  FixArray mul(const FixArray &x, const FixArray &y, int ell,
               uint8_t *msb_x = nullptr, uint8_t *msb_y = nullptr);
  //// x can be PUBLIC or secret-shared
  //// y[i] = y (with same signedness as x; bitlength is ell and scale is 0)
  //// ell >= bitlength of x
  FixArray mul(const FixArray &x, uint64_t y, int ell,
               uint8_t *msb_x = nullptr);
  // same as above mul functions except ell is a constant depending on bitlengths of inputs
  //// ell = sum of bitlengths of x and y (x.ell + y.ell)
  inline FixArray mul(const FixArray &x, const FixArray &y,
          uint8_t *msb_x = nullptr, uint8_t *msb_y = nullptr) {
      return mul(x, y, x.ell + y.ell, msb_x, msb_y);
  }
  //// ell = bitlength of x (x.ell)
  inline FixArray mul(const FixArray &x, uint64_t y, uint8_t *msb_x = nullptr) {
      return mul(x, y, x.ell, msb_x);
  }

  // Left Shift: returns x[i] << s[i] (in ell bits)
  // Output bitlength is ell, output signedness and scale are same as that of x
  // bound is the (closed) upper bound on integer values in s
  // Both x and s must be secret shared FixArray and of equal size
  // ell <= bitlength of x (x.ell) + bound and ell is >= both x.ell and bound
  // s must be an unsigned FixArray with scale 0 and bitlength >= ceil(log2(bound))
  // msb_x is an optional parameter that points to an array holding boolean shares of most significant bit (MSB) of x[i]'s. If msb_x provided, this operation is cheaper
  FixArray left_shift(const FixArray &x, const FixArray &s, int ell, int bound,
                      uint8_t *msb_x = nullptr);

  // Right Shift: returns x[i] >> s[i]
  // Output bitlength, signedness and scale are same as that of x
  // bound is the (closed) upper bound on integer values in s
  // Both x and s must be secret shared FixArray and of equal size
  // bound <= bitlength of x (x.ell) and bound + x.ell < 64
  // s must be an unsigned FixArray with scale 0 and bitlength >= ceil(log2(bound))
  // msb_x is an optional parameter that points to an array holding boolean shares of most significant bit (MSB) of x[i]'s. If msb_x provided, this operation is cheaper
  FixArray right_shift(const FixArray &x, const FixArray &s, int bound,
                       uint8_t *msb_x = nullptr);

  // Right Shift: returns x[i] >> s
  // Output scale is x.s - s; Output bitlength and signedness are same as that of x
  // x must be secret shared FixArray
  // s <= bitlength of x (x.ell) and s >= 0
  // msb_x is an optional parameter that points to an array holding boolean shares of most significant bit (MSB) of x[i]'s. If msb_x provided, this operation is cheaper
  FixArray right_shift(const FixArray &x, int s, uint8_t *msb_x = nullptr);

  // Truncate and Reduce: returns x[i] >> s mod 2^{x.ell - s}
  // Output bitlength and scale are x.ell-s and x.s-s; Output signedness is same as that of x
  // x must be secret shared FixArray
  // s < bitlength of x (x.ell) and s >= 0
  // wrap_x_s is an optional parameter that points to an array holding boolean shares of the wrap-bit of lower s bits of x[i]'s (i.e., wrap_x_s[i] = 1{ share(1, x[i]) mod 2^{s} + share(2, x[i]) mod 2^{s} >= 2^{s} }). If wrap_x_s is provided, this operation is cheaper
  FixArray truncate_reduce(const FixArray &x, int s,
                           uint8_t *wrap_x_s = nullptr);

  // Round Nearest: returns (x[i] + 2^(s-1)) >> s mod 2^{x.ell - s}
  // Output bitlength and scale are x.ell-s and x.s-s; Output signedness is same as that of x
  // x must be secret shared FixArray
  // s < bitlength of x (x.ell) and s >= 0
  FixArray round_nearest(const FixArray &x, int s) {
    FixArray y = fix->add(x, 1ULL << (s-1));
    return truncate_reduce(y, s);
  }

  // Truncate with Sticky-Bit: returns (x[i] >> s) mod 2^{x.ell-s} if x[i] mod 2^{s} = 0 and returns ((x[i] >> s) | 1) mod 2^{x.ell-s} otherwise
  // Output bitlength and scale are x.ell-s and x.s-s; Output signedness is same as that of x
  // x must be secret shared FixArray
  // s < bitlength of x (x.ell) and s >= 0
  FixArray truncate_with_sticky_bit(const FixArray &x, int s);

  // Round (Ties to Even): returns (x[i] >> s) mod 2^{x.ell-s} if x[i] mod 2^{s} < 2^{s-1} or x[i] mod 2^{s+1} = 2^{s-1}, and returns ((x[i] >> s) + 1) mod 2^{x.ell-s} otherwise
  // Output bitlength and scale are x.ell-s and x.s-s; Output signedness is same as that of x
  // x must be secret shared FixArray
  // s <= bitlength of x (x.ell) and s >= 2
  // TODO: Improve this protocol: perform wrap_and_zero_test to get wrap_x_s and eq_x_s, then do truncate_reduce(x[i] + 2^{s-1}, s, wrap_x_s) and finally, subtract 1 from output out if eq_x_s & (out mod 2) = 1
  FixArray round_ties_to_even(const FixArray &x, int s);

  // Scale-Up: returns (x[i] << (s - x.s)) mod 2^{ell}
  // Output bitlength and scale are ell and s, and output signedness is same as that of x
  // x can be PUBLIC or secret-shared
  // s >= x.s and ell <= x.ell + (s - x.s)
  FixArray scale_up(const FixArray &x, int ell, int s);

  // (Modulo) Reduce: returns x[i] mod 2^{ell}
  // Output bitwidth is ell, and output scale and signedness are same as that of x
  // x can be PUBLIC or secret-shared
  // ell <= x.ell and ell > 0
  FixArray reduce(const FixArray &x, int ell);

  // Least Significant Bit: returns x[i] mod 2 in the form of BoolArray
  // x can be PUBLIC or secret-shared
  BoolArray LSB(const FixArray &x);

  // Most Significant Bit: returns MSB of x in the form of BoolArray
  // x must be secret-shared
  BoolArray MSB(const FixArray &x) {
      assert(x.party != sci::PUBLIC);
      return fix->LT(x, 0);
  }

  // Wrap Bit and Zero-test Bit: returns 1{ share(1, x[i]) + share(2, x[i]) >= 2^{x.ell} } and 1{ x[i] = 0 } in the form of BoolArrays
  // x must be secret-shared
  std::tuple<BoolArray,BoolArray> wrap_and_zero_test(const FixArray &x);
  // Most Significant Bit and Zero-test Bit: returns 1{ x[i] >= 2^{x.ell-1} } and  1{ x[i] = 0 } in the form of BoolArrays
  // x must be secret-shared
  std::tuple<BoolArray,BoolArray> MSB_and_zero_test(const FixArray &x);

  // Equality and Comparison Operations: return (x[i]-y[i] mod 2^{x.ell}) OP 0 (OP = {=, <, >, <=, >=}) in the form of a BoolArray, where the comparison is over signed integers
  // The comparison operations have correctness for signed inputs if | x[i] | + | y[i] | < 2^{x.ell-1}, and for unsigned inputs if | x[i]-y[i] | < 2^{x.ell-1}
  //// At least one of x and y must be a secret-shared FixArray
  //// x, y must have same size, signedness, bitlength and scale
  BoolArray EQ(const FixArray &x, const FixArray &y);
  BoolArray LT(const FixArray &x, const FixArray &y);
  BoolArray GT(const FixArray &x, const FixArray &y);
  BoolArray LE(const FixArray &x, const FixArray &y);
  BoolArray GE(const FixArray &x, const FixArray &y);
  //// x must be secret-shared
  //// y[i] = y (with same signedness, bitlength and scale as x)
  BoolArray EQ(const FixArray &x, uint64_t y);
  BoolArray LT(const FixArray &x, uint64_t y);
  BoolArray GT(const FixArray &x, uint64_t y);
  BoolArray LE(const FixArray &x, uint64_t y);
  BoolArray GE(const FixArray &x, uint64_t y);

  // Less-Than Bit and Equal-To Bit: returns LT(x, y) and EQ(x, y) but has better cost than the sum of their costs
  // At least one of x and y must be a secret-shared FixArray
  // x, y must have same size, signedness, bitlength and scale
  std::tuple<BoolArray,BoolArray> LT_and_EQ(const FixArray &x, const FixArray &y);

  // Lookup Table (LUT): returns spec_vec[x[i] mod 2^{l_in}] mod 2^{l_out}
  // Output has bitlength l_out, scale s_out, and is signed if signed_ = true
  // x must be secret-shared and an unsigned FixArray
  // l_out < 64, l_in <= 8, and l_in <= x.ell
  // spec_vec.size() must be equal to 2^{l_in}
  FixArray LUT(const std::vector<uint64_t> &spec_vec, const FixArray &x,
               bool signed_, int l_out, int s_out, int l_in);
  // same as above LUT function except l_in is same as x.ell
  inline FixArray LUT(const std::vector<uint64_t> &spec_vec, const FixArray &x,
          bool signed_, int l_out, int s_out) {
    return this->LUT(spec_vec, x, signed_, l_out, s_out, x.ell);
  }

  // Digit Decomposition: returns { x_i }_{i \in [d]}, where x = x_0 || ... || x_{d-1} (x_0 has the LSBs) and d = ceil(x.ell/digit_size)
  // i-th output digit is unsigned and has scale x.s - i*digit_size
  // All output digits have bitlength digit_size except the last digit which has bitlength x.ell - (d-1)*digit_size
  // x must be secret-shared and digit_size should be <= 8
  std::vector<FixArray> digit_decomposition(const FixArray& x, int digit_size);

  // Most Significant Non-Zero Bit (MSNZB): return { z_i }_{i \in {0, ..., x.ell - 1}}, where z_i = 1 if 2^i <= x < 2^{i+1}
  // The output is an x.ell sized vector of unsigned FixArrays of bitlength ell and scale 0
  std::vector<FixArray> msnzb_one_hot(const FixArray& x, int ell);

  // Finds max element in x[i], forall i
  FixArray max(const std::vector<FixArray>& x);

  // SIRNN's math functions

  // Exponentiation: returns y = e^{-x} in an l_y-bit fixed-point representation with scale s_y
  // x must be secret-shared and signed, and l_y should be >= s_y + 2
  // digit_size is an optional parameter that should be <= 8. This parameter affects both efficiency and precision (the larger the better)
  FixArray exp(const FixArray& x, int l_y, int s_y, int digit_size = 8);

  // Division: return out = nm / dn in an l_out fixed-point representation with scale s_out
  // nm and dn must be secret-shared, and s_out should be <= dn.s
  // if normalized_dn is true, it is assumed that dn \in [1,2)
  FixArray div(const FixArray& nm, const FixArray& dn, int l_out, int s_out, bool normalized_dn = false);

  FixArray sqrt(const FixArray& x, int l_y, int s_y, bool recp_sqrt = false);

  FixArray sigmoid(const FixArray& x, int l_y, int s_y);

  FixArray tanh(const FixArray& x, int l_y, int s_y);

};

#endif // FIXED_POINT_H__
