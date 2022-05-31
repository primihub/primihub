// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_CHL_OPERATION_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_CHL_OPERATION_H_

#include <sstream>
#include <exception>
#include <string>
#include <future> 
#include <functional>
#include <memory> 
#include <boost/asio.hpp>
#include <system_error>
#include <type_traits>
#include <list>
#include <boost/variant.hpp>
#include <limits>
#include <utility>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/network/socket/channel_base.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/iobuffer.h"

#ifdef ENABLE_NET_LOG
#define LOG_MSG(m) mLog.push(m)
#define IF_LOG(m) m
#else
#define LOG_MSG(m)
#define IF_LOG(m)
#endif

namespace primihub {


}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_CHL_OPERATION_H_
