// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_H_

#include <thread>
#include <chrono>
#include <future>
#include <ostream>
#include <list>
#include <deque>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/network/socket/iobuffer.h"
#include "src/primihub/util/network/socket/socketadapter.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/channel_base.h"
#include "src/primihub/util/network/socket/chl_operation.h"
#include "src/primihub/util/network/socket/tls.h"

#ifdef ENABLE_NET_LOG
#include "src/primihub/util/log.h"
#endif

namespace primihub {

class ChannelBase;
class BoostSocketInterface;
class SocketInterface;

class Session;
struct SessionBase;
struct StartSocketOp;
class Channel;
class SocketInterface;
class SendOperation;
class RecvOperation;

struct osuCryptoErrCategory;

struct osuCryptoErrCategory : boost::system::error_category {
  const char* name() const noexcept override {
    return "osuCrypto";
  }

  std::string message(int ev) const override {
    switch (static_cast<Errc_Status>(ev)) {
      case Errc_Status::success:
        return "Success";
      default:
        return "(unrecognized error)";
    }
  }
};

const osuCryptoErrCategory theCategory{};

using error_code = boost::system::error_code;

inline error_code chl_make_error_code(Errc_Status e) {
  return { static_cast<int>(e), theCategory };
}

using io_completion_handle = std::function<
  void(const error_code&, u64)>;

using completion_handle = std::function<
  void(const error_code&)>;




template<typename T>
inline u8* channelBuffData(const T& container) {
  return (u8*)container.data();
}

template<typename T>
inline u64 channelBuffSize(const T& container) {
  return container.size() * sizeof(typename  T::value_type);
}

template<typename T>
inline bool channelBuffResize(T& container, u64 size) {
  if (size % sizeof(typename  T::value_type)) return false;

  try {
    container.resize(size / sizeof(typename  T::value_type));
  }
  catch (...) {
    return false;
  }
  return true;
}

template<typename, typename T>
struct has_resize {
    static_assert(
        std::integral_constant<T, false>::value,
        "Second template parameter needs to be of function type.");
};

// specialization that does the checking
template<typename C, typename Ret, typename... Args>
struct has_resize<C, Ret(Args...)> {
private:
    template<typename T>
    static constexpr auto check(T*)
        -> typename
        std::is_same<
        decltype(std::declval<T>().resize(std::declval<Args>()...)),
        Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        >::type;  // attempt to call it and see if the return type is correct

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(0)) type;

public:
    static constexpr bool value = type::value;
};

#define _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING

/// type trait that defines what is considered a STL like Container
/// 
/// Must have the following member types:  pointer, size_type, value_type
/// Must have the following member functions:
///    * Container::pointer Container::data();
///    * Container::size_type Container::size();
/// Must contain Plain Old Data:
///    * std::is_pod<Container::value_type>::value == true
template<typename Container>
using is_container =
    std::is_same<typename std::enable_if<
    std::is_convertible<
    typename Container::pointer,
    decltype(std::declval<Container>().data())>::value&&
    std::is_convertible<
    typename Container::size_type,
    decltype(std::declval<Container>().size())>::value&&
    std::is_pod<typename Container::value_type>::value&&
    std::is_pod<Container>::value == false>::type
    ,
    void>;

// A class template that allows fewer than the specified number of bytes to be received. 
template<typename T>
class ReceiveAtMost
{
public:
    using pointer = T *;
    using value_type = T;
    using size_type = u64;

    T* mData;
    u64 mMaxReceiveSize, mTrueReceiveSize;


    // A constructor that takes the loction to be written to and 
    // the maximum number of T's that should be written. 
    // Call 
    ReceiveAtMost(T* dest, u64 maxReceiveCount)
        : mData(dest)
        , mMaxReceiveSize(maxReceiveCount)
        , mTrueReceiveSize(0)
    {}


    u64 size() const
    {
        if (mTrueReceiveSize)
            return mTrueReceiveSize;
        else
            return mMaxReceiveSize;
    }

    const T* data() const { return mData; }
    T* data() { return mData; }

    void resize(u64 size)
    {
        if (size > mMaxReceiveSize) throw std::runtime_error(LOCATION);
        mTrueReceiveSize = size;
    }

    u64 receivedSize() const
    {
        return mTrueReceiveSize;
    }
};

static_assert(is_container<ReceiveAtMost<u8>>::value, "sss");
static_assert(has_resize<ReceiveAtMost<u8>, void(typename ReceiveAtMost<u8>::size_type)>::value, "sss");


// Channel is the standard interface use to send data over the network.
class Channel {
 public:
  // The default constructors
  Channel() = default;
  Channel(const Channel& copy);
  Channel(Channel&& move) = default;

