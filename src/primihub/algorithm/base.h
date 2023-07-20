/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef SRC_PRIMIHUB_ALGORITHM_BASE_H_
#define SRC_PRIMIHUB_ALGORITHM_BASE_H_

#include <string>
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/util/network/link_context.h"
#include "cryptoTools/Network/Session.h"
#include "src/primihub/common/party_config.h"

using primihub::rpc::Task;
using primihub::service::DatasetService;

namespace primihub {
struct ABY3PartyConfig {
  ABY3PartyConfig() = default;
  ABY3PartyConfig(const PartyConfig& config) {
    party_config.CopyFrom(config);
  }

  retcode Init(const PartyConfig& config) {
    party_config.CopyFrom(config);
    return retcode::SUCCESS;
  }
  uint16_t NextPartyId() {
    return (SelfPartyId() + 1) % ABY3_TOTAL_PARTY_NUM;
  }

  uint16_t SelfPartyId() {
    return party_config.party_id();
  }

  uint16_t PrevPartyId() {
    return (SelfPartyId() + 2) % ABY3_TOTAL_PARTY_NUM;
  }

  std::string NextPartyName() {
    auto& party_id_map = party_config.PartyId2PartyNameMap();
    return party_id_map[NextPartyId()];
  }

  std::string PrevPartyName() {
    auto& party_id_map = party_config.PartyId2PartyNameMap();
    return party_id_map[PrevPartyId()];
  }

  std::string SelfPartyName() {
    return party_config.party_name();
  }

  Node NextPartyInfo() {
    return PartyName2PartyInfo(NextPartyName());
  }

  Node PrevPartyInfo() {
    return PartyName2PartyInfo(PrevPartyName());
  }

  Node SelfPartyInfo() {
    return PartyName2PartyInfo(SelfPartyName());
  }

  bool IsSelfParty(const uint16_t party_id) {
    return party_id == SelfPartyId();
  }

  bool IsPrevParty(const uint16_t party_id) {
    return party_id == PrevPartyId();
  }

  bool IsNextParty(const uint16_t party_id) {
    return party_id == NextPartyId();
  }

  retcode PartyName2PartyId(const std::string& party_name, uint16_t* party_id) {
    auto& party_id_map = party_config.PartyId2PartyNameMap();
    for (const auto& [id, name] : party_id_map) {
      if (party_name == name) {
        *party_id = id;
        return retcode::SUCCESS;
      }
    }
    return retcode::FAIL;
  }

 protected:
  Node PartyName2PartyInfo(const std::string& party_name) {
    Node party_node;
    auto& party_map = party_config.PartyName2PartyInfoMap();
    auto& pb_party_node = party_map[party_name];
    pbNode2Node(pb_party_node, &party_node);
    return party_node;
  }

 private:
  PartyConfig party_config;
};

class AlgorithmBase {
 public:
  explicit AlgorithmBase(std::shared_ptr<DatasetService> dataset_service)
      : dataset_service_(dataset_service) {};
  virtual ~AlgorithmBase(){};

  virtual int loadParams(primihub::rpc::Task &task) = 0;
  virtual int loadDataset() = 0;
  virtual int initPartyComm() = 0;
  virtual int execute() = 0;
  virtual int finishPartyComm() = 0;
  virtual int saveModel() = 0;

  std::shared_ptr<DatasetService>& datasetService() {
    return dataset_service_;
  }

  void InitLinkContext(network::LinkContext* link_ctx) {
    link_ctx_ref_ = link_ctx;
  }

  network::LinkContext* GetLinkContext() {
    return link_ctx_ref_;
  }

  uint16_t party_id() {return party_id_;}
  void set_party_id(uint16_t party_id) {party_id_ = party_id;}

  std::string party_name() {return party_name_;}
  void set_party_name(const std::string& party_name) {party_name_ = party_name;}

 protected:
  std::shared_ptr<DatasetService> dataset_service_;
  std::string algorithm_name_;
  network::LinkContext* link_ctx_ref_{nullptr};
  std::string party_name_;
  uint16_t party_id_;
};
} // namespace primihub

#endif // SRC_PRIMIHUB_ALGORITHM_BASE_H_
