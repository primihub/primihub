// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_BASE_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_BASE_H_

#include <ostream>
#include <list>
#include <deque>
#include <thread>
#include <chrono>
#include <future>
#include <string>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/network/socket/iobuffer.h"
#include "src/primihub/util/network/socket/socketadapter.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/chl_operation.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/tls.h"

#ifdef ENABLE_NET_LOG
#define LOG_MSG(m) mLog.push(m)
#define IF_LOG(m) m
#else
#define LOG_MSG(m)
#define IF_LOG(m)
#endif

namespace primihub {


}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_BASE_H_
