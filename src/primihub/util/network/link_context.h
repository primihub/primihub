// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
#include "src/primihub/common/defines.h"
#include "src/primihub/protos/worker.pb.h"
#include <string_view>
#include <string>
#include <unordered_map>
#include <shared_mutex>
namespace primihub::network {
namespace rpc = primihub::rpc;
class IChannel {
 public:
  IChannel() = default;
  virtual ~IChannel() = default;
  // virtual retcode send(const std::string& data) = 0;
  // virtual retcode send(std::string_view sv_data) = 0;
  // virtual retcode send(const std::vector<std::string>& datas) = 0;
  // virtual retcode send(const std::vector<std::string_view> sv_datas) = 0;
  virtual retcode send(const rpc::TaskRequest& data) = 0;
  virtual retcode send(const std::vector<rpc::TaskRequest>& data) = 0;
  virtual retcode recv(const rpc::TaskRequest& request, std::vector<rpc::TaskResponse>* datas) = 0;
  virtual retcode sendRecv(const std::vector<rpc::TaskRequest>& senddata,
                           std::vector<rpc::TaskResponse>* recv_data) = 0;

};

/**
 * link connection manager
 * manage channel create by node info
*/
class LinkContext {
 public:
  LinkContext() = default;
  virtual ~LinkContext() = default;
  /**
   * return channel for specified node
   * if channel is not exist, create
  */
  virtual std::shared_ptr<IChannel> getChannel(const primihub::Node& node) = 0;
 protected:
  std::shared_mutex connection_mgr_mtx;
  std::unordered_map<std::string, std::shared_ptr<IChannel>> connection_mgr;
};

}  // namespace primihub::network

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_LINK_CONTEXT_H_
