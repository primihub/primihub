// Copyright [2021] <primihub.com>
#include "src/primihub/util/timer.h"

namespace primihub {
const Timer::timeUnit& Timer::setTimePoint(const std::string& msg)
{
    mTimes.push_back(std::make_pair(timeUnit::clock::now(), msg));
    return  mTimes.back().first;
}

void Timer::reset()
{
    setTimePoint("__Begin__");
    mTimes.clear();
}

std::ostream& operator<<(std::ostream& out, const Timer& timer)
{
    if (timer.mTimes.size() > 1)
    {
        uint64_t maxStars = 10;
        uint64_t p = 9;
        uint64_t width = 0;
        auto maxLog = 1.0;

        {
            auto prev = timer.mTimes.begin();
            auto iter = timer.mTimes.begin(); ++iter;

            while (iter != timer.mTimes.end())
            {
                width = std::max<uint64_t>(width, iter->second.size());
                auto diff = std::chrono::duration_cast<std::chrono::microseconds>(iter->first - prev->first).count() / 1000.0;
                maxLog = std::max(maxLog, std::log2(diff));
                ++iter;
                ++prev;
            }
        }
        width += 3;


        out << std::left << std::setw(width) << "Label  " << "  " << std::setw(p) << "Time (ms)" << "  " << std::setw(p) << "diff (ms)\n__________________________________"  << std::endl;

        auto prev = timer.mTimes.begin();
        auto iter = timer.mTimes.begin(); ++iter;

        while (iter != timer.mTimes.end())
        {
            auto time = std::chrono::duration_cast<std::chrono::microseconds>(iter->first - timer.mTimes.front().first).count() / 1000.0;
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(iter->first - prev->first).count() / 1000.0;
            uint64_t numStars = static_cast<uint64_t>(std::round(std::max(0.1, std::log2(diff)) * maxStars / maxLog));

            out << std::setw(width) << std::left << iter->second
                << "  " << std::right << std::fixed << std::setprecision(1) << std::setw(p) << time
                << "  " << std::right << std::fixed << std::setprecision(3) << std::setw(p) << diff
                << "  " << std::string(numStars, '*') << std::endl;;

            ++prev;
            ++iter;
        }
    }
    return out;
}

Timer gTimer(true);

}
