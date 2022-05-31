// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_ACCEPTOR_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_ACCEPTOR_H_

#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <string>
#include <memory>
#include <unordered_map>
#include <list>
#include <utility>

#include <boost/asio.hpp>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/finally.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/crypto/aes/aes.h"
#include "src/primihub/util/network/socket/socketadapter.h"
#include "src/primihub/util/network/socket/channel.h"

#ifdef ENABLE_NET_LOG
#define LOG_MSG(m) mLog.push(m)
#define IF_LOG(m) m
#else
#define LOG_MSG(m)
#define IF_LOG(m)
#endif

namespace primihub {

// struct osuCryptoErrCategory : boost::system::error_category {
//   const char* name() const noexcept override {
//     return "osuCrypto";
//   }

//   std::string message(int ev) const override {
//     switch (static_cast<Errc_Status>(ev)) {
//       case Errc_Status::success:
//         return "Success";
//       default:
//         return "(unrecognized error)";
//     }
//   }
// };

// const osuCryptoErrCategory theCategory{};

using error_code = boost::system::error_code;

using io_completion_handle = std::function<void(const error_code&, u64)>;

using completion_handle = std::function<void(const error_code&)>;

class ChannelBase;
class Acceptor;
struct SessionBase;
class BoostSocketInterface;

// A socket which has not been named yet. The Acceptor will create these
// objects and then call listen on them. When a new connection comes in,
// the Accept will receive the socket's name. At this point it will
// be converted to a NamedSocket and matched with a Channel.
struct PendingSocket {
  PendingSocket(boost::asio::io_service& ios) : mSock(ios) {}
  boost::asio::ip::tcp::socket mSock;
  std::string mBuff;
  // #ifdef ENABLE_NET_LOG
  u64 mIdx = 0;
  // #endif
};

// A single socket that has been named by client. Next it will be paired with
// a Channel at which point the object will be destructed.
struct NamedSocket {
  NamedSocket() = default;
  NamedSocket(NamedSocket&&) = default;

  std::string mLocalName, mRemoteName;
  std::unique_ptr<BoostSocketInterface> mSocket;
};

// A group of sockets from a single remote session which
// has not been paired with a local session/Channel.
struct SocketGroup {
  SocketGroup() = default;
  SocketGroup(SocketGroup&&) = default;

  ~SocketGroup();

  // returns whether this socket group has a socket with a matching
  // name session name and channel name.
  bool hasMatchingSocket(const std::shared_ptr<ChannelBase>& chl) const;

  // Removes this SocketGroup from the containing Acceptor once this
  // group has been merged with a SessionGroup or on cleanup.
  std::function<void()> removeMapping;

  // The human readable name associated with the remote session.
  // Can be empty.
  std::string mName;

  // The unique session identifier chosen by the client
  u64 mSessionID = 0;

  // The unclaimed sockets associated with this remote session.
  std::list<NamedSocket> mSockets;
};

// A local session that pairs up incoming sockets with local sessions.
// A SessionGroup is created when a local Session is created.
struct SessionGroup {
  SessionGroup() = default;
  ~SessionGroup();

  // return true if there are active channels waiting for a socket
  // or if there is an active session associated with this group.
  bool hasSubscriptions() const {
    return mChannels.size() || mBase.expired() == false;
  }

  // Add a newly accepted socket to this group. If there is a matching
  // ChannelBase, this socket will be directly given to it. Otherwise
  // the socket will be stored locally and wait for a matching ChannelBase
  // or until the local session is destructed.
  void add(NamedSocket sock, Acceptor* a);

  // Add a local ChannelBase to this group. If there is a matching socket,
  // this socket will be handed to the ChannelBase. Otherwise the a shared_ptr
  // to the ChannelBase will be stored here until the associated socket is
  // connected or the channel is canceled.
  void add(const std::shared_ptr<ChannelBase>& chl, Acceptor* a);

  // returns true if there is a ChannelBase in this group that has the same
  // channel name as this socket. The session name and id are not considered.
  bool hasMatchingChannel(const NamedSocket& sock) const;

  // Takes all the sockets in the SocketGroup and pairs them up with
  // ChannelBase's in this group. Any socket without a matching ChannelBase
  // will be stored here until it is matched or the session is destructed.
  void merge(SocketGroup& merge, Acceptor* a);

  // A weak pointer to the associated session. Has to be non-owning so that the
  // session is destructed when it has no more channel/references.
  std::weak_ptr<SessionBase> mBase;

  // removes this session group from the associated Acceptor.
  // Should be called when hasSubscription returns false.
  std::function<void()> removeMapping;

