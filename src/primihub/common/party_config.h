// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_COMMON_PARTY_CONFIG_H_
#define SRC_PRIMIHUB_COMMON_PARTY_CONFIG_H_
#include <string>
#include <map>
#include "src/primihub/protos/common.pb.h"
namespace primihub {
struct PartyConfig {
  PartyConfig() = default;
  PartyConfig(const std::string &node_id, const rpc::Task &task);
  void CopyFrom(const PartyConfig& config);
  const std::string party_name() const {return node_id;}
  const uint16_t party_id() const {return this->party_id_;}
  std::map<uint16_t, std::string>& PartyId2PartyNameMap() {return node_id_map;}
  std::map<std::string, rpc::Node>& PartyName2PartyInfoMap() {return node_map;}

 public:
  std::string node_id;
  uint16_t party_id_;
  std::string task_id;
  std::string job_id;
  std::string request_id;
  std::map<uint16_t, std::string> node_id_map;
  std::map<std::string, rpc::Node> node_map;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_COMMON_PARTY_CONFIG_H_
