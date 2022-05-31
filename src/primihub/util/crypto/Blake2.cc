// Copyright [2021] <primihub.com>
#include "src/primihub/util/crypto/Blake2.h"

namespace primihub {

const u64 Blake2::HashSize;
const u64 Blake2::MaxHashSize;

const Blake2& Blake2::operator=(const Blake2& src) {
  state = src.state;
  return *this;
}

}
