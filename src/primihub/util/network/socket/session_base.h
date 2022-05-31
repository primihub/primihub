// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_BASE_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_BASE_H_

#include <sstream>
#include <random>
#include <string>
#include <list>
#include <mutex>
#include <memory>

#include <boost/lexical_cast.hpp>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/network/socket/tls.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/acceptor.h"

#ifdef ENABLE_NET_LOG
#define LOG_MSG(m) mLog.push(m)
#define IF_LOG(m) m
#else
#define LOG_MSG(m)
#define IF_LOG(m)
#endif

namespace primihub {

class IOService;
class Acceptor;
struct SessionGroup;

struct SessionBase {
  SessionBase(IOService& ios);
  ~SessionBase();

  void stop();
  //  Removes the channel with chlName.
  // void removeChannel(ChannelBase* chl);

  // if this channnel is waiting on a socket, cancel that
  // operation and set the future to contain an exception
  // void cancelPendingConnection(ChannelBase* chl);

  std::string mIP;
  u32 mPort = 0, mAnonymousChannelIdx = 0;
  SessionMode mMode = SessionMode::Client;
  bool mStopped = true;
  IOService* mIOService = nullptr;
  Acceptor* mAcceptor = nullptr;

  std::atomic<u32> mRealRefCount;

  Work mWorker;

  // bool mHasGroup = false;
  std::shared_ptr<SessionGroup> mGroup;

  TLSContext mTLSContext;

  std::mutex mAddChannelMtx;
  std::string mName;

  u64 mSessionID = 0;

#ifdef ENABLE_WOLFSSL
  std::mutex mTLSSessionIDMtx;
  bool mTLSSessionIDIsSet = false;
  block mTLSSessionID;
#endif

  boost::asio::ip::tcp::endpoint mRemoteAddr;
};

}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_BASE_H_
