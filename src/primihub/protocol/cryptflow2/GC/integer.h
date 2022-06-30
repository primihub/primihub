/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2021 Microsoft Research

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

#ifndef EMP_INTEGER_H__
#define EMP_INTEGER_H__

#include "src/primihub/protocol/cryptflow2/GC/bit.h"
#include "src/primihub/protocol/cryptflow2/GC/comparable.h"
#include "src/primihub/protocol/cryptflow2/GC/number.h"
#include "src/primihub/protocol/cryptflow2/GC/swappable.h"
#include <algorithm>
#include <bitset>
#include <math.h>
#include <vector>

using std::min;
using std::vector;

namespace primihub::sci {
class Integer : public Swappable<Integer>, public Comparable<Integer> {
public:
  std::vector<Bit> bits;
  Integer() {}
  Integer(int length, int64_t input, int party = PUBLIC);

  // Comparable
  Bit geq(const Integer &rhs) const;
  Bit equal(const Integer &rhs) const;

  // Swappable
  Integer select(const Bit &sel, const Integer &rhs) const;
  Integer operator^(const Integer &rhs) const;

  int size() const;
  template <typename T> T reveal(int party = PUBLIC) const;

  Integer abs() const;
  Integer &resize(int length, bool signed_extend = true);
  Integer modExp(Integer p, Integer q);
  Integer leading_zeros() const;
  Integer hamming_weight() const;

  Integer operator<<(int shamt) const;
  Integer operator>>(int shamt) const;
  Integer operator<<(const Integer &shamt) const;
  Integer operator>>(const Integer &shamt) const;

  Integer operator+(const Integer &rhs) const;
  Integer operator-(const Integer &rhs) const;
  Integer operator-() const;
  Integer operator*(const Integer &rhs) const;
  Integer operator/(const Integer &rhs) const;
  Integer operator%(const Integer &rhs) const;
  Integer operator&(const Integer &rhs) const;
  Integer operator|(const Integer &rhs) const;

  Bit &operator[](int index);
  const Bit &operator[](int index) const;
};

#include "src/primihub/protocol/cryptflow2/GC/integer.hpp"
} // namespace primihub::sci
#endif // INTEGER_H__