  // The list of unmatched sockets that we know are associated with
  // this session.
  std::list<NamedSocket> mSockets;

  // The list of unmatched Channels what are associated with this session.
  std::list<std::shared_ptr<ChannelBase>> mChannels;

  std::list<std::shared_ptr<SessionGroup>>::iterator mSelfIter;
};

class Acceptor {
 public:
  Acceptor() = delete;
  Acceptor(const Acceptor&) = delete;

  Acceptor(IOService& ioService);
  ~Acceptor();

  std::promise<void> mPendingSocketsEmptyProm, mStoppedPromise;
  std::future<void> mPendingSocketsEmptyFuture, mStoppedFuture;

  IOService& mIOService;

  boost::asio::strand<boost::asio::io_service::executor_type> mStrand;
  boost::asio::ip::tcp::acceptor mHandle;

  std::atomic<bool> mStopped;

  bool mListening = false;

  u64 mPendingSocketIdx = 0;

  // The list of sockets which have connected but which have not
  // completed the initial handshake which allows them to be named.
  std::list<PendingSocket> mPendingSockets;

  typedef std::list<std::shared_ptr<SessionGroup>> GroupList;
  typedef std::list<SocketGroup> SocketGroupList;

  // The full list of unmatched named sockets groups associated
  // with this Acceptor.
  // Each group has a session name (hint) and a unqiue session ID.
  SocketGroupList mSockets;

  // The full list of matched and unmatched SessionGroups.
  // These all have active Sessions
  // or active Channels which are waiting on matching sockets.
  GroupList mGroups;

  // A map: SessionName -> { socketGroup1, ..., socketGroupN }
  // which all have the
  // same session name (hint) but a unique ID. These groups of sockets
  // have not been
  // matched with a local Session/Channel. Then this happens the
  // SocketGroup is merged into a SessionGroup
  std::unordered_map<std::string,
    std::list<SocketGroupList::iterator>> mUnclaimedSockets;

  // A map: SessionName -> { sessionGroup1, ..., sessionGroupN }.
  // Each session has not
  // been paired up with sockets. The key is the session name.
  // For any given session name,
  // there may be several sessions. When this happens,
  // the first matching socket name gives
  // that group a Session ID.
  std::unordered_map<std::string,
    std::list<GroupList::iterator>> mUnclaimedGroups;

  // A map: SessionName || SessionID -> sessionGroup. The list of sessions
  // what have been
  // matched with a remote session. At least one channel must have been
  // matched and this
  // SessionGroup has a session ID.
  std::unordered_map<std::string,
    GroupList::iterator> mClaimedGroups;

  // Takes the connection
  // stringname="SessionName`SessionID`remoteName`localName"
  // and matches the name with a compatable ChannelBase.
  // SessionName is not unique,
  // the remote and local name of the channel itself will be used.
  // Note SessionID
  // will always be unique.
  void asyncSetSocket(std::string name,
    std::unique_ptr<BoostSocketInterface> handel);

  // Let the acceptor know that this channel is looking for a socket
  // with a matching name.
  void asyncGetSocket(std::shared_ptr<ChannelBase> chl);

  // Remove this channel from the list of channels awaiting
  // a matching socket. effectively undoes asyncGetSocket(...);
  void cancelPendingChannel(std::shared_ptr<ChannelBase> chl);

  // return true if any sessions are still accepting connections or
  // if there are still some channels with unmatched sockets.
  bool hasSubscriptions() const;

  // Make this session are no longer accepting any *new* connections. Channels
  // that have previously required sockets will still be matched.
  void unsubscribe(SessionBase* ep);

  // Make this session as interested in accepting connecctions. Will create
  // a SessionGroup.
  void asyncSubscribe(std::shared_ptr<SessionBase>& session,
    completion_handle ch);

  // void subscribe(std::shared_ptr<ChannelBase>& chl);

  // returns a pointer to the session group that has the provided name and
  // seesion ID. If no such socket group exists, one is created and returned.
  SocketGroupList::iterator getSocketGroup(const std::string& name, u64 id);

  void stopListening();
  u64 mPort;
  boost::asio::ip::tcp::endpoint mAddress;

  void bind(u32 port, std::string ip, boost::system::error_code& ec);
  void start();
  void stop();
  bool stopped() const;
  bool isListening() const { return mListening; }

  void sendServerMessage(std::list<PendingSocket>::iterator iter);

  void recvConnectionString(std::list<PendingSocket>::iterator iter);
  void erasePendingSocket(
    std::list<PendingSocket>::iterator iter);

  std::string print() const;
#ifdef ENABLE_NET_LOG
  Log mLog;
#endif
};

}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_ACCEPTOR_H_
