// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_IOSERVICE_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_IOSERVICE_H_

#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <memory>
#include <vector>
#include <utility>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/finally.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/crypto/aes/aes.h"
#include "src/primihub/util/network/socket/socketadapter.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/iobuffer.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/acceptor.h"

# if defined(_WINSOCKAPI_) && !defined(_WINSOCK2API_)
#  error WinSock.h has already been included. Please move the boost headers above the WinNet*.h headers
# endif // defined(_WINSOCKAPI_) && !defined(_WINSOCK2API_)

#ifndef _MSC_VER
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <boost/asio.hpp>

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <thread> 
#include <mutex>
#include <list> 
#include <future>
#include <string>
#include <unordered_map>
#include <functional>

namespace primihub {

class Acceptor;
class IOOperation;
struct SessionBase;

std::vector<std::string> split(const std::string &s, char delim);

class IOService {
  friend class Channel;
  friend class Session;
 public:
#ifdef ENABLE_NET_LOG
  Log mLog;
  std::mutex mWorkerMtx;
  std::unordered_map<void*, std::string> mWorkerLog;
#endif
  // std::unordered_set<ChannelBase*> mChannels;

  block mRandSeed;
  std::atomic<u64> mSeedIndex;

  // returns a unformly random block.
  block getRandom();

  IOService(const IOService&) = delete;

  // Constructor for the IO service that services network IO operations.
  // threadCount is The number of threads that should be used to
  // service IO operations. 0 = use # of CPU cores.
  IOService(u64 threadCount = 0);
  ~IOService();

  boost::asio::io_service mIoService;
  boost::asio::strand<boost::asio::io_service::executor_type> mStrand;

  Work mWorker;

  std::list<std::pair<std::thread, std::promise<void>>> mWorkerThrds;

  // The list of acceptor objects that hold state about the ports
  // that are being listened to.
  std::list<Acceptor> mAcceptors;

  // indicates whether stop() has been called already.
  bool mStopped = false;

  // Gives a new endpoint which is a host endpoint the acceptor
  // which provides sockets.
  void aquireAcceptor(std::shared_ptr<SessionBase>& session);

  // Shut down the IO service. WARNING: blocks until all Channels
  // and Sessions are stopped.
  void stop();

  void showErrorMessages(bool v);

  void printError(std::string msg);

  void workUntil(std::future<void>& fut);

  operator boost::asio::io_context&() {
    return mIoService;
  }

  bool mPrint = true;
};

}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_IOSERVICE_H_