  // Special constructor used to construct a Channel from some socket.
  // Note, Channel takes ownership of the socket and will delete it
  // when done.
  Channel(IOService& ios, SocketInterface* sock);

  ~Channel();

  // Default move assignment
  Channel& operator=(Channel&& move);

  // Default copy assignment
  Channel& operator=(const Channel& copy);


  //////////////////////////////////////////////////////////////////////////////
        //						   Sending interface								//
        //////////////////////////////////////////////////////////////////////////////

        // Sends length number of T pointed to by src over the network. The type T 
        // must be POD. Returns once all the data has been sent.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            send(const T* src, u64 length);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns once all the data has been sent.
        template <class T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            send(const T& buf);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns once all the data has been sent.
        template <class Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            send(const Container& buf);


        // Sends the data in buf over the network. The type T must be POD.
        // Returns before the data has been sent. The life time of the data must be 
        // managed externally to ensure it lives longer than the async operations.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            asyncSend(const T* data, u64 length);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns before the data has been sent. 
        // The life time of the data must be managed externally to ensure it lives 
        // longer than the async operations.  callback is a function that is called 
        // from another thread once the send operation has succeeded.
        template<typename Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(Container&& data, std::function<void()> callback);


        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns before the data has been sent. 
        // The life time of the data must be managed externally to ensure it lives 
        // longer than the async operations.  callback is a function that is called 
        // from another thread once the send operation has completed.
        template<typename Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(Container&& data, std::function<void(const error_code&)> callback);


        // Sends the data in buf over the network. The type T must be POD.
        // Returns before the data has been sent. The life time of the data must be 
        // managed externally to ensure it lives longer than the async operations.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            asyncSend(const T& data);

        // Sends the data in buf over the network. The type T must be POD.
        // Returns before the data has been sent. The life time of the data must be 
        // managed externally to ensure it lives longer than the async operations.
        template<typename Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(const Container& data);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns before the data has been sent. 
        template <class Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(Container&& c);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns before the data has been sent. 
        template <class Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(std::unique_ptr<Container> buffer);

        // Sends the data in buf over the network. The type Container  must meet the 
        // requirements defined in IoBuffer.h. Returns before the data has been sent. 
        template <class Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSend(std::shared_ptr<Container> buffer);


        // Sends the data in buf over the network. The type T must be POD.
        // Returns before the data has been sent. The life time of the data must be 
        // managed externally to ensure it lives longer than the async operations.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
            asyncSendFuture(const T* data, u64 length);


        // Performs a data copy and then sends the data in buf over the network. 
        //  The type T must be POD. Returns before the data has been sent. 
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            asyncSendCopy(const T& buff);

        // Performs a data copy and then sends the data in buf over the network. 
        //  The type T must be POD. Returns before the data has been sent. 
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            asyncSendCopy(const T* bufferPtr, u64 length);

        // Performs a data copy and then sends the data in buf over the network. 
        // The type Container must meet the requirements defined in IoBuffer.h. 
        // Returns before the data has been sent. 
        template <typename  Container>
        typename std::enable_if<is_container<Container>::value, void>::type
            asyncSendCopy(const Container& buf);


        //////////////////////////////////////////////////////////////////////////////
        //						   Receiving interface								//
        //////////////////////////////////////////////////////////////////////////////

        // Receive data over the network. If possible, the container c will be resized
        // to fit the data. The function returns once all the data has been received.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value&&
            has_resize<Container, void(typename Container::size_type)>::value, void>::type
            recv(Container& c)
        {
            asyncRecv(c).get();
        }

        // Receive data over the network. The container c must be the correct size to 
        // fit the data. The function returns once all the data has been received.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value &&
            !has_resize<Container, void(typename Container::size_type)>::value, void>::type
            recv(Container& c)
        {
            asyncRecv(c).get();
        }

        // Receive data over the network. The function returns once all the data 
        // has been received.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            recv(T* dest, u64 length);

        // Receive data over the network. The function returns once all the data 
        // has been received.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, void>::type
            recv(T& dest) { recv(&dest, 1); }

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
            asyncRecv(T* dest, u64 length);

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set and the callback fn is called.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
            asyncRecv(T* dest, u64 length, std::function<void()> fn);

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set.
        template<typename T>
        typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
            asyncRecv(T& dest) { return asyncRecv(&dest, 1); }

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set. The container must be the correct size to fit the data received.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value &&
            !has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
            asyncRecv(Container & c);

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set. The container is resized to fit the data.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value&&
            has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
            asyncRecv(Container & c);

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set and the callback fn is called. The container must be the correct 
        // size to fit the data received.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value&&
            has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
            asyncRecv(Container & c, std::function<void()> fn);

