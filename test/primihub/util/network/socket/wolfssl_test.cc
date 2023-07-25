// Copyright [2021] <primihub.com>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <array>
#include <stdio.h>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

#include "src/primihub/common/common.h"
#include "src/primihub/common/finally.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/timer.h"
#include "src/primihub/util/crypto/bit_vector.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session_base.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/channel_base.h"
#include "src/primihub/util/network/socket/channel.h"
#include "test/primihub/util/util_test.h"
#include "src/primihub/util/network/socket/tls.h"

namespace primihub {

#ifdef ENABLE_WOLFSSL

#define SVR_COMMAND_SIZE 256
#define throwEC(ec) {lout << "throwing " << ec.message() << " @ " <<LOCATION << std::endl; throw std::runtime_error(ec.message() + LOCATION);}

int client() {
  u64 trials = 10;
  WolfContext ctx;
  error_code ec;
  lout << "c 0" << std::endl;
  ctx.init(WolfContext::Mode::Client, ec);
  if (ec)
    throwEC(ec);
  lout << "c 1" << std::endl;
  // ctx.loadCert(sample_ca_cert_pem, ec);
  ctx.loadCertFile("data/certs/ca-cert.pem", ec);
  lout << "c 2" << std::endl;

  IOService ios;
  WolfSocket sock(ios.mIoService, ctx);
  lout << "c 3" << std::endl;

  if (ec) {
    lout << "failed to load cert, ec.message():" << ec.message() << std::endl;
    throw std::runtime_error(ec.message());
  }

  boost::asio::ip::tcp::resolver resolver(ios.mIoService);
  boost::asio::ip::tcp::resolver::query query("127.0.0.1", "1212");
  boost::asio::ip::tcp::endpoint addr = *resolver.resolve(query);

  u64 bt;
  sock.mSock.connect(addr, ec);
  if (!ec)
    sock.connect(ec);
  if (ec) throwEC(ec);

  std::array<char, SVR_COMMAND_SIZE> msg;
  std::array<char, SVR_COMMAND_SIZE> reply;

  for (u64 i =0; i < trials; ++i) {
    auto sendSz = 1 + (rand() % (msg.size()-1));
    for (u64 j = 0; j < sendSz; ++j)
      msg[j] = static_cast<char>(j);

    boost::asio::mutable_buffer bufs[2];
    bufs[0] = boost::asio::mutable_buffer((char*)&sendSz, sizeof(int));
    bufs[1] = boost::asio::mutable_buffer(msg.data(), sendSz);
    sock.send(bufs, ec, bt);

    if (ec)
      throwEC(ec);

    if (strncmp(msg.data(), "quit", 4) == 0) {
      fputs("sending server shutdown command: quit!\n", stdout);
      break;
    }

    if (strncmp(msg.data(), "break", 5) == 0) {
      fputs("sending server session close: break!\n", stdout);
      break;
    }

    bufs[1] = boost::asio::mutable_buffer(reply.data(), sendSz);
    sock.recv({ &bufs[1], 1 }, ec, bt);

    if (ec)
      throwEC(ec);
    if (memcmp(reply.data(), msg.data(), sendSz) != 0) {
      std::cout << "bad echo message " << std::endl;
      throw std::runtime_error(LOCATION);
    }
  }
  return 0;
}

int server() {
  try {
    int shutDown = 0;

    IOService ios;
    boost::asio::ip::tcp::resolver resolver(ios.mIoService);
    boost::asio::ip::tcp::resolver::query query("127.0.0.1", "1212");
    boost::asio::ip::tcp::endpoint addr = *resolver.resolve(query);
    boost::asio::ip::tcp::acceptor accpt(ios.mIoService);

    error_code ec;
    accpt.open(addr.protocol());
    accpt.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    accpt.bind(addr, ec);
    if (ec) {
      lout << "ec bind " << ec.message() << std::endl;
      throw RTE_LOC;
    }

    accpt.listen(boost::asio::socket_base::max_connections, ec);
    if (ec) {
      lout << "ec listen " << ec.message() << std::endl;
      throw RTE_LOC;
    }

    WolfContext ctx;
    ctx.init(WolfContext::Mode::Server, ec);
    if (ec) throwEC(ec);

    ctx.loadKeyPairFile("data/certs/server-cert.pem",
      "data/certs/server-key.pem", ec);

    if (ec) {
      lout << "failed to load server keys\n"
        << "ec = " << ec.message() << std::endl;
      throw RTE_LOC;
    }

    while (!shutDown) {
      std::array<char, SVR_COMMAND_SIZE> command;
      int echoSz = 0;

      WolfSocket sock(ios.mIoService, ctx);

      sock.setDHParam(sample_dh2048_pem, ec);
      if (ec) throwEC(ec);

      accpt.accept(sock.mSock, ec);

      if (!ec)
        sock.accept(ec);
      if (ec) {
        std::cout << "accept failed: " << ec.message() << std::endl;
        continue;
      }
      u64 bt;

      boost::asio::mutable_buffer bufs[1];

      while (true) {
        bufs[0] = boost::asio::mutable_buffer((char*)&echoSz,
          sizeof(int));
        sock.recv(bufs, ec, bt);

        if (ec) {
          shutDown = true;
          break;
        }

        if (echoSz > command.size()) {
          std::cout << Color::Green << "msg too large. "
            << std::endl << Color::Default;
          break;
        }

        bufs[0] = boost::asio::mutable_buffer(command.data(), echoSz);
        sock.recv(bufs, ec, bt);

        if (strncmp(command.data(), "quit", 4) == 0) {
          printf("client sent quit command: shutting down!\n");
          shutDown = 1;
          break;
        } else if (strncmp(command.data(), "break", 5) == 0) {
          printf("client sent break command: closing session!\n");
          break;
        } else {
          command[echoSz] = 0;
        }

        sock.send(bufs, ec, bt);
        if (ec) {
          std::cout << "failed to send. " << ec.message() << std::endl;
        }
      }
    }
  } catch (...) {
    lout << "server threw" << std::endl;
  }
  return 0;
}
#endif

TEST(wolfSSL_echoServer_test, echo_server) {
#ifdef ENABLE_WOLFSSL
  #ifdef __APPLE__
  throw UnitTestSkipped("known issue with raw ASIO.connect() for MAC");
  #endif

  auto thrd = std::thread([] { server(); });

  try {
    client();
  } catch (...) {
    thrd.join();
    throw;
  }

  thrd.join();
#else
  throw UnitTestSkipped("ENABLE_WOLFSSL not defined");
#endif
}

TEST(wolfSSL_mutualAuth_test, mutual_auth) {
#ifdef ENABLE_WOLFSSL
  error_code ec;
  IOService ios;
  u64 bt;

  boost::asio::ip::tcp::resolver resolver(ios.mIoService);
  boost::asio::ip::tcp::resolver::query query("127.0.0.1", "1212");
  boost::asio::ip::tcp::endpoint addr = *resolver.resolve(query);
  boost::asio::ip::tcp::acceptor accpt(ios.mIoService);
  accpt.open(addr.protocol());
  accpt.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  accpt.bind(addr, ec);
  if (ec) throwEC(ec);
  accpt.listen(boost::asio::socket_base::max_connections);

  WolfContext sctx;
  sctx.init(WolfContext::Mode::Server, ec);
  if (ec)
    throwEC(ec);
  sctx.requestClientCert(ec);
  if (ec)
    throwEC(ec);
  // sctx.loadCert(sample_ca_cert_pem, ec);
  sctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (ec)
    throwEC(ec);
  // sctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
  sctx.loadKeyPairFile("data/certs/server-cert.pem",
  "data/certs/server-key.pem", ec);
  if (ec)
    throwEC(ec);

  WolfContext cctx;
  cctx.init(WolfContext::Mode::Client, ec);
  if (ec)
    throwEC(ec);
  // cctx.loadCert(sample_ca_cert_pem, ec);
  cctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (ec)
    throwEC(ec);
  // cctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
  cctx.loadKeyPairFile("data/certs/server-cert.pem",
  "data/certs/server-key.pem", ec);
  if (ec)
    throwEC(ec);
  WolfSocket csock(ios.mIoService, cctx);
  csock.mSock.connect(addr, ec);
  if (ec)
    throwEC(ec);
  std::promise<error_code> prom;
  csock.async_connect([&](const error_code& ec) {
    prom.set_value(ec);
  });

  WolfSocket ssock(ios.mIoService, sctx);
  ssock.setDHParam(sample_dh2048_pem, ec);
  if (ec)
    throwEC(ec);

  accpt.accept(ssock.mSock, ec);
  if (ec)
    throwEC(ec);
  ssock.accept(ec);
  if (ec) {
    std::cout << "accept failed: " << ec.message() << std::endl;
    throwEC(ec);
  }

  ec = prom.get_future().get();
  if (ec) {
    std::cout << "connect failed: " << ec.message() << std::endl;
    throwEC(ec);
  }

  std::array<u8, 100> msg, resp;
  boost::asio::mutable_buffer bufs[1];
  bufs[0] = boost::asio::mutable_buffer(msg.data(), msg.size());
  ssock.send(bufs, ec, bt);
  if (ec)
    throwEC(ec);
  bufs[0] = boost::asio::mutable_buffer(resp.data(), resp.size());
  csock.recv(bufs, ec, bt);
  if (ec)
    throwEC(ec);
  bufs[0] = boost::asio::mutable_buffer(msg.data(), msg.size());
  csock.send(bufs, ec, bt);
  if (ec)
    throwEC(ec);
  bufs[0] = boost::asio::mutable_buffer(resp.data(), resp.size());
  ssock.recv(bufs, ec, bt);
  if (ec)
    throwEC(ec);

#else
  throw UnitTestSkipped("ENABLE_WOLFSSL not defined");
#endif
}

TEST(wolfSSL_channel_test, channel) {
#ifdef ENABLE_WOLFSSL
  IOService ios;
  error_code ec;
  WolfContext sctx, cctx;

  if (!ec) sctx.init(WolfContext::Mode::Server, ec);
  if (!ec) sctx.requestClientCert(ec);
  if (!ec)
    // sctx.loadCert(sample_ca_cert_pem, ec);
    sctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (!ec)
    // sctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
    sctx.loadKeyPairFile("data/certs/server-cert.pem",
      "data/certs/server-key.pem", ec);

  if (!ec)
    cctx.init(WolfContext::Mode::Client, ec);
  if (!ec)
    // cctx.loadCert(sample_ca_cert_pem, ec);
    cctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (!ec)
    // cctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
    cctx.loadKeyPairFile("data/certs/server-cert.pem",
      "data/certs/server-key.pem", ec);

  if (ec) throwEC(ec);

  Session sses, cses;

  sses.start(ios, "127.0.0.1", 1212, SessionMode::Server, sctx);
  cses.start(ios, "127.0.0.1", 1212, SessionMode::Client, cctx);

  auto schl = sses.addChannel();
  auto cchl = cses.addChannel();

  schl.waitForConnection();

  std::array<char, 10> data{32, 34, 3, 4, 5, 55, 3}, data2;
  schl.send(data);
  cchl.recv(data);
  cchl.send(data);
  schl.recv(data2);
  if (data != data2)
    throw UnitTestFail(LOCATION);

#else
  throw UnitTestSkipped("ENABLE_WOLFSSL not defined");
#endif
}

TEST(wolfSSL_CancelChannel_Test, cancel_channel) {
#ifdef ENABLE_WOLFSSL
  u64 trials = 3;

  error_code ec;
  WolfContext sctx, cctx;

  if (!ec) sctx.init(WolfContext::Mode::Server, ec);
  if (!ec) sctx.requestClientCert(ec);
  if (!ec)
    // sctx.loadCert(sample_ca_cert_pem, ec);
    sctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (!ec)
    // sctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
    sctx.loadKeyPairFile("data/certs/server-cert.pem",
      "data/certs/server-key.pem", ec);
  if (!ec)
    cctx.init(WolfContext::Mode::Client, ec);
  if (!ec)
    // cctx.loadCert(sample_ca_cert_pem, ec);
    cctx.loadCertFile("data/certs/ca-cert.pem", ec);
  if (!ec)
    // cctx.loadKeyPair(sample_server_cert_pem, sample_server_key_pem, ec);
    cctx.loadKeyPairFile("data/certs/server-cert.pem",
  "data/certs/server-key.pem", ec);
  if (ec)
    throwEC(ec);

  for (u64 i = 0; i < trials; ++i) {
    IOService ioService;
    ioService.showErrorMessages(false);
    {
      Session c1(ioService, "127.0.0.1", 1212, SessionMode::Server, sctx);
      Session s1(ioService, "127.0.0.1", 1212, SessionMode::Client, cctx);
      auto ch1 = c1.addChannel("t2");
      auto ch0 = s1.addChannel("t2");

      int i = 8;
      ch0.send(i);
      ch1.recv(i);

      bool throws = false;
      std::vector<u8> rr;
      auto f = ch1.asyncRecv(rr);
      std::this_thread::sleep_for(std::chrono::milliseconds(i));
      ch1.cancel();

      try { f.get(); }
      catch (...) { throws = true; }
      if (throws == false) {
#ifdef ENABLE_NET_LOG
        std::cout << ch1.mBase->mLog << std::endl;
#endif
        throw UnitTestFail(LOCATION);
      }
    }

    if (ioService.mAcceptors.size() != 1)
      throw UnitTestFail(LOCATION);
    if (ioService.mAcceptors.front().hasSubscriptions())
      throw UnitTestFail(LOCATION);
    if (ioService.mAcceptors.front().isListening())
        throw UnitTestFail(LOCATION);

    {
      Session c1(ioService, "127.0.0.1", 1212, SessionMode::Server, sctx);
      Session s1(ioService, "127.0.0.1", 1212, SessionMode::Client, cctx);
      auto ch1 = c1.addChannel("t2");
      auto ch0 = s1.addChannel("t2");

      bool throws = false;
      std::vector<u8> rr;
      auto f = ch1.asyncRecv(rr);
      std::this_thread::sleep_for(std::chrono::milliseconds(i));
      ch1.cancel();
      try { f.get(); }
      catch (...) { throws = true; }

      if (throws == false) {
#ifdef ENABLE_NET_LOG
        std::cout << ch1.mBase->mLog << std::endl;
#endif
        throw UnitTestFail(LOCATION);
      }
      ch0.cancel();
    }

    if (ioService.mAcceptors.front().hasSubscriptions())
      throw UnitTestFail(LOCATION);
    if (ioService.mAcceptors.front().isListening())
      throw UnitTestFail(LOCATION);
  }

#else
  throw UnitTestSkipped("ENABLE_WOLFSSL not defined");
#endif
}

}  // namespace primihub
