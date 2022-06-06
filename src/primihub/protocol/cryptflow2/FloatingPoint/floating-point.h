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

#ifndef FLOATING_POINT_H__
#define FLOATING_POINT_H__

#include "FloatingPoint/fixed-point.h"
#include "Math/math-functions.h"
#include <iostream>

#define FP32_M_BITS 23
#define FP32_E_BITS 8

#define print_fp(vec)                                                          \
  {                                                                            \
    auto tmp_pub = fp_op->output(PUBLIC, vec).subset(I, I + J);                \
    cout << #vec << "_pub: " << tmp_pub << endl;                               \
  }

// A container to hold an array of floating-point values
// If party is set as PUBLIC for a FPArray instance, then the underlying array is known publicly and we maintain the invariant that both parties will hold identical data in that instance.
// Else, the underlying array is secret-shared and the class instance will hold the party's share of the secret array. In this case, the party data member denotes which party this share belongs to.
// m_bits denotes the mantissa bits (after the binary point; e.g. 23 for 32-bit floats) and e_bits denotes the exponent bits
// For efficiency reasons, we maintain the sign-bit (s; BoolArray), exponent (e; FixArray with ell = e_bits + 2 and s = 0) and mantissa (m; FixArray with ell = m_bits + 1 and s = m_bits) in different secret-shared arrays, and additionally maintain a zero-bit (z; BoolArray)
class FPArray {
public:
  int party = sci::PUBLIC;
  int size = 0;          // size of array
  uint8_t *s = nullptr;  // sign
  uint8_t *z = nullptr;  // is_zero?
  uint64_t *m = nullptr; // mantissa
  uint64_t *e = nullptr; // exponent
  uint8_t m_bits; // mantissa bits (after binary point)
  uint8_t e_bits; // exponent bits

  FPArray(){};

  FPArray(int party_, int sz, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS) {
    assert(party_ == sci::PUBLIC || party_ == sci::ALICE || party_ == sci::BOB);
    assert(sz > 0);
    assert(m_bits_ > 0);
    assert(e_bits_ > 0);
    this->party = party_;
    this->size = sz;
    this->m_bits = m_bits_;
    this->e_bits = e_bits_;
    s = new uint8_t[sz];
    z = new uint8_t[sz];
    m = new uint64_t[sz];
    e = new uint64_t[sz];
  }

  // copy constructor
  FPArray(const FPArray &other) {
    this->party = other.party;
    this->size = other.size;
    this->m_bits = other.m_bits;
    this->e_bits = other.e_bits;
    this->s = new uint8_t[size];
    this->z = new uint8_t[size];
    this->m = new uint64_t[size];
    this->e = new uint64_t[size];
    memcpy(this->s, other.s, size * sizeof(uint8_t));
    memcpy(this->z, other.z, size * sizeof(uint8_t));
    memcpy(this->m, other.m, size * sizeof(uint64_t));
    memcpy(this->e, other.e, size * sizeof(uint64_t));
  }

  // move constructor
  FPArray(FPArray &&other) noexcept {
    this->party = other.party;
    this->size = other.size;
    this->m_bits = other.m_bits;
    this->e_bits = other.e_bits;
    this->s = other.s;
    this->z = other.z;
    this->m = other.m;
    this->e = other.e;
    other.s = nullptr;
    other.z = nullptr;
    other.m = nullptr;
    other.e = nullptr;
  }

  ~FPArray() {
    delete[] s;
    delete[] z;
    delete[] m;
    delete[] e;
  }

  template <class T> std::vector<T> get_native_type();

  // if the input floating-point value overflows/underflows the representation, set it as inf/0
  void check_bounds() {
    for (int i = 0; i < size; i++) {
      if (z[i] || (int64_t(e[i]) - int64_t(e_min()) < 0)) {
        // 0 representation
        e[i] = 0;
        m[i] = 0;
      }
      if (int64_t(e[i]) - int64_t(e_max()) > 0) {
        // inf representation
        e[i] = e_max() + 1;
        m[i] = 0;
      }
    }
  }

