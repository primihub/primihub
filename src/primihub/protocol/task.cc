

#include <algorithm> //std::find

#include "task.h"

namespace primihub {
void TaskBase::addDownstream(u64 idx)
{
    auto &ds = mDownstream;
    if (std::find(ds.begin(), ds.end(), idx) == ds.end())
        ds.push_back(idx);
}

void TaskBase::addUpstream(u64 idx)
{
    auto &ds = mUpstream;
    if (std::find(ds.begin(), ds.end(), idx) == ds.end())
        ds.push_back(idx);
}

void TaskBase::removeUpstream(u64 idx)
{
    auto &ds = mUpstream;
    auto iter = std::find(ds.begin(), ds.end(), idx);
    if (iter == ds.end())
        throw RTE_LOC;

    std::swap(*iter, *(ds.end() - 1));
    ds.resize(ds.size() - 1);
}

void TaskBase::addClosure(u64 idx)
{
    auto &ds = mClosures;
    if (std::find(ds.begin(), ds.end(), idx) == ds.end())
        ds.push_back(idx);
}

} // namespace primihub