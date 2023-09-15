// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_COMMON_H_
#define SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_COMMON_H_
namespace primihub::psi {

enum class PsiType {
  ECDH = 0,
  KKRT,
  TEE,
};

enum class PsiResultType {
  INTERSECTION = 0,
  DIFFERENCE = 1,
};
}  // namespace primihub::psi

#endif  // SRC_PRIMIHUB_KERNEL_PSI_OPERATOR_COMMON_H_
