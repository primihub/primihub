
#ifndef SRC_PRIMIHUB_PRIMITIVE_OT_SHARE_OT_H_
#define SRC_PRIMIHUB_PRIMITIVE_OT_SHARE_OT_H_

#include <tuple>
#include <memory>
#include <vector>
#include <utility>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/crypto/bit_vector.h"
#include "src/primihub/util/network/socket/channel.h"

namespace primihub {

class SharedOT {
 public:
  AES_Type mAes;
  u64 mIdx = -1;

  void setSeed(const block& seed, u64 seedIdx = 0);

  void send(Channel& recver, span<std::array<i64,2>> msgs);

  void help(Channel& recver, const BitVector& choices);

  static void recv(Channel& sender, Channel & helper,
                   const BitVector& choices, span<i64> recvMsgs);

  static std::future<void> asyncRecv(Channel& sender,
                                     Channel & helper,
                                     BitVector&& choices,
                                     span<i64> recvMsgs);
};

}  // namespace primihub

#endif  // SRC_primihub_PRIMITIVE_OT_SHARE_OT_H_
