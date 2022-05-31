// Copyright [2021] <primihub.com>
#include "src/primihub/util/network/socket/session_base.h"

namespace primihub {

SessionBase::SessionBase(IOService& ios)
  : mRealRefCount(1)
  , mWorker(ios, "Session:" + std::to_string((u64)this)) {}

void SessionBase::stop() {
  if (mStopped == false) {
    mStopped = true;
    if (mAcceptor)
      mAcceptor->unsubscribe(this);
    mWorker.reset();
  }
}

SessionBase::~SessionBase() {
  stop();
}

}  // namespace primihub