        // Receive data over the network asynchronously. The function returns right away,
        // before the data has been received. When all the data has benn received the 
        // future is set and the callback fn is called. The container must be the correct 
        // size to fit the data received.
        template <class Container>
        typename std::enable_if<
            is_container<Container>::value&&
            has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
            asyncRecv(Container & c, std::function<void(const error_code&)> fn);


  // The handle for this channel. Both ends will always have the same name.
  std::string getName() const;

  // Returns the name of the remote endpoint.
  std::string getRemoteName() const;

  // Return the name of the endpoint of this channel has once.
  Session getSession() const;

  // Sets the data send and recieved counters to zero.
  void resetStats();

  // Returns the amount of data that this channel has sent since it was created or when resetStats() was last called.
  u64 getTotalDataSent() const;

  // Returns the amount of data that this channel has sent since it was created or when resetStats() was last called.
  u64 getTotalDataRecv() const;

  // Returns the maximum amount of data that this channel has queued up to send since it was created or when resetStats() was last called.
  //u64 getMaxOutstandingSendData() const;

  // Returns whether this channel is open in that it can send/receive data
  bool isConnected();

  // A blocking call that waits until the channel is open in that it can send/receive data
  // Returns if the connection has been made. Always true if no timeout is provided.
  bool waitForConnection(std::chrono::milliseconds timeout);

  // A blocking call that waits until the channel is open in that it can send/receive data
  // Returns if the connection has been made. 
  void waitForConnection();

  void onConnect(completion_handle handle);

  // Close this channel to denote that no more data will be sent or received.
  // blocks until all pending operations have completed.
  void close();

  // Aborts all current operations (connect, send, receive).
  void cancel(bool close = true);

  void asyncClose(std::function<void()> completionHandle);

  void asyncCancel(std::function<void()> completionHandle, bool close = true);

  std::string commonName();

  std::shared_ptr<ChannelBase> mBase;

  operator bool() const {
    return static_cast<bool>(mBase);
  }

 private:
    friend class IOService;
    friend class Session;
    Channel(Session& endpoint, std::string localName, std::string remoteName);
};

inline std::ostream& operator<< (std::ostream& o, const Channel_Status& s) {
    switch (s) {
    case Channel_Status::Normal:
        o << "Status::Normal";
        break;
    case Channel_Status::Closing:
        o << "Status::Closing";
        break;
    case Channel_Status::Closed:
        o << "Status::Closed";
        break;
    case Channel_Status::Canceling:
        o << "Status::Canceling";
        break;
    default:
        o << "Status::??????";
        break;
    }
    return o;
}

class SocketConnectError : public std::runtime_error {
 public:
    SocketConnectError(const std::string& reason)
        :std::runtime_error(reason) {}
};


class CanceledOperation : public std::runtime_error {
 public:
  CanceledOperation(const std::string& str)
    : std::runtime_error(str) {}
};

class ChlOperation {
 public:
  ChlOperation() = default;
  ChlOperation(ChlOperation&& copy) = default;
  ChlOperation(const ChlOperation& copy) = default;

  virtual ~ChlOperation() {}

  // perform the operation
  virtual void asyncPerform(ChannelBase* base,
    io_completion_handle&& completionHandle) = 0;

  // cancel the operation, this is called durring or after asyncPerform 
  // has been called. If after then it should be a no-op.
  virtual void asyncCancelPending(ChannelBase* base,
    const error_code& ec) = 0;

  // cancel the operation, this is called before asyncPerform. 
  virtual void asyncCancel(ChannelBase* base, const error_code& ec,
    io_completion_handle&& completionHandle) = 0;

  virtual std::string toString() const = 0;

#ifdef ENABLE_NET_LOG
  u64 mIdx = 0;
  Log* mLog = nullptr;
  void log(std::string msg) { if (mLog) mLog->push(msg); }
#endif
};

class RecvOperation : public ChlOperation { };
class SendOperation : public ChlOperation { };

template<typename Base>
struct BaseCallbackOp : public Base {
  std::function<void()> mCallback;

  BaseCallbackOp(const std::function<void()>& cb)
    : mCallback(cb) {}
  BaseCallbackOp(std::function<void()>&& cb) 
    : mCallback(std::move(cb)){}

  void asyncPerform(ChannelBase* base,
    io_completion_handle&& completionHandle) override {
    error_code ec = primihub::chl_make_error_code(Errc_Status::success);
    mCallback();
    completionHandle(ec, 0);
  }

  void asyncCancelPending(ChannelBase* base, const error_code& ec) override {}

