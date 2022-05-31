

#ifndef SRC_PRIMIHUB_PROTOCOL_SH_RUNTIME_H_
#define SRC_PRIMIHUB_PROTOCOL_SH_RUNTIME_H_

#include <deque>
#include <glog/logging.h>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// #include <span>

#include "function2/function2.hpp"
#include "src/primihub/common/defines.h"
#include "src/primihub/common/gsl/span"
#include "src/primihub/protocol/scheduler.h"
#include "src/primihub/protocol/sh_task.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {
class ShTask;

class Runtime {
public:
  Runtime() = default;
  Runtime(const Runtime &) = default;
  Runtime(Runtime &&) = default;
  Runtime(u64 partyIdx, std::shared_ptr<CommPkgBase> commPtr) {
    init(partyIdx, commPtr);
  }
  ~Runtime() {
    if (mSched.mTasks.size()) {
      LOG(WARNING) << "Runtime not empty.";
    }
  }

  void init(u64 partyIdx, std::shared_ptr<CommPkgBase> commPtr) {
    LOG(INFO) << "Init Runtime : " << partyIdx << ", " << commPtr.get();
    mPartyIdx = partyIdx;
    mCommPtr = commPtr;

    mNullTask.mRuntime = this;
    mNullTask.mIdx = -1;
  }

  const ShTask &noDependencies() const { return mNullTask; }
  operator ShTask() const { return noDependencies(); }

  ShTask addTask(span<ShTask> deps, ShTask::RoundFunc &&func,
                 std::string &&name);

  ShTask addTask(span<ShTask> deps, ShTask::ContinuationFunc &&func,
                 std::string &&name);

  ShTask addClosure(ShTask dep);
  ShTask addAnd(span<ShTask> deps, std::string &&name);

  void runUntilTaskCompletes(ShTask task);
  void runNext();
  void runAll();
  void runOneRound();

  // protected:
  bool mPrint = false;
  u64 mPartyIdx = -1;
  std::shared_ptr<CommPkgBase> mCommPtr; // FIXME CommPkg需要被不同协议重新实现

  bool mIsActive = false;
  std::unordered_map<u64, ShTaskBase> mTasks;
  Scheduler mSched;
  ShTask mNullTask;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_PROTOCOL_SH_RUNTIME_H_