  // modulo mask for m
  uint64_t m_mask() const {
    return (1ULL << (this->m_bits + 1)) - 1;
  }

  // modulo mask for e
  uint64_t e_mask() const {
    return (uint64_t(1) << (this->e_bits + 2)) - 1;
  }

  uint64_t e_bias() const { return (uint64_t(1) << (this->e_bits - 1)) - 1; }

  uint64_t e_min() const { return 1; }

  uint64_t e_max() const { return 2 * e_bias(); }

  // copy assignment
  FPArray &operator=(const FPArray &other) {
    if (this == &other) return *this;

    delete[] this->s;
    delete[] this->z;
    delete[] this->m;
    delete[] this->e;
    this->party = other.party;
    this->size = other.size;
    this->m_bits = other.m_bits;
    this->e_bits = other.e_bits;
    this->s = new uint8_t[size];
    this->z = new uint8_t[size];
    this->m = new uint64_t[size];
    this->e = new uint64_t[size];
    memcpy(this->s, other.s, size * sizeof(uint8_t));
    memcpy(this->z, other.z, size * sizeof(uint8_t));
    memcpy(this->m, other.m, size * sizeof(uint64_t));
    memcpy(this->e, other.e, size * sizeof(uint64_t));
    return *this;
  }

  // move assignment
  FPArray &operator=(FPArray &&other) noexcept {
    if (this == &other) return *this;

    delete[] this->s;
    delete[] this->z;
    delete[] this->m;
    delete[] this->e;
    this->party = other.party;
    this->size = other.size;
    this->m_bits = other.m_bits;
    this->e_bits = other.e_bits;
    this->s = other.s;
    this->z = other.z;
    this->m = other.m;
    this->e = other.e;
    other.s = nullptr;
    other.z = nullptr;
    other.m = nullptr;
    other.e = nullptr;
    return *this;
  }

  // FPArray[i, j)
  FPArray subset(int i, int j);

};

std::ostream &operator<<(std::ostream &os, FPArray &other);

FPArray concat(const vector<FPArray>& x);

class FPMatrix : public FPArray {
public:
  int dim1, dim2;

  FPMatrix(){};

  FPMatrix(int party_, int dim1_, int dim2_, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS) : FPArray(party_, dim1_*dim2_, m_bits_, e_bits_) {
    this->dim1 = dim1_;
    this->dim2 = dim2_;
  }

  FPMatrix(int dim1_, int dim2_, const FPArray &other) : FPArray (other) {
    assert(dim1_*dim2_ == other.size);
    this->dim1 = dim1_;
    this->dim2 = dim2_;
  }

  // copy constructor
  FPMatrix(const FPMatrix &other) : FPArray(other) {
    this->dim1 = other.dim1;
    this->dim2 = other.dim2;
  }

  // move constructor
  FPMatrix(FPMatrix &&other) noexcept : FPArray(std::move(other)) {
    this->dim1 = other.dim1;
    this->dim2 = other.dim2;
  }

  // copy assignment
  FPMatrix &operator=(const FPMatrix &other) {
    FPArray::operator=(other);
    this->dim1 = other.dim1;
    this->dim2 = other.dim2;
    return *this;
  }

  // move assignment
  FPMatrix &operator=(FPMatrix &&other) noexcept {
    FPArray::operator=(std::move(other));
    this->dim1 = other.dim1;
    this->dim2 = other.dim2;
    return *this;
  }

  FPMatrix transpose();
};

std::ostream &operator<<(std::ostream &os, FPMatrix &other);
class FPOp {
public:
  int party;
  sci::IOPack *iopack;
  sci::OTPack *otpack;
  FixOp *fix;
  BoolOp *bool_op;
  FPOp *fp_op;

  FPOp(int party, sci::IOPack *iopack, sci::OTPack *otpack) {
    this->party = party;
    this->iopack = iopack;
    this->otpack = otpack;
    this->fix = new FixOp(party, iopack, otpack);
    this->bool_op = new BoolOp(party, iopack, otpack);
    this->fp_op = this;
  }