  void asyncCancel(ChannelBase* base, const error_code&,
    io_completion_handle&& completionHandle) override {
    asyncPerform(base, std::forward<io_completion_handle>(completionHandle));
  }

  std::string toString() const override {
    return std::string("CallbackOp #")
#ifdef ENABLE_NET_LOG
      + std::to_string(Base::mIdx)
#endif
    ;
  }
};

using RecvCallbackOp = BaseCallbackOp<RecvOperation>;
using SendCallbackOp = BaseCallbackOp<SendOperation>;

using size_header_type = u32;

// A class for sending or receiving data over a channel. 
// Datam sent/received with this type sent over the network 
// with a header denoting its size in bytes.
class BasicSizedBuff {
 public:
  BasicSizedBuff(BasicSizedBuff&& v) {
    mHeaderSize = v.mHeaderSize;
    mBuff = v.mBuff;
    v.mBuff = {};
  }

  BasicSizedBuff() = default;

  BasicSizedBuff(const u8* data, u64 size)
    : mHeaderSize(size_header_type(size))
    , mBuff{ (u8*)data,  span<u8>::size_type(size) } {
    Expects(size < std::numeric_limits<size_header_type>::max());
  }

  void set(const u8* data, u64 size) {
    Expects(size < std::numeric_limits<size_header_type>::max());
    mBuff = { (u8*)data, span<u8>::size_type(size) };
  }

  inline u64 getHeaderSize() const { return mHeaderSize; }
  inline u64 getBufferSize() const { return mBuff.size(); }
  inline u8* getBufferData() { return mBuff.data(); }

  inline std::array<boost::asio::mutable_buffer, 2> getSendBuffer() {
    Expects(mBuff.size());
    mHeaderSize = size_header_type(mBuff.size());
    return { { getRecvHeaderBuffer(), getRecvBuffer() } };
  }

  inline boost::asio::mutable_buffer getRecvHeaderBuffer() {
    return boost::asio::mutable_buffer(&mHeaderSize, sizeof(size_header_type));
  }

  inline boost::asio::mutable_buffer getRecvBuffer() {
    return boost::asio::mutable_buffer(mBuff.data(), mBuff.size());
  }

 protected:
  size_header_type mHeaderSize = 0;
  span<u8> mBuff;
};

class FixedSendBuff : public BasicSizedBuff, public SendOperation {
 public:
  FixedSendBuff() = default;
  FixedSendBuff(const u8* data, u64 size)
    : BasicSizedBuff(data, size)
  {}

  FixedSendBuff(FixedSendBuff&& v) = default;

  void asyncPerform(ChannelBase* base,
    io_completion_handle && completionHandle) override;

  void asyncCancelPending(ChannelBase* base, const error_code& ec) override {}

  void asyncCancel(ChannelBase* base, const error_code&,
    io_completion_handle&& completionHandle) override {
    error_code ec = primihub::chl_make_error_code(Errc_Status::success);
    completionHandle(ec , 0);
  }

  std::string toString() const override;
};

template <typename F>
class MoveSendBuff : public FixedSendBuff {
 public:
  MoveSendBuff() = delete;
  F mObj;

  MoveSendBuff(F&& obj) : mObj(std::move(obj)) {
    // set must be called after the move in case channelBuffData(mObj) != channelBuffData(obj)
    set(channelBuffData(mObj), channelBuffSize(mObj));
  }

  MoveSendBuff(MoveSendBuff&& v) : MoveSendBuff(std::move(v.mObj)) {}
};

template <typename T>
class MoveSendBuff<std::unique_ptr<T>> : public FixedSendBuff {
 public:
  MoveSendBuff() = delete;
  typedef std::unique_ptr<T> F;
  F mObj;
  MoveSendBuff(F&& obj)
    : FixedSendBuff(channelBuffData(*obj), channelBuffSize(*obj))
    , mObj(std::move(obj)) {}

  MoveSendBuff(MoveSendBuff<std::unique_ptr<T>>&& v)
    : MoveSendBuff(std::move(v.mObj)) {}
};

template <typename T>
class MoveSendBuff<std::shared_ptr<T>> :public FixedSendBuff {
 public:
  MoveSendBuff() = delete;
  typedef std::shared_ptr<T> F;
  F mObj;
  MoveSendBuff(F&& obj)
    : FixedSendBuff(channelBuffData(*obj), channelBuffSize(*obj))
    , mObj(std::move(obj)) {}

  MoveSendBuff(MoveSendBuff<std::unique_ptr<T>>&& v)
    : MoveSendBuff(std::move(v.mObj)) {}
};

