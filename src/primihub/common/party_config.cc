#include "src/primihub/common/party_config.h"
#include "src/primihub/common/common.h"
namespace primihub {
PartyConfig::PartyConfig(const std::string& node_id_, const rpc::Task &task) {
  this->node_id = task.party_name();
  // Map in protocolbuf may give different sequence when iterate it, so copy
  // content to std::map.

  std::map<std::string, rpc::Node> node_map;
  {
    const auto& info = task.party_access_info();
    for (const auto& [party_name, node] : info) {
      int32_t tmp_party_id = node.party_id();
      node_id_map[tmp_party_id] = party_name;
      node_map[party_name] = node;
      if (party_name == this->node_id) {
        this->party_id_ = tmp_party_id;
      }
    }
  }

  this->task_id = task.task_info().task_id();
  this->job_id = task.task_info().job_id();
  this->request_id = task.task_info().request_id();
  this->node_map = std::move(node_map);
}

void PartyConfig::CopyFrom(const PartyConfig& config) {
  this->node_id = config.node_id;
  this->task_id = config.task_id;
  this->job_id = config.job_id;
  this->request_id = config.request_id;
  this->party_id_ = config.party_id_;
  this->node_id_map = config.node_id_map;
  this->node_map = config.node_map;
}

}
