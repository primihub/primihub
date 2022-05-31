// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_TIMER_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_TIMER_H_

#include <list>
#include <chrono>
#include <string>
#include <mutex>
#include <ostream>
#include <iomanip>
#include <cmath>

#include "src/primihub/util/log.h"

namespace primihub { 

    class Timer
    {
    public:

        typedef std::chrono::system_clock::time_point timeUnit;

        //struct TimerSpan
        //{
        //    TimerSpan(Timer&t, std::string name)
        //        : mTimer(&t)
        //        , mName(std::move(name))
        //        , mBegin(timeUnit::clock::now())
        //    {}
        //    ~TimerSpan()
        //    {
        //        end();
        //    }
        //    void end()
        //    {
        //        if (mTimer)
        //        {
        //            mTimer->addSpan(mBegin, timeUnit::clock::now());
        //            mTimer = nullptr;
        //        }
        //    }
        //    Timer* mTimer;
        //    std::string mName;
        //    timeUnit mBegin;
        //};
        //void addSpan(timeUnit begin, timeUnit end);


        std::list< std::pair<timeUnit, std::string>> mTimes;
        bool mLocking;
        std::mutex mMtx;

        Timer(bool locking = false)
            :mLocking(locking)
        {
            reset();
        }

        const timeUnit& setTimePoint(const std::string& msg);


        friend std::ostream& operator<<(std::ostream& out, const Timer& timer);

        void reset();
    };

	extern Timer gTimer;
    class TimerAdapter
    {
    public:
        virtual void setTimer(Timer& timer)
        {
            mTimer = &timer;
        }

        Timer& getTimer()
        {
            if (mTimer)
                return *mTimer;

            throw std::runtime_error("Timer net set. ");
        }

        Timer::timeUnit setTimePoint(const std::string& msg)
        {
            if(mTimer) return getTimer().setTimePoint(msg);
            else return Timer::timeUnit::clock::now();
        }

        Timer* mTimer = nullptr;
    };


}

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_TIMER_H_
