
#include "src/primihub/protocol/runtime.h"
#include "src/primihub/protocol/scheduler.h"
#include "src/primihub/common/defines.h"

namespace primihub
{
    
    ShTask Runtime::addTask(span<ShTask> deps,
                              ShTask::RoundFunc&& func, std::string&& name) {
    if (func) {
      std::vector<Task> deps2(deps.size());
      for (u64 i = 0; i < deps.size(); ++i) {
        deps2[i].mSched = &mSched;
        deps2[i].mTaskIdx = deps[i].mIdx;
      }

      auto tt = mSched.addTask(TaskType::Round, deps2);

      auto& newTask = mTasks.emplace(tt.mTaskIdx, ShTaskBase{}).first->second;
      newTask.mFunc = std::forward<ShTask::RoundFunc>(func);
      newTask.mName = std::forward<std::string>(name);

      return { this, (i64)tt.mTaskIdx };
    } else {
      std::cout << "empty task (round function)" << std::endl;
      throw RTE_LOC;
    }
  }

  ShTask Runtime::addTask(span<ShTask> deps,
    ShTask::ContinuationFunc&& func, std::string&& name) {
    if (func) {
      std::vector<Task> deps2(deps.size());
      for (u64 i = 0; i < deps.size(); ++i) {
        deps2[i].mSched = &mSched;
        deps2[i].mTaskIdx = deps[i].mIdx;
      }

      auto tt = mSched.addTask(TaskType::Round, deps2);

      auto& newTask = mTasks.emplace(tt.mTaskIdx, ShTaskBase{}).first->second;
      newTask.mFunc = std::forward<ShTask::ContinuationFunc>(func);
      newTask.mName = std::forward<std::string>(name);

      return { this, (i64)tt.mTaskIdx };
    } else {
      std::cout << "empty task (round function)" << std::endl;
      throw RTE_LOC;
    }
  }

  ShTask Runtime::addClosure(ShTask dep) {
    Task dd;
    dd.mSched = &mSched;
    dd.mTaskIdx = dep.mIdx;
    auto tt = mSched.addClosure({ &dd, 1 });

    return { this, (i64)tt.mTaskIdx };
  }

  ShTask Runtime::addAnd(span<ShTask> deps, std::string && name) {
    std::vector<Task> deps2(deps.size());
    for (u64 i = 0; i < deps.size(); ++i) {
      deps2[i].mSched = &mSched;
      deps2[i].mTaskIdx = deps[i].mIdx;
    }

    auto tt = mSched.addTask(TaskType::Round, deps2);

    auto& newTask = mTasks.emplace(tt.mTaskIdx, ShTaskBase{}).first->second;
    newTask.mFunc = ShTaskBase::And{};
    newTask.mName = std::forward<std::string>(name);

    return { this, (i64)tt.mTaskIdx };
  }

  void Runtime::runUntilTaskCompletes(ShTask task) {
    while (task.isCompleted() == false)
      runNext();
  }

  void Runtime::runAll() {
    while (mTasks.size())
      runNext();
  }

  void Runtime::runOneRound() {
    if (mSched.mTasks.size() == 0)
      return;

    mSched.currentTask();
    while (mSched.mReady.size()) {
      runNext();
    }
  }

  void Runtime::runNext() {
    if (mIsActive)
      throw std::runtime_error("The runtime is currently running a different task. Do not call ShTask.get() recursively. " LOCATION);

    auto tt = mSched.currentTask();
    auto task = mTasks.find(tt.mTaskIdx);

    ShTask t{ this, tt.mTaskIdx, ShTask::Type::Evaluation };
    auto roundFuncPtr = boost::get<ShTask::RoundFunc>(&task->second.mFunc);
    auto continueFuncPtr = boost::get<ShTask::ContinuationFunc>(
                                            &task->second.mFunc);

    mIsActive = true;
    if (roundFuncPtr)
      (*roundFuncPtr)(mCommPtr.get(), t);
    else if (continueFuncPtr)
      (*continueFuncPtr)(t);
    mIsActive = false;

    mTasks.erase(task);
    mSched.popTask();

  }

} // namespace primihub
