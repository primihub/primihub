// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
#include <memory>
#include "src/primihub/kernel/psi/operator/common.h"
#include "src/primihub/kernel/psi/operator/base_psi.h"
#include "src/primihub/kernel/psi/operator/kkrt_psi.h"
#include "src/primihub/kernel/psi/operator/ecdh_psi.h"
#ifdef SGX
#include "src/primihub/kernel/psi/operator/tee_psi.h"
#endif  // SGX
#include "src/primihub/common/value_check_util.h"

namespace primihub::psi {
class Factory {
 public:
  static std::unique_ptr<BasePsiOperator> Create(PsiType psi_type,
      const Options& options, void* executor = nullptr) {
    std::unique_ptr<BasePsiOperator> operator_ptr{nullptr};
    switch (psi_type) {
    case PsiType::KKRT:
      operator_ptr = std::make_unique<KkrtPsiOperator>(options);
      break;
    case PsiType::ECDH:
      operator_ptr = std::make_unique<EcdhPsiOperator>(options);
      break;
    case PsiType::TEE:
      operator_ptr = CreateTeeOperator(options, executor);
      break;
    default:
      LOG(ERROR) << "unknown psi operator: " << static_cast<int>(psi_type);
      break;
    }
    return operator_ptr;
  }
  static std::unique_ptr<BasePsiOperator> CreateTeeOperator(
      const Options& options, void* executor) {
#ifdef SGX
    if (executor == nullptr) {
      LOG(ERROR) << "TeeEngine is invalid";
      return nullptr;
    }
    auto tee_engine = reinterpret_cast<sgx::TeeEngine*>(executor);
    return std::make_unique<TeePsiOperator>(options, tee_engine);
#else
    std::string err_msg{"sgx is not enabled"};
    LOG(ERROR) << err_msg;
    RaiseException(err_msg);
#endif
    }

};
}  // namespace primihub::psi
#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_FACTORY_H_
