// Copyright [2021] <primihub.com>
#include "src/primihub/util/network/socket/ioservice.h"

namespace primihub {

void post(IOService* ios, std::function<void()>&& fn) {
  ios->mIoService.post(std::move(fn));
}

#ifdef ENABLE_NET_LOG
#define LOG_MSG(m) mLog.push(m)
#define IF_LOG(m) m
#else
#define LOG_MSG(m)
#define IF_LOG(m)
#endif

Work::Work(IOService& ios, std::string reason)
  : mWork(new boost::asio::io_service::work(ios.mIoService))
  , mReason(reason)
  , mIos(ios) {
#ifdef ENABLE_NET_LOG
  std::lock_guard<std::mutex> lock(mIos.mWorkerMtx);
  ios.mWorkerLog.insert({mWork.get(), reason});
#endif
}
Work::~Work() {
  reset();
}

void Work::reset() {
  if (mWork) {
#ifdef ENABLE_NET_LOG
    std::lock_guard<std::mutex> lock(mIos.mWorkerMtx);
    auto iter = mIos.mWorkerLog.find(mWork.get());
    mIos.mWorkerLog.erase(iter);
#endif
    mWork.reset(nullptr);
  }
}

// extern void split(const std::string &s, char delim,
// std::vector<std::string> &elems);
// extern std::vector<std::string> split(const std::string &s, char delim);

block IOService::getRandom() {
  return AES_Type(mRandSeed).ecbEncBlock(toBlock(mSeedIndex++));
}

IOService::IOService(u64 numThreads)
  :
  mRandSeed(sysRandomSeed()),
  mSeedIndex(0),
  mIoService(),
  mStrand(mIoService.get_executor()),
  mWorker(*this, "ios") {
  // if they provided 0, the use the number of processors worker threads
  numThreads = (numThreads) ? numThreads : std::thread::hardware_concurrency();
  mWorkerThrds.resize(numThreads);
  u64 i = 0;
  // Create worker threads based on the number of processors available on the
  // system. Create two worker threads for each processor
  for (auto& thrdProm : mWorkerThrds) {
    auto& thrd = thrdProm.first;
    auto& prom = thrdProm.second;

    // Create a server worker thread and pass the completion port to the thread
    thrd = std::thread([this, i, &prom]() {
      setThreadName("io_Thrd_" + std::to_string(i));
      mIoService.run();
      prom.set_value();
      // std::cout << "io_Thrd_" + std::to_string(i) << " closed" << std::endl;
    });
    ++i;
  }
}

IOService::~IOService() {
  // block until everything has shutdown.
  stop();
}


void IOService::workUntil(std::future<void>& f) {
  while (f.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
    mIoService.poll_one();
}


void IOService::stop() {
  // Skip if its already shutdown.
  if (mStopped == false) {
    mStopped = true;

    // tell all the acceptor threads to stop accepting new connections.
    for (auto& accptr : mAcceptors) {
      accptr.stop();
    }
    // delete all of their state.
    mAcceptors.clear();

    mWorker.reset();

    // we can now join on them.
    for (auto& thrd : mWorkerThrds) {
      auto res = thrd.second.get_future().wait_for(std::chrono::seconds(3));
      if (res != std::future_status::ready && mPrint) {
#ifdef ENABLE_NET_LOG
        std::lock_guard<std::mutex> lock(mWorkerMtx);
        if (mWorkerLog.size()) {
          lout << "IOSerive::stop() is waiting for: \n";
          for (auto& v : mWorkerLog)
            lout << '\t' << v.second << "\n";
          lout << std::flush;
        }
#else
        lout << "IOSerive::stop() is waiting for work to finish" << std::endl;
#endif
      }

      thrd.first.join();
    }
    // clean their state.
    mWorkerThrds.clear();
  }
}

void IOService::printError(std::string msg) {
  if (mPrint)
    std::cerr << msg << std::endl;
}

void IOService::showErrorMessages(bool v) {
  mPrint = v;
}

void IOService::aquireAcceptor(std::shared_ptr<SessionBase>& session) {
  // std::atomic<bool> flag(false);
  std::list<Acceptor>::iterator acceptorIter;
  std::promise<void> p;
  // std::future<std::list<Acceptor>::iterator> f = p.get_future();
  // boost::asio::post
  boost::asio::dispatch(mStrand, [&]() {
    // see if there already exists an acceptor that this endpoint can use.
    acceptorIter = std::find_if(mAcceptors.begin(), mAcceptors.end(),
      [&](const Acceptor& acptr) {
        return acptr.mPort == session->mPort;
    });

    if (acceptorIter == mAcceptors.end()) {
      // an acceptor does not exist for this port. Lets create one.
      mAcceptors.emplace_back(*this);
      acceptorIter = mAcceptors.end();
      --acceptorIter;
      acceptorIter->mPort = session->mPort;
    }

    acceptorIter->asyncSubscribe(session, [&](const error_code& ec) {
      if (ec)
        p.set_exception(std::make_exception_ptr(
          std::runtime_error(ec.message())));
      else
        p.set_value();
      });
  });

  TODO("do something else that does not require workUntil.");
  auto f = p.get_future();
  // contribute this thread to running the dispatch. Sometimes needed.
  workUntil(f);

  f.get();
}

}  // namespace primihub
