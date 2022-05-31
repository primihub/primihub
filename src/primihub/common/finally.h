// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_FINALLY_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_FINALLY_H_

#include <functional>

namespace primihub {

    class Finally
    {
        std::function<void()> mFinalizer;
        Finally() = delete;

    public:
        Finally(const Finally& other) = delete;
        Finally(std::function<void()> finalizer)
            : mFinalizer(finalizer)
        {
        }
        ~Finally()
        {
            if (mFinalizer)
                mFinalizer();
        }
    };
}

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_FINALLY_H_
