#include "gtest/gtest.h"
#include "src/primihub/util/crypto/prng.h"

using namespace primihub;

TEST(prng_test, prng_et) {
  PRNG prng(toBlock(1));
  for (u64 i = 0; i < 100; ++i) {
    std::cout << "prng.get<int>(): " << prng.get<int>() << std::endl;
  }
}