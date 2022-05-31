

#include "src/primihub/protocol/scheduler.h"

namespace primihub
{

    void Scheduler::addReady(i64 idx)
    {
        if (idx > mTaskIdx)
            throw RTE_LOC;

        auto iter = mTasks.find(idx);
        if (iter == mTasks.end())
            throw RTE_LOC;

        if (iter->second.mUpstream.size())
            throw RTE_LOC;

        mReady.push_back(idx);
    }

    void Scheduler::addNextRound(i64 idx)
    {
        if (idx > mTaskIdx)
            throw RTE_LOC;

        auto iter = mTasks.find(idx);
        if (iter == mTasks.end())
            throw RTE_LOC;

        if (iter->second.mUpstream.size())
            throw RTE_LOC;

        mNextRound.push_back(idx);
    }

    Task Scheduler::addTask(TaskType t, Task dep)
    {
        return addTask(t, span<Task>(&dep, 1));
    }

    Task Scheduler::addTask(TaskType t, const std::vector<Task> &deps)
    {
        return addTask(t, span<Task>((Task *)deps.data(), (int)deps.size()));
    }

    Task Scheduler::addTask(TaskType t, span<Task> deps)
    {
        auto idx = mTaskIdx++;
        auto x = mTasks.emplace(idx, TaskBase(t, idx));

        for (auto &d : deps)
        {
            if (d.mSched != this)
                throw RTE_LOC;

            if (d.mTaskIdx != -1 && d.mTaskIdx >= idx)
                throw RTE_LOC;

            auto db = mTasks.find(d.mTaskIdx);

            if (db != mTasks.end())
            {
                db->second.addDownstream(idx);
                x.first->second.addUpstream(d.mTaskIdx);
            }
            else
            {
                if (d.mTaskIdx >= idx)
                    throw std::runtime_error("This tasks idx in invalid");
            }
        }

        if (x.first->second.mUpstream.size() == 0)
        {
            addReady(idx);
        }
        return {idx, this};
    }

    Task Scheduler::addClosure(Task dep)
    {
        return addClosure(span<Task>((Task *)&dep, 1));
    }

    Task Scheduler::addClosure(const std::vector<Task> &deps)
    {
        return addClosure(span<Task>((Task *)deps.data(), deps.size()));
    }

    Task Scheduler::addClosure(span<Task> deps)
    {
        auto idx = mTaskIdx++;
        auto x = TaskBase(TaskType::Continuation, idx);

        for (auto &d : deps)
        {
            if (d.mSched != this)
                throw RTE_LOC;

            if (d.mTaskIdx != -1 && d.mTaskIdx >= idx)
                throw RTE_LOC;

            auto db = mTasks.find(d.mTaskIdx);

            if (db != mTasks.end())
            {
                db->second.addClosure(idx);
                x.addUpstream(d.mTaskIdx);
            }
            else
            {
                if (d.mTaskIdx >= idx)
                    throw std::runtime_error("This tasks idx in invalid");
            }
        }

        if (x.mUpstream.size())
        {
            mTasks.emplace(idx, std::move(x));
        }

        return {idx, this};
    }

    Task Scheduler::currentTask()
    {
        if (mReady.size() == 0)
            std::swap(mReady, mNextRound);

        if (mReady.size() == 0)
            throw RTE_LOC;

        auto idx = mReady.front();
        return {idx, this};
    }

    void Scheduler::popTask()
    {
        if (mReady.size() == 0)
            throw RTE_LOC;
        auto idx = mReady.front();
        removeTask(idx);
        mReady.pop_front();
    }

    void Scheduler::removeTask(u64 idx)
    {
        auto task = mTasks.find(idx);
        if (task == mTasks.end())
            throw RTE_LOC;

        if (task->second.mUpstream.size())
            throw RTE_LOC;

        for (auto &d : task->second.mDownstream)
        {
            auto ds = mTasks.find(d);
            if (ds == mTasks.end())
                throw RTE_LOC;

            ds->second.removeUpstream(idx);
            if (ds->second.mUpstream.size() == 0)
            {
                if (ds->second.mType == TaskType::Round)
                    addNextRound(d);
                else
                    addReady(d);
            }

            for (auto &c : task->second.mClosures)
            {
                auto cc = mTasks.find(c);
                if (cc == mTasks.end())
                    throw RTE_LOC;

                ds->second.addClosure(c);
                cc->second.addUpstream(d);
            }
        }

        for (auto &c : task->second.mClosures)
        {
            auto cc = mTasks.find(c);
            cc->second.removeUpstream(idx);
            if (cc->second.mUpstream.size() == 0)
            {
                removeTask(c);
            }
        }
        mTasks.erase(task);
    }

    Task Scheduler::nullTask()
    {
        return {-1, this};
    }

    std::string Scheduler::print()
    {
        std::stringstream ss;
        ss << "=================================\n";
        ss << "ready:";

        auto &rr = mReady.size() ? mReady : mNextRound;
        for (auto r : rr)
        {
            ss << " " << r;
        }
        ss << "\n---------------------------------\n";
        for (auto &x : mTasks)
        {
            ss << x.second.mIdx << "\n"
               << "\tup:";

            for (auto u : x.second.mUpstream)
                ss << " " << u;

            ss << "\n\tdw:";
            for (auto u : x.second.mDownstream)
                ss << " " << u;
            ss << "\n\tcl:";
            for (auto u : x.second.mClosures)
                ss << " " << u;
            ss << "\n";
        }

        return ss.str();
    }

} // namespace primihub
