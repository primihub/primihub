

#ifndef SRC_PRIMIHUB_PROTOCOL_SH_TASK_H_
#define SRC_PRIMIHUB_PROTOCOL_SH_TASK_H_

#include <memory>

#include <boost/variant.hpp>
#include "function2/function2.hpp"

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/commpkg.h"

namespace primihub {


class Runtime;
class ShTaskBase;


class ShTask {

    public:
        using RoundFunc = fu2::unique_function<void(CommPkgBase* commPtr, ShTask& self)>;
        using ContinuationFunc = fu2::unique_function<void(ShTask& self)>;
        enum Type { Evaluation, Closure };

        // returns the associated runtime.
        Runtime& getRuntime() const { return *mRuntime; }

        // return the task id
        i64 getIdx() const { return mIdx; }

        // schedules a task that can be executed in the following round.
        ShTask then(RoundFunc task);

        // schedule a task that can be executed in the same round as this task.
        ShTask then(ContinuationFunc task);

        // schedules a task that can be executed in the following round.
        ShTask then(RoundFunc task, std::string name);

        // schedule a task that can be executed in the same round as this task.
        ShTask then(ContinuationFunc task, std::string name);

        // Get a task that is fulfilled when all of this tasks dependencies
        // are fulfilled.
        ShTask getClosure();

        std::string& name();

        ShTask operator&&(const ShTask& o) const;

        ShTask operator&=(const ShTask& o);

        // blocks until this task is completed.
        void get();

        bool isCompleted();

        bool operator==(const ShTask& t) const {
            return mRuntime == t.mRuntime && mIdx == t.mIdx && mType == t.mType;
        }

        bool operator!=(const ShTask& t) const { return !(*this == t); }


        ShTaskBase* basePtr();
        Runtime* mRuntime = nullptr;
        i64 mIdx = -1;
        Type mType = Type::Evaluation;
};


class ShTaskBase {
    public:
        ShTaskBase() = default;
        ShTaskBase(const ShTaskBase&) = delete;
        ShTaskBase(ShTaskBase&&) = default;
        ~ShTaskBase() = default;

        struct EmptyState {};
        struct And {};
        struct Closure {};

        std::string mName;

        boost::variant<EmptyState,
                        Closure,
                        And,
                        ShTask::RoundFunc,
                        ShTask::ContinuationFunc>
        mFunc = EmptyState{};
};



} // namespace primihub


#endif  // SRC_PRIMIHUB_PROTOCOL_SH_TASK_H_



