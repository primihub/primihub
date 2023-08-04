// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
#include <memory>
#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "src/primihub/kernel/psi/operator/kkrt_psi.h"
#include "src/primihub/kernel/psi/operator/ecdh_psi.h"
#include "src/primihub/kernel/psi/operator/common.h"
namespace primihub::psi {
class Factory {
 public:
  static std::unique_ptr<BasePsiOperator> Create(PsiType psi_type,
                                                 const Options& options) {
    std::unique_ptr<BasePsiOperator> operator_ptr{nullptr};
    switch (psi_type) {
    case PsiType::KKRT:
      operator_ptr = std::make_unique<KkrtPsiOperator>(options);
      break;
    case PsiType::ECDH:
      operator_ptr = std::make_unique<EcdhPsiOperator>(options);
      break;
    default:
      LOG(ERROR) << "unknown psi operator: " << static_cast<int>(psi_type);
      break;
    }
    return operator_ptr;
  }
};
}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