  ~FPOp() {
    delete fix;
    delete bool_op;
  }

  // input functions: return a FPArray that stores input data_
  // party_ denotes which party provides the input data_ and the data_ provided by the other party is ignored. If party_ is PUBLIC, then the data_ provided by both parties must be identical.
  // sz is the size of the returned FPArray and the array pointed by data_
  // m_bits_ and e_bits_ are the mantissa and exponent bits, respectively of the input and default to 32-bit float if not provided
  // if check_params = true, the input is checked if it overflows/underflows the bounds of the FP representation <e_bits_, m_bits_>
  template <class T>
  FPArray input(int party_, int sz, T* data_, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS, bool check_params = true);
  // same as the above function, except that it replicates data_ in all sz positions of the returned FPArray
  template <class T>
  FPArray input(int party_, int sz, T data_, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS, bool check_params = true);
  // same as above function, except it provides data_ in the form of 4 arrays (s_, z_, m_, e_)
  // used internally to initialize a FPArray from individual components
  FPArray input(int party_, int sz, uint8_t* s_, uint8_t* z_,
          uint64_t* m_, uint64_t* e_, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS);

  template <class T>
  FPMatrix input(int party_, int dim1, int dim2, T* data_, uint8_t m_bits_ = FP32_M_BITS,
      uint8_t e_bits_ = FP32_E_BITS, bool check_params = true) {
    return FPMatrix(dim1, dim2, fp_op->input<T>(party_, dim1*dim2, data_, m_bits_, e_bits_, check_params));
  }
  template <class T>
  FPMatrix input(int party_, int dim1, int dim2, T data_, uint8_t m_bits_ = FP32_M_BITS,
      uint8_t e_bits_ = FP32_E_BITS, bool check_params = true) {
    return FPMatrix(dim1, dim2, fp_op->input<T>(party_, dim1*dim2, data_, m_bits_, e_bits_, check_params));
  }
  FPMatrix input(int party_, int dim1, int dim2, uint8_t* s_, uint8_t* z_,
          uint64_t* m_, uint64_t* e_, uint8_t m_bits_ = FP32_M_BITS,
          uint8_t e_bits_ = FP32_E_BITS) {
    return FPMatrix(dim1, dim2, fp_op->input(party_, dim1*dim2, s_, z_, m_, e_, m_bits_, e_bits_));
  }

  // output function: returns the secret array underlying x in the form of a PUBLIC FPArray
  // party_ denotes which party will receive the output. If party_ is PUBLIC, both parties receive the output.
  FPArray output(int party_, const FPArray& x);

  FPMatrix output(int party_, const FPMatrix& x) {
    return FPMatrix(x.dim1, x.dim2, fp_op->output(party_, static_cast<FPArray>(x)));
  }

  // returns (BoolArray x.s, BoolArray x.z, FixArray x.m, FixArray x.e)
  std::tuple<BoolArray,BoolArray,FixArray,FixArray> get_components(const FPArray &x);

  // if x overflows/underflows the representation, set it as inf/0
  // x must be a secret-shared array (otherwise, simply call x.check_bounds())
  FPArray check_bounds(const FPArray &x);

  // Multiplexers: return x[i] if cond[i] = 1; else return y[i]
  // cond must be a secret-shared BoolArray
  //// x, y must be secret-shared FPArray
  //// cond, x, y must have equal size
  //// x, y must have same m_bits and e_bits
  //// x.m_bits + x.e_bits + 5 <= 64
  FPArray if_else(const BoolArray &cond, const FPArray &x, const FPArray &y);
  //// x must be secret-shared
  //// cond, x must have equal size
  //// y[i] = y (with same m_bits and e_bits as x)
  FPArray if_else(const BoolArray &cond, const FPArray &x, float y);
  FPArray if_else(const BoolArray &cond, const FPArray &x, double y);

