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

#ifndef EMP_NUMBER_H__
#define EMP_NUMBER_H__
#include "src/primihub/protocol/cryptflow2/GC/bit.h"

namespace primihub::sci {
template <typename T, typename D>
void cmp_swap(T *key, D *data, int i, int j, Bit acc) {
  Bit to_swap = ((key[i] > key[j]) == acc);
  swap(to_swap, key[i], key[j]);
  if (data != nullptr)
    swap(to_swap, data[i], data[j]);
}

inline int greatestPowerOfTwoLessThan(int n) {
  int k = 1;
  while (k < n)
    k = k << 1;
  return k >> 1;
}

template <typename T, typename D>
void bitonic_merge(T *key, D *data, int lo, int n, Bit acc) {
  if (n > 1) {
    int m = greatestPowerOfTwoLessThan(n);
    for (int i = lo; i < lo + n - m; i++)
      cmp_swap(key, data, i, i + m, acc);
    bitonic_merge(key, data, lo, m, acc);
    bitonic_merge(key, data, lo + m, n - m, acc);
  }
}

template <typename T, typename D>
void bitonic_sort(T *key, D *data, int lo, int n, Bit acc) {
  if (n > 1) {
    int m = n / 2;
    bitonic_sort(key, data, lo, m, !acc);
    bitonic_sort(key, data, lo + m, n - m, acc);
    bitonic_merge(key, data, lo, n, acc);
  }
}

template <typename T, typename D = Bit>
void sort(T *key, int size, D *data = nullptr, Bit acc = true) {
  bitonic_sort(key, data, 0, size, acc);
}
} // namespace primihub::sci
#endif
