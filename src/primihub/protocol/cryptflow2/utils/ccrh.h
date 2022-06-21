#ifndef EMP_CCRH_H__
#define EMP_CCRH_H__
#include "src/primihub/protocol/cryptflow2/utils/prp.h"
#include <stdio.h>
namespace primihub::emp {

/*
 * By default, CRH use zero_block as the AES key.
 * Here we model f(x) = AES_{00..0}(x) as a random permutation (and thus in the
 * RPM model)
 */
class CCRH : public PRP {
public:
  CCRH(const block128 &key = zero_block) : PRP(key) {}

  block128 H(block128 in) {
    block128 t;
    t = in = sigma(in);
    permute_block(&t, 1);
    return t ^ in;
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
      tmp[i] = out[i] = sigma(in[i]);
    permute_block(tmp, n);
    xorblocks_arr(out, tmp, out, n);
  }
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC pop_options
#endif
#endif

  void Hn(block128 *out, block128 *in, uint64_t id, int length,
          block128 *scratch = nullptr) {
    bool del = false;
    if (scratch == nullptr) {
      del = true;
      scratch = new block128[length];
    }

    for (int i = 0; i < length; ++i)
      scratch[i] = out[i] = sigma(in[i]);

    permute_block(scratch, length);
    xorblocks_arr(out, scratch, out, length);

    if (del) {
      delete[] scratch;
      scratch = nullptr;
    }
  }
};

} // namespace primihub::emp
#endif // CCRH_H__
