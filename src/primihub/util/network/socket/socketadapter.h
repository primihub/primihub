// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_SOCKETADAPTER_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_SOCKETADAPTER_H_

#include <iostream>
#include <queue>
#include <mutex>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

#include "src/primihub/common/defines.h"
// #include "src/primihub/util/network/socket/iobuffer.h"
#include "src/primihub/util/util.h"

namespace primihub {

using error_code = boost::system::error_code;
using io_completion_handle = std::function<void(const error_code&,
  u64)>;
using completion_handle = std::function<void(const error_code&)>;

class SocketInterface {
 public:
  // REQURIED: must implement a distructor as it will be called via
  // the SocketInterface
  virtual ~SocketInterface() {}

  // REQUIRED -- buffers contains a list of buffers that are allocated
  // by the caller. The callee should recv data into those buffers. The
  // callee should take a move of the callback fn. When the IO is complete
  // the callee should call the callback fn.
  // @buffers [output]: is the vector of buffers that should be recved.
  // @fn [input]:   A call back that should be called on completion of the IO.
  virtual void async_recv(
    span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) = 0;

  // REQUIRED -- buffers contains a list of buffers that are allocated
  // by the caller. The callee should send the data in those buffers. The
  // callee should take a move of the callback fn. When the IO is complete
  // the callee should call the callback fn.
  // @buffers [input]: is the vector of buffers that should be sent.
  // @fn [input]:   A call back that should be called on completion of the IO
  virtual void async_send(span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) = 0;

  // OPTIONAL -- no-op close is default. Will be called when all
  // Channels that refernece it are destructed/
  virtual void close() {}

  virtual void cancel() {
    std::cout << "Please override SocketInterface::cancel() if you"
    << " want to properly support cancel operations."
    << "Calling std::terminate() " << LOCATION << std::endl;
    std::terminate();
  }

  // OPTIONAL -- no-op close is default. Will be called right after
  virtual void async_accept(completion_handle&& fn) {
    error_code ec;
    fn(ec);
  }

  // OPTIONAL -- no-op close is default. Will be called when all
  // Channels that refernece it are destructed/
  virtual void async_connect(completion_handle&& fn) {
    error_code ec;
    fn(ec);
  }

  // a function that gives this socket access to the IOService.
  // This is called right after being handed to the Channel.
  virtual void setIOService(IOService& ios) {}
};

void post(IOService* ios, std::function<void()>&& fn);

template<typename T>
class SocketAdapter : public SocketInterface {
 public:
  T& mChl;
  IOService* mIos = nullptr;

  SocketAdapter(T& chl) :mChl(chl) {}

  ~SocketAdapter() override {}

  void setIOService(IOService& ios) override { mIos = &ios; }

  void async_send(
    span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) override {
    post(mIos, [this, buffers, fn]() {
      error_code ec;
      u64 bytesTransfered = 0;
      for (u64 i = 0; i < u64(buffers.size()); ++i) {
        try {
          // Use boost conversions to get normal pointer size
          auto data = boost::asio::buffer_cast<u8*>(buffers[i]);
          auto size = boost::asio::buffer_size(buffers[i]);

          // NOTE: I am assuming that this is blocking.
          // Blocking here cause the networking code to deadlock
          // in some senarios. E.g. all threads blocks on recving data
          // that is not being sent since the threads are blocks.
          // Make sure to give the IOService enought threads or make this
          // non blocking somehow.
          mChl.send(data, size);
          bytesTransfered += size;
        } catch (...) {
          ec = boost::system::errc::make_error_code
          (boost::system::errc::io_error);
          break;
        }
      }

      // once all the IO is sent (or error), we should call the callback.
      fn(ec, bytesTransfered);
    });
  }

  void async_recv(
    span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) override {
    post(mIos, [this, buffers, fn]() {
      error_code ec;
      u64 bytesTransfered = 0;
      for (u64 i = 0; i < u64(buffers.size()); ++i) {
        try {
          // Use boost conversions to get normal pointer size
          auto data = boost::asio::buffer_cast<u8*>(buffers[i]);
          auto size = boost::asio::buffer_size(buffers[i]);

          // Note that I am assuming that this is blocking.
          // Blocking here cause the networking code to deadlock
          // in some senarios. E.g. all threads blocks on recving data
          // that is not being sent since the threads are blocks.
          // Make sure to give the IOService enought threads or make this
          // non blocking somehow.
          mChl.recv(data, size);
          bytesTransfered += size;
        } catch (...) {
          ec = boost::system::errc::make_error_code
          (boost::system::errc::io_error);
          break;
        }
      }
      fn(ec, bytesTransfered);
    });
  }

  void cancel() override {
      mChl.asyncCancel([](){});
  }
};

class BasicAdapter {
 public:
  BasicAdapter() = default;
  BasicAdapter(const BasicAdapter&) = delete;
  BasicAdapter(BasicAdapter&&) = delete;

  struct Operation {
    enum Type {
      Recv,
      Send,
      Done
    };