template <typename F>
class  RefSendBuff :public FixedSendBuff {
 public:
  const F& mObj;
  RefSendBuff(const F& obj)
    : FixedSendBuff(channelBuffData(*obj), channelBuffSize(*obj))
    , mObj(obj) {}

  RefSendBuff(RefSendBuff<F>&& v)
    :RefSendBuff(v.obj) {}
};

class FixedRecvBuff : public BasicSizedBuff, public RecvOperation {
 public:
  io_completion_handle mComHandle;
  ChannelBase* mBase = nullptr;
  std::promise<void> mPromise;

  FixedRecvBuff(FixedRecvBuff&& v)
    : BasicSizedBuff(v.getBufferData(), v.getBufferSize())
    , RecvOperation(static_cast<RecvOperation&&>(v))
    , mComHandle(std::move(v.mComHandle))
    , mBase(v.mBase)
    , mPromise(std::move(v.mPromise)) {}

  FixedRecvBuff(std::future<void>& fu) {
    fu = mPromise.get_future();
  }

  FixedRecvBuff(const u8* data, u64 size, std::future<void>& fu)
    : BasicSizedBuff(data, size) {
    fu = mPromise.get_future();
  }

  void asyncPerform(ChannelBase* base,
    io_completion_handle&& completionHandle) override;

  void asyncCancel(ChannelBase* base, const error_code& ec,
    io_completion_handle&& completionHandle) override {
    mPromise.set_exception(std::make_exception_ptr(
      std::runtime_error(ec.message())));
    completionHandle(ec, 0);
  }

  void asyncCancelPending(ChannelBase* base, const error_code& ec) override {}

  std::string toString() const override;

  virtual void resizeBuffer(u64) {}
};

template <typename F>
class RefRecvBuff : public FixedRecvBuff {
 public:
  const F& mObj;
  RefRecvBuff(const F& obj, std::future<void>& fu)
    : FixedRecvBuff(fu) , mObj(obj) {
    // set must be called after the move in case channelBuffData(mObj) != channelBuffData(obj)
    set(channelBuffData(mObj), channelBuffSize(mObj));
  }

  RefRecvBuff(RefRecvBuff<F>&& v)
    : FixedRecvBuff(std::move(v)) , mObj(v.mObj) {}
};

template <typename F>
class ResizableRefRecvBuff : public FixedRecvBuff {
 public:
  ResizableRefRecvBuff() = delete;
  F& mObj;

  ResizableRefRecvBuff(F& obj, std::future<void>& fu)
    : FixedRecvBuff(fu) , mObj(obj) {}

  ResizableRefRecvBuff(ResizableRefRecvBuff<F>&& v)
    : FixedRecvBuff(std::move(v)) , mObj(v.mObj) {}

  virtual void resizeBuffer(u64 size) override {
    channelBuffResize(mObj, size);
    set((u8*)channelBuffData(mObj), channelBuffSize(mObj));
  }
};

boost::asio::io_context& getIOService(ChannelBase* base);

template< typename T>
class WithCallback : public T {
 public:
  template<typename CB, typename... Args>
  WithCallback(CB&& cb, Args&& ... args)
    : T(std::forward<Args>(args)...)
    , mCallback(std::forward<CB>(cb)) {}

  WithCallback(WithCallback<T>&& v)
    : T(std::move(v))
    , mCallback(std::move(v.mCallback)) {}
  boost::variant<
    std::function<void()>,
    std::function<void(const error_code&)>>mCallback;
  io_completion_handle mWithCBCompletionHandle;

  void asyncPerform(ChannelBase* base,
    io_completion_handle&& completionHandle) override {
    mWithCBCompletionHandle = std::move(completionHandle);

    T::asyncPerform(base,
      [this, base](const error_code& ec, u64 bytes) mutable {
      if (mCallback.which() == 0) {
        auto& c = boost::get<std::function<void()>>(mCallback);
        if (c) {
          boost::asio::post(getIOService(base).get_executor(),
            std::move(c));
        }
      } else {
        auto& c = boost::get<std::function<void(const error_code&)>>
          (mCallback);
        if (c) {
          boost::asio::post(getIOService(base).get_executor(),
            [cc = std::move(c), ec](){
              cc(ec);
          });
        }
      }

      mWithCBCompletionHandle(ec, bytes);
    });
  }

  void asyncCancel(ChannelBase* base, const error_code& ec,
    io_completion_handle&& completionHandle) override {
    if (mCallback.which() == 1) {
      auto& c = boost::get<std::function<void(const error_code&)>>(mCallback);
      if (c)
        c(ec);
      c = {};
    }

    T::asyncCancel(base, ec,
      std::forward<io_completion_handle>(completionHandle));
  }

  void asyncCancelPending(ChannelBase* base, const error_code& ec) override {}
};

