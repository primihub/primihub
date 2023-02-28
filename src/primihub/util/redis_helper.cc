// Copyright [2022] <primihub>
#include <glog/logging.h>
#include <string>
#include "hiredis/hiredis.h"
#include "src/primihub/util/redis_helper.h"

namespace primihub {
int RedisHelper::connect(std::string redis_addr, const std::string &redis_passwd) {
  auto pos = redis_addr.find(":");
  if (pos == std::string::npos) {
    LOG(ERROR)
        << "The address of redis should be HOST:PORT format, but not found in "
        << redis_addr << ".";
    return -1;
  }

  std::string host = redis_addr.substr(0, pos);
  std::string port_str = redis_addr.substr(pos + 1, redis_addr.size());

  LOG(INFO) << "Redis server: host " << host << ", port " << port_str << ".";

  int port = std::stoi(port_str);

  context_ = redisConnect(host.c_str(), port);
  if (context_ == nullptr || context_->err == 1) {
    LOG(ERROR) << "Connect to redis server failed, " << context_->errstr << ".";
    return -2;
  }

  auto reply = reinterpret_cast<redisReply*>(
          redisCommand(context_, "AUTH %s", redis_passwd.c_str()));
  if (reply->type == REDIS_REPLY_ERROR) {
    LOG(ERROR) << "Authorization failed: " << reply->str << ".";
    freeReplyObject(reply);
    return -3;
  }

  LOG(INFO) << "Connect to redis server " << host << ":" << port << " finish.";
  return 0;
}

void RedisHelper::disconnect(void) {
  if (context_) {
    redisFree(context_);
    context_ = nullptr;
  }
}

RedisHelper::~RedisHelper() { disconnect(); }

redisContext *RedisHelper::getContext(void) { return context_; }

int RedisStringKVHelper::setString(const std::string &key,
                                   const std::string &value) {
  redisContext *context = getContext();
  auto reply = reinterpret_cast<redisReply*>(
          redisCommand(context, "SET %s %s", key.c_str(), value.c_str()));
  if (reply == nullptr) {
    LOG(ERROR) << "Get null redis reply.";
    return -1;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    LOG(ERROR) << "Redis server returns an error: " << reply->str << ".";
    return -2;
  }

  freeReplyObject(reply);

  return 0;
}

int RedisStringKVHelper::getString(const std::string &key, std::string &value) {
  redisContext *context = getContext();
  auto reply = reinterpret_cast<redisReply*>(
          redisCommand(context, "GET %s", key.c_str()));

  if (reply->type == REDIS_REPLY_ERROR) {
    LOG(ERROR) << "Redis server returns an error: " << reply->str << ".";
    return -1;
  }

  if (reply->type != REDIS_REPLY_STRING) {
    LOG(ERROR) << "Redis server don't return a string, return type "
               << reply->type << ".";
    return -1;
  }

  if (reply->len == 0) {
    LOG(ERROR) << "Get value for key " << key << " from redis failed.";
    return -1;
  }

  value = std::string(reply->str);
  freeReplyObject(reply);

  return 0;
}
}  // namespace primihub
