

#ifndef SRC_PRIMIHUB_PROTOCOL_SCHEDULER_H_
#define SRC_PRIMIHUB_PROTOCOL_SCHEDULER_H_

#include <unordered_map>
#include <list>

#include "src/primihub/common/defines.h"
#include "src/primihub/protocol/task.h"
#include "src/primihub/common/gsl/span"

namespace primihub
{

  class Scheduler;

  struct Task {
      i64 mTaskIdx;
      Scheduler* mSched;
  };

  class Scheduler
  {
  public:
    void addReady(i64 idx);
    void addNextRound(i64 idx);
    Task addTask(TaskType t, Task dep);
    Task addTask(TaskType t, const std::vector<Task> &deps);
    Task addTask(TaskType t, span<Task> deps);
    Task addClosure(Task dep);
    Task addClosure(const std::vector<Task> &deps);
    Task addClosure(span<Task> deps);
    Task currentTask();
    void popTask();
    void removeTask(u64 idx);
    Task nullTask();
    std::string print();

  // private: // FIXME need private
    i64 mTaskIdx = 0;
    std::unordered_map<u64, TaskBase> mTasks;
    std::list<i64> mReady, mNextRound;
  };

} // namespace primihub

#endif  // SRC_PRIMIHUB_PROTOCOL_SCHEDULER_H_