    std::vector<span<u8>> mBuffers;
    Type mType;

    io_completion_handle mFn;

    void finished() {
      auto size = 0ull;
      for (auto& b : mBuffers)
        size += b.size();

      mFn({}, size);
      mFn = {};
    }

    void cancel() {
      error_code ec =
        boost::system::errc::make_error_code
        (boost::system::errc::operation_canceled);
      mFn(ec, 0);
      mFn = {};
    }
  };

  std::condition_variable mCV;
  std::mutex mMtx;

  std::queue<Operation> mOps;

  class Socket : public SocketInterface {
   public:
    BasicAdapter* mSock;

    Socket(BasicAdapter* sock)
      :mSock(sock) {}

    //////////////////////////////////   REQUIRED ///////////////////////////
    // REQURIED: must implement a distructor as it will be called via
    // the SocketAdpater
    ~Socket() {}


    // REQUIRED -- buffers contains a list of buffers that are allocated
    // by the caller. The callee should recv data into those buffers. The
    // callee should take a move of the callback fn. When the IO is complete
    // the callee should call the callback fn.
    // @buffers [output]: is the vector of buffers that should be recved.
    // @fn [input]:   A call back that should be called on completion of the IO.
    void async_recv(
      span<boost::asio::mutable_buffer> buffers,
      io_completion_handle&& fn) override {
      Operation op;

      for (auto buffer : buffers) {
        auto b = span<u8>(boost::asio::buffer_cast<u8*>(buffer),
          boost::asio::buffer_size(buffer));
        op.mBuffers.push_back(b);
      }
      op.mType = Operation::Recv;
      op.mFn = std::move(fn);

      {
        std::unique_lock<std::mutex> lk(mSock->mMtx);
        mSock->mOps.emplace(std::move(op));
      }
      mSock->signal();
    }

    // REQUIRED -- buffers contains a list of buffers that are allocated
    // by the caller. The callee should send the data in those buffers. The
    // callee should take a move of the callback fn. When the IO is complete
    // the callee should call the callback fn.
    // @buffers [input]: is the vector of buffers that should be sent.
    // @fn [input]:   A call back that should be called on completion of the IO
    void async_send(
      span<boost::asio::mutable_buffer> buffers,
      io_completion_handle&& fn) override {
      Operation op;
      for (auto buffer : buffers) {
        auto b = span<u8>(boost::asio::buffer_cast<u8*>(buffer),
          boost::asio::buffer_size(buffer));
        op.mBuffers.push_back(b);
      }
      op.mType = Operation::Send;
      op.mFn = std::move(fn);

      {
        std::unique_lock<std::mutex> lk(mSock->mMtx);
        mSock->mOps.emplace(std::move(op));
      }
      mSock->signal();
    }

    void close() override {
      Operation op;
      op.mType = Operation::Done;

      {
        std::unique_lock<std::mutex> lk(mSock->mMtx);
        mSock->mOps.emplace(std::move(op));
      }
      mSock->signal();
    };

    void cancel() override {
      std::cout << "Please override SocketInterface::cancel() if you"
        << " want to properly support cancel operations. "
        << "Calling std::terminate() " << LOCATION << std::endl;
      std::terminate();
    };
  };

  Socket* getSocket() {
    return new Socket(this);
  }

  void signal() {
    mCV.notify_all();
  }

  Operation getOp() {
    std::unique_lock<std::mutex> lk(mMtx);

    if (mOps.size() == 0)
      mCV.wait(lk, [this]() {return mOps.size() > 0; });

    auto op = std::move(mOps.front());
    mOps.pop();
    return op;
  }
};

class BoostSocketInterface : public SocketInterface {
 public:
  boost::asio::ip::tcp::socket mSock;

#ifndef BOOST_ASIO_HAS_MOVE
#error "require move"
#endif

  BoostSocketInterface(boost::asio::ip::tcp::socket&& ios)
    : mSock(std::forward<boost::asio::ip::tcp::socket>(ios))
  {}

  ~BoostSocketInterface() override {
    close();
  }

  void close() override {
    boost::system::error_code ec;
    mSock.close(ec);
    if (ec)
      std::cout <<"BoostSocketInterface::close() error: "
        << ec.message() << std::endl;
  }

  void cancel() override {
    boost::system::error_code ec;
#if defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC >= 1400) \
&& (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600) \
&& !defined(BOOST_ASIO_ENABLE_CANCELIO)
      mSock.close(ec);
#else
      mSock.cancel(ec);
#endif
    if (ec)
      std::cout <<"BoostSocketInterface::cancel() error: "
        << ec.message() << std::endl;
  }

  void async_recv(span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) override {
      boost::asio::async_read(mSock, buffers,
        std::forward<io_completion_handle>(fn));
  }

  void async_send(span<boost::asio::mutable_buffer> buffers,
    io_completion_handle&& fn) override {
      boost::asio::async_write(mSock, buffers,
        std::forward<io_completion_handle>(fn));
  }
};

}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_SOCKETADAPTER_H_
