// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_MPC_COMMPKG_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_MPC_COMMPKG_H_

#include <string>
#include <unordered_map>

#include "src/primihub/util/network/mpc_channel.h"
namespace primihub {
struct CommPkgBase {
  CommPkgBase();
  virtual ~CommPkgBase() = default;
  std::unordered_map<std::string, MpcChannel*> channels;
};

class ABY3CommPkg : public CommPkgBase {
 public:
  ABY3CommPkg() {}

  ABY3CommPkg(MpcChannel& prev, MpcChannel& next) {
    channels.emplace("prev", &prev);
    channels.emplace("next", &next);
  }

  ABY3CommPkg(const ABY3CommPkg& other) {
    channels.emplace("prev", other.channels.at("prev"));
    channels.emplace("next", other.channels.at("next"));
  }

  ABY3CommPkg& operator=(const ABY3CommPkg& other) {
    channels.emplace("prev", other.channels.at("prev"));
    channels.emplace("next", other.channels.at("next"));
    return *this;
  }

  MpcChannel& mNext() { return *(channels.at("next")); }

  MpcChannel& mPrev() { return *(channels.at("prev")); }

  void setPrev(MpcChannel& channel) { channels["prev"] = &channel; }

  void setNext(MpcChannel& channel) { channels["next"] = &channel; }
};

using CommPkg = ABY3CommPkg;
}  // namespace primihub
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_MPC_COMMPKG_H_