template< typename T>
class WithPromise : public T {
 public:
  template<typename... Args>
  WithPromise(std::future<void>& f, Args&& ... args)
    : T(std::forward<Args>(args)...) {
    f = mPromise.get_future();
  }

  WithPromise(WithPromise<T>&& v)
    : T(std::move(v))
    , mPromise(std::move(v.mPromise))
  {}

  std::promise<void> mPromise;
  io_completion_handle mWithPromCompletionHandle;

  void asyncPerform(ChannelBase* base,
    io_completion_handle&& completionHandle) override {
    mWithPromCompletionHandle = std::move(completionHandle);

    T::asyncPerform(base,
      [this](const error_code& ec, u64 bytes) mutable {
      if (ec)
        mPromise.set_exception(std::make_exception_ptr(CanceledOperation("network send error: " + ec.message() + "\n" LOCATION)));
      else
        mPromise.set_value();

      mWithPromCompletionHandle(ec, bytes);
    });
  }

  void asyncCancel(ChannelBase* base, const error_code& ec,
    io_completion_handle&& completionHandle) override {
      mPromise.set_exception(std::make_exception_ptr(CanceledOperation(ec.message())));
      T::asyncCancel(base, ec,
        std::forward<io_completion_handle>(completionHandle));
  }
};

struct StartSocketOp {
  StartSocketOp(std::shared_ptr<ChannelBase> chl);

  void setHandle(io_completion_handle&& completionHandle, bool sendOp);
  void cancelPending(bool sendOp);
  void connectCallback(const error_code& ec);
  bool canceled() const;
  void asyncConnectToServer();
  void recvServerMessage();
  void sendConnectionString();
  void retryConnect(const error_code& ec);

  char mRecvChar;
  void setSocket(std::unique_ptr<BoostSocketInterface> socket,
    const error_code& ec);

  void finalize(std::unique_ptr<SocketInterface> sock, error_code ec);

  void addComHandle(completion_handle&& comHandle) {
    boost::asio::dispatch(mStrand,
      [this, ch = std::forward<completion_handle>(comHandle)]() mutable {
        if (mFinalized) {
            ch(mEC);
        } else {
            mComHandles.emplace_back(std::forward<completion_handle>(ch));
        }
    });
  }

  boost::asio::deadline_timer mTimer;

  enum class ComHandleStatus { Uninit, Init, Eval };
  ComHandleStatus mSendStatus = ComHandleStatus::Uninit;
  ComHandleStatus mRecvStatus = ComHandleStatus::Uninit;

  boost::asio::strand<boost::asio::io_context::executor_type>& mStrand;
  std::vector<u8> mSendBuffer;
  std::unique_ptr<BoostSocketInterface> mSock;

#ifdef ENABLE_WOLFSSL
  void validateTLS(const error_code& ec);
  std::unique_ptr<TLSSocket> mTLSSock;
  block mTLSSessionIDBuf;
#endif
  double mBackoff = 1;
  bool mFinalized = false, mCanceled = false, mIsFirst;
  error_code mEC, mConnectEC;
  ChannelBase* mChl;
  io_completion_handle mSendComHandle, mRecvComHandle;
  std::list<completion_handle> mComHandles;
};


struct StartSocketSendOp : public SendOperation {
  StartSocketOp* mBase;

  StartSocketSendOp(StartSocketOp* base) : mBase(base) {}

  void asyncPerform(ChannelBase*, io_completion_handle &&completionHandle) override {
    mBase->setHandle(std::forward<io_completion_handle>(completionHandle), true);
  }

  void asyncCancelPending(ChannelBase* base, const error_code& ec)override {
    mBase->cancelPending(true); 
  }

  void asyncCancel(ChannelBase* base, const error_code& ec,io_completion_handle&& completionHandle) override { 
    mBase->setHandle(std::forward<io_completion_handle>(completionHandle), true);
    mBase->cancelPending(true); 
  }

  std::string toString() const override {
      return std::string("StartSocketSendOp # ")
#ifdef ENABLE_NET_LOG
        + std::to_string(mIdx)
#endif
        ;
  }
};

struct StartSocketRecvOp : public RecvOperation {
  StartSocketOp* mBase;
  StartSocketRecvOp(StartSocketOp* base)
      : mBase(base) {}

  void asyncPerform(ChannelBase* base, io_completion_handle&& completionHandle) override {
      mBase->setHandle(std::forward<io_completion_handle>(completionHandle), false);
  }

  void asyncCancelPending(ChannelBase* base, const error_code& ec)override { 
    mBase->cancelPending(false); 
  }

