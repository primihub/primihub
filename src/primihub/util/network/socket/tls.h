// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_TLS_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_TLS_H_

#if defined(ENABLE_WOLFSSL) && defined(ENABLE_BOOST)

#include <string>
#include <memory>
#include <vector>
#include <utility>

#include <boost/system/error_code.hpp>
#include <boost/asio/strand.hpp>

#include "src/primihub/util/log.h"
#include "src/primihub/util/network/socket/socketadapter.h"
// #include "wolfssl/ssl.h"
// #include "src/primihub/util/network/socket/acceptor.h"

#ifndef WC_NO_HARDEN
#define WC_NO_HARDEN
#endif

#ifdef _MSC_VER
#define WOLFSSL_USER_SETTINGS
#define WOLFSSL_LIB
#endif

#undef ALIGN16

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#if defined(_MSC_VER) && !defined(KEEP_PEER_CERT)
#error "please compile wolfSSl with KEEP_PEER_CERT. add this to the user_setting.h file in wolfssl..."
#endif

#ifdef ENABLE_NET_LOG
#define WOLFSSL_LOGGING
#endif

namespace primihub {
using error_code = boost::system::error_code;
error_code readFile(const std::string& file,
  std::vector<u8>& buffer);

enum class WolfSSL_errc {
  Success = 0,
  Failure = 1
};

enum class TLS_errc {
  Success = 0,
  Failure,
  ContextNotInit,
  ContextAlreadyInit,
  ContextFailedToInit,
  OnlyValidForServerContext,
  SessionIDMismatch
};
}  // namespace primihub

namespace boost {
namespace system {
  template <>
  struct is_error_code_enum<primihub::WolfSSL_errc> : true_type {};
  template <>
  struct is_error_code_enum<primihub::TLS_errc> : true_type {};
}
}

namespace primihub {  // anonymous namespace

struct WolfSSLErrCategory : boost::system::error_category {
  const char* name() const noexcept override {
    return "osuCrypto_WolfSSL";
  }

  std::string message(int err) const override {
    std::array<char, WOLFSSL_MAX_ERROR_SZ> buffer;
    if (err == 0) return "Success";
    if (err == 1) return "Failure";
    return wolfSSL_ERR_error_string(err, buffer.data());
  }
};

const WolfSSLErrCategory WolfSSLCategory{};

struct TLSErrCategory : boost::system::error_category {
  const char* name() const noexcept override {
    return "osuCrypto_TLS";
  }

  std::string message(int err) const override {
    switch (static_cast<primihub::TLS_errc>(err)) {
      case primihub::TLS_errc::Success:
        return "Success";
      case primihub::TLS_errc::Failure:
        return "Generic Failure";
      case primihub::TLS_errc::ContextNotInit:
        return "TLS context not init";
      case primihub::TLS_errc::ContextAlreadyInit:
        return "TLS context is already init";
      case primihub::TLS_errc::ContextFailedToInit:
        return "TLS context failed to init";
      case primihub::TLS_errc::OnlyValidForServerContext:
        return "Operation is only valid for server initialized TLC context";
      case primihub::TLS_errc::SessionIDMismatch:
        return "Critical error on connect. Likely active attack by thirdparty";
      default:
        return "unknown error";
    }
  }
};

const TLSErrCategory TLSCategory{};

}  // anonymous namespace

namespace primihub {
inline error_code tls_make_error_code(WolfSSL_errc e) {
  auto ee = static_cast<int>(e);
  return { ee, WolfSSLCategory };
}

inline error_code tls_make_error_code(TLS_errc e) {
  auto ee = static_cast<int>(e);
  return { ee, TLSCategory };
}

inline error_code wolfssl_error_code(int ret) {
  switch (ret) {
    case WOLFSSL_SUCCESS:
      return tls_make_error_code(WolfSSL_errc::Success);
    case WOLFSSL_FAILURE:
      return tls_make_error_code(WolfSSL_errc::Failure);
    default:
      return tls_make_error_code(WolfSSL_errc(ret));
  }
}

struct WolfContext {
  enum class Mode {
    Client,
    Server,
    Both
  };

  struct Base {
    WOLFSSL_METHOD* mMethod = nullptr;
    WOLFSSL_CTX* mCtx = nullptr;
    Mode mMode = Mode::Client;

    Base(Mode mode);
    ~Base();
  };

  std::shared_ptr<Base> mBase;

  void init(Mode mode, error_code& ec);

  void loadCertFile(std::string path, error_code& ec);
  void loadCert(span<u8> data, error_code& ec);

  void loadKeyPairFile(std::string pkPath, std::string skPath, error_code& ec);
  void loadKeyPair(span<u8> pkData, span<u8> skData, error_code& ec);

  void requestClientCert(error_code& ec);

  bool isInit() const {
    return mBase != nullptr;
  }

  Mode mode() const {
    if (isInit())
      return mBase->mMode;
    else
      return Mode::Both;
  }

  operator bool() const {
    return isInit();
  }

