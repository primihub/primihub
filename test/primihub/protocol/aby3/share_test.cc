// Copyright [2021] <primihub.com>
#include <string>
#include <vector>
#include <iomanip>

#include "gtest/gtest.h"

#include "src/primihub/protocol/aby3/share.h"
#include "test/primihub/test_util.h"

namespace primihub {

TEST(ShareTest, share_add) {
  // ABY3Share<i64> alice(1), bob(10), charle(0);
  // charle = alice + bob;
 
  // std::cout << "charle:" << charle._curr._data << std::endl;
  // f64<D8> a = 1.50, b = 10.17, c = 0.0;
  // ABY3Share<f64<D8>> falice(a), fbob(b), fcharle(c);
  // fcharle = falice + fbob;

  // std::cout << "fcharle:" << fcharle.data.data << std::endl;
}

TEST(RandRingBuffer, get_share) {
  RandRingBuffer randBuffer;
  randBuffer.init(toBlock(1));
  std::cout << "get rand :" << randBuffer.getShare() << std::endl;
}

TEST(ABY3RandBuffer, get_share) {
  DistributedOracle oracle(ProtocolType::ABY3);
  std::cout << "aby3 rand share"<< std::endl;
}

}
