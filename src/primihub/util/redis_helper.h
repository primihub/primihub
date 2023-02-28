// Copyright [2022] <primihub>
#ifndef SRC_PRIMIHUB_UTIL_REDIS_HELPER_H_
#define SRC_PRIMIHUB_UTIL_REDIS_HELPER_H_

#include <string.h>
#include <string>
#include "hiredis/hiredis.h"

namespace primihub {
class RedisHelper {
 public:
  RedisHelper() = default;
  ~RedisHelper();
  int connect(std::string redis_addr, const std::string &redis_passwd);
  void disconnect(void);
  redisContext *getContext(void);

 private:
  redisContext *context_;
};

class RedisStringKVHelper : public RedisHelper {
 public:
  int setString(const std::string &key, const std::string &value);
  int getString(const std::string &key, std::string &value);
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_UTIL_REDIS_HELPER_H_
