
#ifndef SRC_primihub_PROTOCOL_ABY3_RUNTIME_H_
#define SRC_primihub_PROTOCOL_ABY3_RUNTIME_H_

#include <deque>
#include <unordered_map>
#include <string>
#include <unordered_set>
#include <sstream>
#include <vector>
#include <utility>

#include <boost/variant.hpp>
#include "function2/function2.hpp"

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/protocol/task.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/protocol/sh_task.h"
#include "src/primihub/protocol/runtime.h"

namespace primihub {

using Sh3Task = ShTask;
using Sh3Runtime = Runtime;


inline std::ostream& operator<<(std::ostream& o, const Sh3Task& d) {
  o << d.mIdx;
  if (d.mType == Sh3Task::Evaluation)
    o << ".E";
  else
    o << ".C";
  return o;
}

}  // namespace primihub

#endif  // SRC_primihub_PROTOCOL_ABY3_RUNTIME_H_
