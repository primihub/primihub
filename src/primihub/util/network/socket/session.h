// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_H_

#include <list>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>

#include "src/primihub/common/defines.h"
#include "src/primihub/protos/common.grpc.pb.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/iobuffer.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session_base.h"
#include "src/primihub/util/network/socket/socketadapter.h"
#include "src/primihub/util/network/socket/tls.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/util.h"

namespace primihub {

class IOService;
class Acceptor;
namespace details {
struct SessionGroup;
}
class ChannelBase;
struct SessionBase;
class Channel;

typedef SessionMode EpMode;

struct PartyConfig {
  PartyConfig() = default;
  PartyConfig(const std::string &node_id, const rpc::Task &task);
  void CopyFrom(const PartyConfig& config);
  std::string party_name() {return node_id;}
  uint16_t party_id() {return this->party_id_;}
  std::map<uint16_t, std::string>& PartyId2PartyNameMap() {return node_id_map;}
  std::map<std::string, rpc::Node>& PartyName2PartyInfoMap() {return node_map;}

 public:
  std::string node_id;
  uint16_t party_id_;
  std::string task_id;
  std::string job_id;
  std::string request_id;
  std::map<uint16_t, std::string> node_id_map;
  std::map<std::string, rpc::Node> node_map;
};

class Session {
public:
  // Start a session for the given IP and port in either Client or Server mode.
  // The server should use their local address on which the socket should bind.
  // The client should use the address of the server.
  // The same name should be used by both sessions. Multiple Sessions can be
  // bound to the same address if the same IOService is used but with different
  // name.
  void start(IOService &ioService, std::string remoteIp, u32 port,
             SessionMode type, std::string name = "");

  // Start a session for the given address in either Client or Server mode.
  // The server should use their local address on which the socket should bind.
  // The client should use the address of the server.
  // The same name should be used by both sessions. Multiple Sessions can be
  // bound to the same address if the same IOService is used but with different
  // name.
  void start(IOService &ioService, std::string address, SessionMode type,
             std::string name = "");

  void start(IOService &ioService, std::string ip, u64 port, SessionMode type,
             TLSContext &tls, std::string name = "");

  // See start(...)
  Session(IOService &ioService, std::string address, SessionMode type,
          std::string name = "");

  // See start(...)
  Session(IOService &ioService, std::string remoteIP, u32 port,
          SessionMode type, std::string name = "");

  Session(IOService &ioService, std::string remoteIP, u32 port,
          SessionMode type, TLSContext &tls, std::string name = "");

  // Default constructor
  Session();

  Session(const Session &);
  Session(Session &&) = default;

  Session(const std::shared_ptr<SessionBase> &c);

  ~Session();

  std::string getName() const;

  u64 getSessionID() const;

  IOService &getIOService();

  // Adds a new channel (data pipe) between this endpoint and the remote. The
  // channel is named at each end.
  Channel addChannel(std::string localName = "", std::string remoteName = "");

  // Stops this Session.
  void stop(/*const std::optional<std::chrono::milliseconds>& waitTime = {}*/);

  // returns whether the endpoint has been stopped (or never isConnected).
  bool stopped() const;

  u32 port() const;

  std::string IP() const;

  bool isHost() const;

  std::shared_ptr<SessionBase> mBase;
};

typedef Session Endpoint;

} // namespace primihub

#endif // SRC_primihub_UTIL_NETWORK_SOCKET_SESSION_H_
