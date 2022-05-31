// Copyright [2021] <primihub.com>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "src/primihub/common/type/fixed_point.h"


namespace primihub {

TEST(FixedPointTest, fp_add) {
  fp<i64, D8> fp_a(10);
  fp<i64, D8> fp_b(100);
  fp<i64, D8> fp_c;
  fp_c = fp_a + fp_b;
  EXPECT_DOUBLE_EQ(110, (double)fp_c);
}

}  // namespace primihub
