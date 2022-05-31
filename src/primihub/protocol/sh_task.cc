
#include "src/primihub/protocol/sh_task.h"
#include "src/primihub/protocol/runtime.h"

namespace primihub {

   ShTask ShTask::then(RoundFunc task) {
    return getRuntime().addTask({ this, 1 }, std::move(task), {});
  }

  ShTask ShTask::then(ContinuationFunc task) {
    return getRuntime().addTask({ this, 1 }, std::move(task), {});
  }

  ShTask ShTask::then(RoundFunc task, std::string name) {
    return getRuntime().addTask({ this, 1 }, std::move(task), std::move(name));
  }

  ShTask ShTask::then(ContinuationFunc task, std::string name) {
    return getRuntime().addTask({ this, 1 }, std::move(task), std::move(name));
  }

  ShTask ShTask::getClosure() {
    return getRuntime().addClosure(*this);
  }

  std::string& ShTask::name() { return basePtr()->mName; }

  ShTask ShTask::operator&&(const ShTask& o) const {
    std::array<ShTask, 2> deps{ *this, o };
    return getRuntime().addAnd(deps, {});
  }

  ShTask ShTask::operator&=(const ShTask& o) {
    *this = *this && o;
    return *this;
  }

  void ShTask::get() {
    getRuntime().runUntilTaskCompletes(*this);
  }

  bool ShTask::isCompleted() {
    auto iter = mRuntime->mSched.mTasks.find(mIdx);
    return iter == mRuntime->mSched.mTasks.end();
  }


  ShTaskBase* ShTask::basePtr() {
    auto iter = mRuntime->mTasks.find(mIdx);
    if (iter != mRuntime->mTasks.end())
      return &iter->second;
    return nullptr;
  }

} 

