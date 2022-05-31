// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_LOG_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_LOG_H_

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <array>

namespace primihub {
extern std::chrono::time_point<std::chrono::system_clock> gStart;
class Log {
 public:
  Log() = default;
  Log(const Log& c) {
    std::lock_guard<std::mutex>l(const_cast<std::mutex&>(c.mLock));
    mMessages = c.mMessages;
  }

  std::vector<std::pair<uint64_t, std::string>> mMessages;
  std::mutex mLock;

  void push(const std::string& msg) {
    std::lock_guard<std::mutex>l(mLock);
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(now - gStart).count();

    mMessages.emplace_back(ts, msg);
  }
};

inline std::ostream& operator<<(std::ostream& o, Log& log) {
  std::lock_guard<std::mutex>l(log.mLock);
  for (uint64_t i = 0; i < log.mMessages.size(); ++i) {
    o << "[" << i << ", " << log.mMessages[i].first / 1000.0
    << "ms ]  " << log.mMessages[i].second << std::endl;
  }

  return o;
}

class LogAdapter {
 public:
  Log* mLog = nullptr;

  LogAdapter() = default;
  LogAdapter(const LogAdapter&) = default;
  LogAdapter(Log& log) : mLog(&log) {}

  void push(const std::string& msg) {
    if (mLog)
      mLog->push(msg);
  }

  void setLog(Log& log) {
    mLog = &log;
  }
};

inline std::ostream& operator<<(std::ostream& o, LogAdapter& log) {
  if (log.mLog)
    o << *log.mLog;
  else
    o << "{null log}";
  return o;
}

enum class Color {
  LightGreen = 2,
  LightGrey = 3,
  LightRed = 4,
  OffWhite1 = 5,
  OffWhite2 = 6,
  Grey = 8,
  Green = 10,
  Blue = 11,
  Red = 12,
  Pink = 13,
  Yellow = 14,
  White = 15,
  Default
};

extern const Color ColorDefault;

std::ostream& operator<<(std::ostream& out, Color color);

enum class IoStream {
  lock,
  unlock
};

extern std::mutex gIoStreamMtx;

struct ostreamLock {
  std::ostream& out;
  std::unique_lock<std::mutex> mLock;

  ostreamLock(ostreamLock&&) = default;

  ostreamLock(std::ostream& o, std::mutex& lock = gIoStreamMtx) :
      out(o),
      mLock(lock)
  {}

  template<typename T>
  ostreamLock& operator<<(const T& v)
  {
      out << v;
      return *this;
  }

  template<typename T>
  ostreamLock& operator<<(T& v)
  {
      out << v;
      return *this;
  }
  ostreamLock& operator<< (std::ostream& (*v)(std::ostream&))
  {
      out << v;
      return *this;
  }
  ostreamLock& operator<< (std::ios& (*v)(std::ios&))
  {
      out << v;
      return *this;
  }
  ostreamLock& operator<< (std::ios_base& (*v)(std::ios_base&))
  {
      out << v;
      return *this;
  }
};


struct ostreamLocker
{
    std::ostream& out;

    ostreamLocker(std::ostream& o) :
        out(o)
    {}

    template<typename T>
    ostreamLock operator<<(const T& v)
    {
        ostreamLock r(out);
        r << v;

#ifndef NO_RETURN_ELISION
        return r;
#else
        return std::move(r);
#endif
    }

    template<typename T>
    ostreamLock operator<<(T& v)
    {
        ostreamLock r(out);
        r << v;
#ifndef NO_RETURN_ELISION
        return r;
#else
        return std::move(r);
#endif
    }
    ostreamLock operator<< (std::ostream& (*v)(std::ostream&))
    {
        ostreamLock r(out);
        r << v;
#ifndef NO_RETURN_ELISION
        return r;
#else
        return std::move(r);
#endif
    }
    ostreamLock operator<< (std::ios& (*v)(std::ios&))
    {
        ostreamLock r(out);
        r << v;
#ifndef NO_RETURN_ELISION
        return r;
#else
        return std::move(r);
#endif
    }
    ostreamLock operator<< (std::ios_base& (*v)(std::ios_base&))
    {
        ostreamLock r(out);
        r << v;
#ifndef NO_RETURN_ELISION
        return r;
#else
        return std::move(r);
#endif
    }
};
extern ostreamLocker lout;

std::ostream& operator<<(std::ostream& out, IoStream color);


void setThreadName(const std::string name);
void setThreadName(const char* name);

}

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_LOG_H_