  void asyncCancel(ChannelBase* base, const error_code& ec,io_completion_handle&& completionHandle) override { 
    mBase->setHandle(std::forward<io_completion_handle>(completionHandle), false);
    mBase->cancelPending(false);
  }

  std::string toString() const override {
    return std::string("StartSocketRecvOp # ")
#ifdef ENABLE_NET_LOG
        + std::to_string(mIdx)
#endif
    ;
  }
};



// The Channel base class the actually holds a socket. 
class ChannelBase : public    std::enable_shared_from_this<ChannelBase> {
 public:
  ChannelBase(Session& endpoint, std::string localName,
    std::string remoteName);
  ChannelBase(IOService& ios, SocketInterface* sock);
  ~ChannelBase();

  IOService& mIos;
  Work mWork;
  std::unique_ptr<StartSocketOp> mStartOp;

  std::shared_ptr<SessionBase> mSession;
  std::string mRemoteName, mLocalName;

  Channel_Status mStatus = Channel_Status::Normal;

  std::atomic<u32> mChannelRefCount;

  std::shared_ptr<ChannelBase> mRecvLoopLifetime, mSendLoopLifetime;

  bool recvSocketAvailable() const {
    return !mRecvLoopLifetime;
  }

  bool sendSocketAvailable() const {
    return !mSendLoopLifetime;
  }

  bool mRecvCancelNew = false;
  bool mSendCancelNew = false;

  std::unique_ptr<SocketInterface> mHandle;

  boost::asio::strand<boost::asio::io_context::executor_type> mStrand;

  u64 mTotalSentData = 0;
  u64 mTotalRecvData = 0;

  void cancelRecvQueue(const error_code& ec);
  void cancelSendQueue(const error_code& ec);

  void close();
  void cancel(bool close);
  void asyncClose(std::function<void()> completionHandle);
  void asyncCancel(std::function<void()> completionHandle, bool close);

  IOService& getIOService() {
    return mIos;
  }

  bool stopped() {
    return mStatus == Channel_Status::Closed;
  }

  SpscQueue<SBO_ptr<SendOperation>> mSendQueue;
  SpscQueue<SBO_ptr<RecvOperation>> mRecvQueue;
  void recvEnque(SBO_ptr<RecvOperation>&& op);
  void sendEnque(SBO_ptr<SendOperation>&& op);

  void asyncPerformRecv();
  void asyncPerformSend();

  std::string commonName();

  std::array<boost::asio::mutable_buffer, 2> mSendBuffers;
  boost::asio::mutable_buffer mRecvBuffer;

  void printError(std::string s);

#ifdef ENABLE_NET_LOG
  u32 mRecvIdx = 0, mSendIdx = 0;
  Log mLog;
#endif
};



