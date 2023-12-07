// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_KKRT_PSI_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_KKRT_PSI_H_
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>

#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/Defines.h"
#include "libPSI/PSI/Kkrt/KkrtPsiReceiver.h"
#include "src/primihub/util/network/message_interface.h"

namespace primihub::psi {
using TaskMessagePassInterface = primihub::network::TaskMessagePassInterface;
class KkrtPsiOperator : public BasePsiOperator {
 public:
  explicit KkrtPsiOperator(const Options& options) : BasePsiOperator(options) {}
  retcode OnExecute(const std::vector<std::string>& input,
                    std::vector<std::string>* result) override;

 protected:
  auto BuildChannelInterface() -> std::unique_ptr<TaskMessagePassInterface>;
  retcode KkrtRecv(oc::Channel& chl,
                   const std::vector<std::string>& input,
                   std::vector<uint64_t>* result_index);
  retcode KkrtSend(oc::Channel& chl, const std::vector<std::string>& input);
  retcode HashDataParallel(const std::vector<std::string>& input,
                           std::vector<oc::block>* result);
};
}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_KKRT_PSI_H_
