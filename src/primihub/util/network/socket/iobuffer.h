// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_IOBUFFER_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_IOBUFFER_H_

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

#include "src/primihub/common/defines.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/ioservice.h"

#ifdef ENABLE_NET_LOG
#include "src/primihub/util/log.h"
#endif

// #define ENABLE_NET_LOG

namespace boost {
namespace system {
  template <>
  struct is_error_code_enum<primihub::Errc_Status> : true_type {};
}
}

namespace primihub {

class ChannelBase;

class BadReceiveBufferSize : public std::runtime_error {
 public:
  u64 mSize;

  BadReceiveBufferSize(const std::string& what, u64 length)
    : std::runtime_error(what),
      mSize(length)
  { }

  BadReceiveBufferSize(const BadReceiveBufferSize& src) = default;
  BadReceiveBufferSize(BadReceiveBufferSize&& src) = default;
};


}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_IOBUFFER_H_
