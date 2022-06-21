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

#ifndef EMP_SWAPPABLE_H__
#define EMP_SWAPPABLE_H__
#include "src/primihub/protocol/cryptflow2/GC/bit.h"
namespace primihub::sci {
class Bit;
template <typename T> class Swappable {
public:
  T If(const Bit &sel, const T &rhs) const {
    return static_cast<const T *>(this)->select(sel, rhs);
  }
};
template <typename T> inline T If(const Bit &select, const T &o1, const T &o2) {
  T res = o2;
  return res.If(select, o1);
}
template <typename T> inline void swap(const Bit &swap, T &o1, T &o2) {
  T o = If(swap, o1, o2);
  o = o ^ o2;
  o1 = o ^ o1;
  o2 = o ^ o2;
}
} // namespace primihub::sci
#endif