struct SessionBase;


    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::asyncSend(std::unique_ptr<Container> c)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(*c) - 1 < u32(-2));

        auto op = make_SBO_ptr<
            SendOperation,
            MoveSendBuff<
                std::unique_ptr<Container>
            >>(std::move(c));

        mBase->sendEnque(std::move(op));
    }

    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::asyncSend(std::shared_ptr<Container> c)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(*c) - 1 < u32(-2));


        auto op = make_SBO_ptr<
            SendOperation,
            MoveSendBuff<
                std::shared_ptr<Container>
            >>(std::move(c));

        mBase->sendEnque(std::move(op));
    }


    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::asyncSend(const Container& c)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(c) - 1 < u32(-2));

        auto* buff = (u8*)c.data();
        auto size = c.size() * sizeof(typename Container::value_type);

        auto op = make_SBO_ptr<
            SendOperation, 
            FixedSendBuff>(buff, size);

        mBase->sendEnque(std::move(op));
    }

    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::asyncSend(Container&& c)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(c) - 1 < u32(-2));

        auto op = make_SBO_ptr<
            SendOperation, 
            MoveSendBuff<Container>>
            (std::move(c));

        mBase->sendEnque(std::move(op));
    }

    template <class Container>
    typename std::enable_if<
        is_container<Container>::value &&
        !has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
        Channel::asyncRecv(Container & c)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(c) - 1 < u32(-2));

        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation, 
            RefRecvBuff<Container>>
            (c, future);

        mBase->recvEnque(std::move(op));

        return future;
    }

    template <class Container>
    typename std::enable_if<
        is_container<Container>::value&&
        has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
        Channel::asyncRecv(Container & c)
    {
        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation, 
            ResizableRefRecvBuff<Container>>
            (c, future);

        mBase->recvEnque(std::move(op));
        return future;
    }


    template <class Container>
    typename std::enable_if<
        is_container<Container>::value&&
        has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
        Channel::asyncRecv(Container & c, std::function<void()> fn)
    {
        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation,
            WithCallback<
                ResizableRefRecvBuff<Container>
            >>
            (std::move(fn), c, future);

        mBase->recvEnque(std::move(op));

        return future;
    }


    template <class Container>
    typename std::enable_if<
        is_container<Container>::value&&
        has_resize<Container, void(typename Container::size_type)>::value, std::future<void>>::type
        Channel::asyncRecv(Container & c, std::function<void(const error_code&)> fn)
    {
        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation,
            WithCallback<
                ResizableRefRecvBuff<Container>
            >>(std::move(fn), c, future);

        mBase->recvEnque(std::move(op));

        return future;
    }

    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::send(const Container& buf)
    {
        send(channelBuffData(buf), channelBuffSize(buf));
    }

    template<typename Container>
    typename std::enable_if<is_container<Container>::value, void>::type Channel::asyncSendCopy(const Container& buf)
    {
        asyncSend(std::move(Container(buf)));
    }


    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::send(const T* buffT, u64 sizeT)
    {
        asyncSendFuture(buffT, sizeT).get();
    }


    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
        Channel::asyncSendFuture(const T* buffT, u64 sizeT)
    {
        u8* buff = (u8*)buffT;
        auto size = sizeT * sizeof(T);

        // not zero and less that 32 bits
        Expects(size - 1 < u32(-2));

        std::future<void> future;
        auto op = make_SBO_ptr<
            SendOperation, 
            WithPromise<FixedSendBuff>>
            (future, buff, size);

        mBase->sendEnque(std::move(op));

        return future;
    }


    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::send(const T& buffT)
    {
        send(&buffT, 1);
    }

    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
        Channel::asyncRecv(T* buffT, u64 sizeT)
    {
        u8* buff = (u8*)buffT;
        auto size = sizeT * sizeof(T);

        // not zero and less that 32 bits
        Expects(size - 1 < u32(-2));

        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation, 
            FixedRecvBuff>
            (buff, size, future);

        mBase->recvEnque(std::move(op));
        return future;
    }

    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, std::future<void>>::type
        Channel::asyncRecv(T* buffT, u64 sizeT, std::function<void()> fn)
    {
        u8* buff = (u8*)buffT;
        auto size = sizeT * sizeof(T);

        // not zero and less that 32 bits
        Expects(size - 1 < u32(-2));

        std::future<void> future;
        auto op = make_SBO_ptr<
            RecvOperation, 
            WithCallback<FixedRecvBuff>>
            (std::move(fn), buff, size, future);

        mBase->recvEnque(std::move(op));

        return future;
    }

    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::asyncSend(const T* buffT, u64 sizeT)
    {
        u8* buff = (u8*)buffT;
        auto size = sizeT * sizeof(T);

        // not zero and less that 32 bits
        Expects(size - 1 < u32(-2));

        auto op = make_SBO_ptr<
            SendOperation, 
            FixedSendBuff>
            (buff, size);

        mBase->sendEnque(std::move(op));
    }



    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::asyncSend(const T& v)
    {
        asyncSend(&v, 1);
    }



    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type
        Channel::asyncSend(Container&& c, std::function<void()> callback)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(c) - 1 < u32(-2));

        auto op = make_SBO_ptr<
            SendOperation,
            WithCallback<
                MoveSendBuff<Container>
            >>(
                std::move(callback),
                std::forward<Container>(c));

        mBase->sendEnque(std::move(op));
    }

    template<class Container>
    typename std::enable_if<is_container<Container>::value, void>::type
        Channel::asyncSend(Container&& c, std::function<void(const error_code&)> callback)
    {
        // not zero and less that 32 bits
        Expects(channelBuffSize(c) - 1 < u32(-2));

        auto op = make_SBO_ptr<
            SendOperation,
            WithCallback<
                MoveSendBuff<Container>
            >>(
                std::move(callback),
                std::forward<Container>(c));

        mBase->sendEnque(std::move(op));
    }


    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::recv(T* buff, u64 size)
    {
        asyncRecv(buff, size).get();
    }

    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::asyncSendCopy(const T* bufferPtr, u64 length)
    {
        std::vector<u8> bs((u8*)bufferPtr, (u8*)bufferPtr + length * sizeof(T));
        asyncSend(std::move(bs));
    }


    template<typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type
        Channel::asyncSendCopy(const T& buf)
    {
        asyncSendCopy(&buf, 1);
    }

// FIXME （chenhongbo） CommPkg目前只能针对ABY3 的通信，需要定义基类让Protocol继承

// struct CommPkg {
//   Channel mPrev, mNext;
// };


}  // namespace primihub

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_CHANNEL_H_
