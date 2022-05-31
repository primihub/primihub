
#ifndef SRC_PRIMIHUB_PROTOCOL_TASK_H_
#define SRC_PRIMIHUB_PROTOCOL_TASK_H_

#include <vector>

#include "src/primihub/common/defines.h"

using namespace std;

namespace primihub
{

    enum class TaskType
    {
        Round,
        Continuation
    };

    class TaskBase
    {
    public:
        TaskBase(TaskType t, u64 idx) : mType(t), mIdx(idx) {}
        ~TaskBase() {}

        void addDownstream(u64 idx);
        void addUpstream(u64 idx);
        void removeUpstream(u64 idx);
        void addClosure(u64 idx);

    // protected: // FIXME need private
        TaskType mType;
        i64 mIdx = -1;
        std::vector<u64> mUpstream, mDownstream, mClosures;
    };

} // namespace primihub


#endif  // SRC_PRIMIHUB_PROTOCOL_TASK_H_