  operator WOLFSSL_CTX* () const {
    return mBase ? mBase->mCtx : nullptr;
  }
};

using TLSContext = WolfContext;

struct WolfCertX509 {
  WOLFSSL_X509* mCert = nullptr;

  std::string commonName() {
    return wolfSSL_X509_get_subjectCN(mCert);
  }

  std::string notAfter() {
    WOLFSSL_ASN1_TIME* ptr = wolfSSL_X509_get_notAfter(mCert);
    std::array<char, 1024> buffer;
    wolfSSL_ASN1_TIME_to_string(ptr, buffer.data(), buffer.size());
    return buffer.data();
  }

  std::string notBefore() {
    WOLFSSL_ASN1_TIME* ptr = wolfSSL_X509_get_notBefore(mCert);
    std::array<char, 1024> buffer;
    wolfSSL_ASN1_TIME_to_string(ptr, buffer.data(), buffer.size());
    return buffer.data();
  }
};

struct WolfSocket : public SocketInterface, public LogAdapter {
  using buffer = boost::asio::mutable_buffer;

  boost::asio::ip::tcp::socket mSock;
  boost::asio::strand<boost::asio::io_context::executor_type> mStrand;
  boost::asio::io_context& mIos;
  WOLFSSL* mSSL = nullptr;
#ifdef WOLFSSL_LOGGING
  Log mLog_;
#endif
  std::vector<buffer> mSendBufs, mRecvBufs;

  u64 mSendBufIdx = 0, mRecvBufIdx = 0;
  u64 mSendBT = 0, mRecvBT = 0;

  error_code mSendEC, mRecvEC, mSetupEC;

  io_completion_handle mSendCB, mRecvCB;
  completion_handle mSetupCB, mShutdownCB;

  bool mCancelingPending = false;

  struct WolfState {
    enum class Phase { Uninit, Connect, Accept, Normal, Closed };
    Phase mPhase = Phase::Uninit;
    span<char> mPendingSendBuf;
    span<char> mPendingRecvBuf;
    bool hasPendingSend() { return mPendingSendBuf.size() > 0; }
    bool hasPendingRecv() { return mPendingRecvBuf.size() > 0; }
  };

  WolfState mState;

  WolfSocket(boost::asio::io_context& ios, WolfContext& ctx);
  WolfSocket(boost::asio::io_context& ios, boost::asio::ip::tcp::socket&& sock, WolfContext& ctx);

  WolfSocket(WolfSocket&&) = delete;
  WolfSocket(const WolfSocket&) = delete;

  ~WolfSocket() {
    close();
    if (mSSL)
      wolfSSL_free(mSSL);
  }

  void close() override;
  void cancel() override;

  void async_send(
    span<buffer> buffers,
    io_completion_handle&& fn) override;

  void async_recv(
    span<buffer> buffers,
    io_completion_handle&& fn) override;

  void setDHParamFile(std::string path, error_code& ec);
  void setDHParam(span<u8> paramData, error_code& ec);

  WolfCertX509 getCert();

  bool hasRecvBuffer() { return mRecvBufIdx < mRecvBufs.size(); }
  buffer& curRecvBuffer() { return mRecvBufs[mRecvBufIdx]; }

  bool hasSendBuffer() { return mSendBufIdx < mSendBufs.size(); }
  buffer& curSendBuffer() { return mSendBufs[mSendBufIdx]; }

  void send(
    span<buffer> buffers,
    error_code& ec,
    u64& bt);

  void sendNext();

  int sslRquestSendCB(char* buf, int size);

  void recv(
    span<buffer> buffers,
    error_code& ec,
    u64& bt);

  void recvNext();

  int sslRquestRecvCB(char* buf, int size);

  void connect(error_code& ec);
  void async_connect(completion_handle&& cb) override;
  void connectNext();

  void accept(error_code& ec);
  void async_accept(completion_handle&& cb) override;
  void acceptNext();

#ifdef WOLFSSL_LOGGING
  void LOG(std::string X);
#endif
  static int recvCallback(WOLFSSL* ssl, char* buf, int size,
    void* ctx) {
    WolfSocket& sock = *(WolfSocket*)ctx;
    assert(sock.mSSL == ssl);
    return sock.sslRquestRecvCB(buf, size);
  }

  static int sendCallback(WOLFSSL* ssl, char* buf, int size,
    void* ctx) {
    WolfSocket& sock = *(WolfSocket*)ctx;
    assert(sock.mSSL == ssl);
    return sock.sslRquestSendCB(buf, size);
  }
};

using TLSSocket = WolfSocket;

extern std::array<u8, 5010> sample_ca_cert_pem;
extern std::array<u8, 0x26ef> sample_server_cert_pem;
extern std::array<u8, 0x68f> sample_server_key_pem;
extern std::array<u8, 0x594> sample_dh2048_pem;

}  // namespace primihub

#else
namespace primihub {
struct TLSContext {
  operator bool() const {
    return false;
  }
};
}  // namespace primihub

#endif

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_TLS_H_
