// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
#include <map>
#include <vector>
#include <string>

#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/common.h"
#include "src/primihub/kernel/psi/operator/common.h"
namespace primihub::psi {
using LinkContext = network::LinkContext;
struct Options {
  LinkContext* link_ctx_ref;
  std::map<std::string, Node> party_info;
  std::string self_party;
  PsiResultType psi_result_type{PsiResultType::INTERSECTION};
  std::string code;
};

class BasePsiOperator {
 public:
  explicit BasePsiOperator(const Options& options) : options_(options) {}
  virtual ~BasePsiOperator() = default;
  /**
   * PSI protocol
  */
  retcode Execute(const std::vector<std::string>& input,
                  bool sync_result,
                  std::vector<std::string>* result);
  virtual retcode OnExecute(const std::vector<std::string>& input,
                            std::vector<std::string>* result) = 0;
  /**
   * broadcast from the party who get the result to the others who participate
   * in the protocol
   * result: input or output
   *  for party who finally get the result after protocol, reuslt is input
   *  for party who get result after broadcast step, result is output
  */
  virtual retcode BroadcastPsiResult(std::vector<std::string>* result);

 protected:
  /**
   * get partylist who need receive result from
  */
  retcode BroadcastPartyList(std::vector<Node>* party_list);
  /**
   * party who does not belong to broadcast party list,
   * such as the party tho has already get result after execute protocol,
   * in TEE mode, the executor which does not care about the result
  */
  bool IgnoreParty(const std::string& party_name);
  /**
   * party who doest not care about the result
  */
  bool IgnoreResult(const std::string& party_name);
  retcode BroadcastResult(const std::vector<std::string>& result);
  retcode ReceiveResult(std::vector<std::string>* result);
  retcode Send(const Node& peer_node, const std::string& send_buff);
  retcode Send(const std::string& key,
               const Node& peer_node,
               const std::string& send_buff);
  retcode Send(const std::string& key,
               const Node& peer_node,
               const std::string_view send_buff_sv);
  retcode Recv(std::string* recv_buff);
  retcode Recv(const std::string& key, std::string* recv_buff);

  void set_stop() {stop_.store(true);}

 protected:
  bool has_stopped() {
    return stop_.load(std::memory_order::memory_order_relaxed);
  }
  std::string PartyName() {return options_.self_party;}
  LinkContext* GetLinkContext() {return options_.link_ctx_ref;}
  PsiResultType GetPsiResultType() {return options_.psi_result_type;}
  Node PeerNode();
  retcode GetNodeByName(const std::string& party_name, Node* node_info);
 protected:
  std::atomic<bool> stop_{false};
  Options options_;
  std::string key_{"default"};
};
}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_BASE_PSI_H_
