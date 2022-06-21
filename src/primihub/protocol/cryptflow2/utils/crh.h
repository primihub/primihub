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

#include "src/primihub/protocol/cryptflow2/utils/prp.h"
#include <stdio.h>
#ifndef CRH_H__
#define CRH_H__
/** @addtogroup BP
  @{
 */
namespace primihub::sci {

class CRH : public PRP {
public:
  CRH(const char *seed = fix_key) : PRP(seed) {}

  CRH(const block128 &seed) : PRP(seed) {}

  block128 H(block128 in) {
    block128 t = in;
    permute_block(&t, 1);
    return xorBlocks(t, in);
  }

#ifdef __GNUC__
#ifndef __clang__
#pragma GCC push_options
#pragma GCC optimize("unroll-loops")
#endif
#endif

  template <int n> void H(block128 out[n], block128 in[n]) {
    block128 tmp[n];
    for (int i = 0; i < n; ++i)
      tmp[i] = in[i];
    permute_block(tmp, n);
    xorBlocks_arr(out, in, tmp, n);
  }
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC pop_options
#endif
#endif

  void Hn(block128 *out, block128 *in, int n, block128 *scratch = nullptr) {
    bool del = false;
    if (scratch == nullptr) {
      del = true;
      scratch = new block128[n];
    }
    for (int i = 0; i < n; ++i)
      scratch[i] = in[i];
    permute_block(scratch, n);
    xorBlocks_arr(out, in, scratch, n);
    if (del) {
      delete[] scratch;
      scratch = nullptr;
    }
  }
};
} // namespace primihub::sci
/**@}*/
#endif // CRH_H__