  // Comparison Operations: return x[i] OP y[i] (OP = {<, >, <=, >=}) in the form of a BoolArray
  // if equal_sign = true, it is assumed that x and y have the same sign and comparison is a bit cheaper
  //// At least one of x and y must be secret-shared
  //// x, y must have same size, m_bits and e_bits
  BoolArray LT(const FPArray &x, const FPArray &y, bool equal_sign = false);
  BoolArray GT(const FPArray &x, const FPArray &y, bool equal_sign = false);
  BoolArray LE(const FPArray &x, const FPArray &y, bool equal_sign = false);
  BoolArray GE(const FPArray &x, const FPArray &y, bool equal_sign = false);
  //// x must be secret-shared
  //// y[i] = y (float/double converted to same m_bits and e_bits as x)
  template <class T>
  BoolArray LT(const FPArray &x, T y, bool equal_sign = false);
  template <class T>
  BoolArray GT(const FPArray &x, T y, bool equal_sign = false);
  template <class T>
  BoolArray LE(const FPArray &x, T y, bool equal_sign = false);
  template <class T>
  BoolArray GE(const FPArray &x, T y, bool equal_sign = false);

  // Lookup Table (LUT): returns spec_vec[x[i]]
  // Output has m_bits mantissa bits and e_bits exponent bits
  // x must be secret-shared and x.ell <= 8
  // spec_vec.size() must be equal to 2^{x.ell}
  FPArray LUT(const std::vector<uint64_t> &spec_vec, const FixArray &x,
              uint8_t m_bits, uint8_t e_bits);

  // Retrieve Spline's Coefficients: returns { spec_coeff[i][j] }_{i \in {0, ..., num_coeffs-1}} if knots_bits[j-1] <= x < knots_bits[j], where num_coeffs = spec_coeff.size() and j \in {0, ..., n-1}
  // n is the number of pieces in the spline
  // Output has m_bits mantissa bits and e_bits exponent bits
  // x must be secret-shared and x.ell <= 8
  // n must be <= min{2^{x.ell}, 64}
  // Size of knots_bits must be n-1 (knots_bits[-1] is -inf and knots_bits[n-1] is inf)
  // For all i \in {0, ..., num_coeffs-1}, spec_coeff[i] must be of size n
  std::vector<FPArray>
  GetCoeffs(const std::vector<std::vector<uint64_t>> &spec_coeff,
            const std::vector<uint64_t> &knots_bits, const FixArray &x,
            int n, uint8_t m_bits, uint8_t e_bits);

  // Normalize m and adjust e accordingly: modifies {m[i] -> m[i] << (ell-1-k)} and {e[i] -> e[i]+k-e_offset}, where 2^k <= m[i] < 2^{k+1}
  // The bitlengths of m and e remain the same. The scale of m changes to ell-1
  // m must be secret-shared and the size of m and e must be the same
  //// ell <= m.ell and this parameter is used when we want to normalize m such that the (ell-1)-th bit position is always set and it is known that k <= ell-1
  void normalize(FixArray &m, FixArray &e, int e_offset, int ell);
  //// ell = m.ell
  inline void normalize(FixArray &m, FixArray &e, int e_offset) {
    return normalize(m, e, e_offset, m.ell);
  }

  // Round and Check: modifies {m[i] -> m[i] >>_R s} and {e[i] -> e[i]} if m[i] < (2^{m.ell}-2^{s-1}) (no overflow from rounding), and modifies {m[i] -> 2^{m.ell-s-1}} and {e[i] -> e[i]+1} otherwise (rounding leads to overflow)
  // m and e must be secret-shared and of equal size
  // m must be normalized (\in [1, 2)) with scale m.ell-1
  void round_and_check(FixArray &m, FixArray &e, int s);

  // Returns -x
  FPArray flip_sign(const FPArray& x);

