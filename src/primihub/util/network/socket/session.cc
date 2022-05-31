// Copyright [2021] <primihub.com>
#include "src/primihub/util/network/socket/session.h"

namespace primihub {

PartyConfig::PartyConfig(const std::string &node_id, const rpc::Task &task) {
  // Map in protocolbuf may give different sequence when iterate it, so copy
  // content to std::map.
  std::map<std::string, rpc::Node> node_map;
  {
    auto tmp = task.node_map();
    for (auto iter = tmp.begin(); iter != tmp.end(); iter++) {
      node_map[iter->first] = iter->second;
    }
  }

  this->node_id = node_id;
  this->task_id = task.task_id();
  this->job_id = task.job_id();
  this->node_map = std::move(node_map);
}

void Session::start(IOService &ioService, std::string remoteIP, u32 port,
                    SessionMode type, std::string name) {
  TLSContext ctx;
  start(ioService, remoteIP, port, type, ctx, name);
}

void Session::start(IOService &ioService, std::string address, SessionMode host,
                    std::string name) {
  auto vec = split(address, ':');

  auto ip = vec[0];
  auto port = 1212;
  if (vec.size() > 1) {
    std::stringstream ss(vec[1]);
    ss >> port;
  }

  start(ioService, ip, port, host, name);
}

void Session::start(IOService &ioService, std::string ip, u64 port,
                    SessionMode type, TLSContext &tls, std::string name) {
  if (mBase && mBase->mStopped == false)
    throw std::runtime_error("rt error at " LOCATION);
#ifdef ENABLE_WOLFSSL
  if (tls && (tls.mode() != WolfContext::Mode::Both) &&
      (tls.mode() == WolfContext::Mode::Server) !=
          (type == SessionMode::Server))
    throw std::runtime_error("TLS context isServer does not match SessionMode");
#endif

  mBase.reset(new SessionBase(ioService));
  mBase->mIP = std::move(ip);
  mBase->mPort = static_cast<u32>(port);
  mBase->mMode = (type);
  mBase->mIOService = &(ioService);
  mBase->mStopped = (false);
  mBase->mTLSContext = tls;
  mBase->mName = (name);

  if (type == SessionMode::Server) {
    ioService.aquireAcceptor(mBase);
  } else {
    PRNG prng(ioService.getRandom(), sizeof(block) + sizeof(u64));
    mBase->mSessionID = prng.get();
#ifdef ENABLE_WOLFSSL
    mBase->mTLSSessionID = prng.get();
#endif
    boost::asio::ip::tcp::resolver resolver(ioService.mIoService);
    boost::asio::ip::tcp::resolver::query query(
        mBase->mIP, boost::lexical_cast<std::string>(port));
    mBase->mRemoteAddr = *resolver.resolve(query);
  }
}

Session::Session(IOService &ioService, std::string address, SessionMode type,
                 std::string name) {
  start(ioService, address, type, name);
}

Session::Session(IOService &ioService, std::string remoteIP, u32 port,
                 SessionMode type, std::string name) {
  start(ioService, remoteIP, port, type, name);
}

Session::Session(IOService &ioService, std::string remoteIP, u32 port,
                 SessionMode type, TLSContext &ctx, std::string name) {
  start(ioService, remoteIP, port, type, ctx, name);
}

//  Default constructor
Session::Session() {}

Session::Session(const Session &v) : mBase(v.mBase) {
  if (mBase)
    ++mBase->mRealRefCount;
}

Session::Session(const std::shared_ptr<SessionBase> &c) : mBase(c) {
  ++mBase->mRealRefCount;
}

Session::~Session() {
  if (mBase) {
    --mBase->mRealRefCount;
    if (mBase->mRealRefCount == 0)
      mBase->stop();
  }
}

std::string Session::getName() const {
  if (mBase)
    return mBase->mName;
  else
    throw std::runtime_error(LOCATION);
}

u64 Session::getSessionID() const {
  if (mBase)
    return mBase->mSessionID;
  else
    throw std::runtime_error(LOCATION);
}

IOService &Session::getIOService() {
  if (mBase)
    return *mBase->mIOService;
  else
    throw std::runtime_error(LOCATION);
}

Channel Session::addChannel(std::string localName, std::string remoteName) {
  if (mBase == nullptr)
    throw std::runtime_error("Session is not initialized");

  // if the user does not provide a local name, use the following.
  if (localName == "") {
    if (remoteName != "")
      throw std::runtime_error(
          "remote name must be empty is local name is empty. " LOCATION);

    std::lock_guard<std::mutex> lock(mBase->mAddChannelMtx);
    localName = "_autoName_" + std::to_string(mBase->mAnonymousChannelIdx++);
  }

  // make the remote name match the local name if empty
  if (remoteName == "")
    remoteName = localName;

  if (mBase->mStopped == true)
    throw std::runtime_error("rt error at " LOCATION);

  // construct the basic channel. Has no socket.
  Channel chl(*this, localName, remoteName);
  return (chl);
}

void Session::stop() { mBase->stop(); }

bool Session::stopped() const { return mBase->mStopped; }

u32 Session::port() const { return mBase->mPort; }

std::string Session::IP() const { return mBase->mIP; }
bool Session::isHost() const { return mBase->mMode == SessionMode::Server; }

} // namespace primihub
