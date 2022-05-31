// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"

namespace primihub {

TEST(Sh3ShareGenTest, get_test) {

  Sh3ShareGen gen;
  gen.init(toBlock(1), toBlock((1 + 1) % 3));
  block p, n;
  p = gen.mPrevCommon.get();
  n = gen.mNextCommon.get();

  std::cout << "p:" << p << ", n:" << n << std::endl;
}

}