  // Multiplication by power-of-2: returns x[i] * 2^{exp[i]}
  // check_bounds is an optional parameter which rounds to 0/inf in case of underflows/overflows after multiplication
  // x must be secret-shared
  //// exp can be secret-shared or PUBLIC
  //// x and exp must be of equal size
  //// exp must be a signed FixArray with bitlength <= x.e_bits + 2 (bitlength of FixArray holding x's exponent)
  FPArray mulpow2(const FPArray &x, const FixArray &exp, bool check_bounds = true);
  //// exp[i] = exp (with bitlength x.e_bits + 2)
  FPArray mulpow2(const FPArray &x, int exp, bool check_bounds = true);

  vector<FPArray> mul(const vector<FPArray> &x, const vector<FPArray> &y) ;

  // Multiplication: returns x[i] * y[i]
  // check_bounds is an optional parameter which rounds to 0/inf in case of underflows/overflows after multiplication
  // At least one of x and y must be secret-shared
  // x and y must be of the same size, and have the same mantissa (m_bits) and exponent bits (e_bits)
  FPArray mul(const FPArray &x, const FPArray &y, bool check_bounds = true);

  // Addition: return x[i] + y[i]
  // If cheap_variant is set to true, addition is cheaper in cost but does not return correctly rounded results
  // compare_and_swap is an optional parameter which can be set to 1 to save some cost if | x | >= | y |
  // check_bounds is an optional parameter which rounds to 0/inf in case of underflows/overflows after addition
  // Both x and y must be secret-shared
  // x and y must be of the same size, and have the same mantissa (m_bits) and exponent bits (e_bits)
  FPArray add(const FPArray &x, const FPArray &y, bool cheap_variant = false,
              bool compare_and_swap = true, bool check_bounds = true, int output_m_bits = -1);

  // Subtraction: return x[i] - y[i]
  // If cheap_variant is set to true, subtraction is cheaper in cost but does not return correctly rounded results
  // compare_and_swap is an optional parameter which can be set to 1 to save some cost if | x | >= | y |
  // check_bounds is an optional parameter which rounds to 0/inf in case of underflows/overflows after subtraction
  // Both x and y must be secret-shared
  // x and y must be of the same size, and have the same mantissa (m_bits) and exponent bits (e_bits)
  FPArray sub(const FPArray &x, const FPArray &y, bool cheap_variant = false,
              bool compare_and_swap = true, bool check_bounds = true);

  vector<FPArray> div(const vector<FPArray> &x, const FPArray &y, bool cheap_variant = false,
              bool check_bounds = true);
  // Division: returns x[i] / y[i]
  // If cheap_variant is set to true, division is cheaper in cost but does not return correctly rounded results
  // check_bounds is an optional parameter which rounds to 0/inf in case of underflows/overflows after division
  // Both x and y must be secret-shared
  // x and y must be of the same size, and have the same mantissa (m_bits) and exponent bits (e_bits)
  FPArray div(const FPArray &x, const FPArray &y, bool cheap_variant = false,
      bool check_bounds = true) {
    vector<FPArray> x_vec(1);
    x_vec[0] = x;
    return (fp_op->div(x_vec, y, cheap_variant, check_bounds))[0];
  }

  // Square-Root: returns sqrt(x[i])
  // x must be secret-shared
  FPArray sqrt(const FPArray &x);

  // Convert Integer to Float: returns an FPArray with m_bits mantissa bits and e_bits exponent bits that holds float(x[i]) in the i-th slot
  // x must be a secret-shared IntegerArray (i.e., x.s must be 0)
  // x.ell <= x.e_max = 2^{x.e_bits - 1} - 1
  FPArray int_to_float(const FixArray &x, int m_bits = FP32_M_BITS,
                       int e_bits = FP32_E_BITS);

  // Finds max element m_i in x[i], forall i
  // Returns a FPArray of length x.size() with m_i in i-th index
  FPArray max(const std::vector<FPArray>& x);

  // Finds sum s_i of elements in x[i], forall i
  // Returns a FPArray of length x.size() with s_i in i-th index
  FPArray treesum(const vector<FPArray> &x) ;

  FPMatrix matrix_multiplication(const FPMatrix &x, const FPMatrix &y) ;
};

#endif // FLOATING_POINT_H__
