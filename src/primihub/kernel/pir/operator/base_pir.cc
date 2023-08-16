// "Copyright [2023] <PrimiHub>"
#include "src/primihub/kernel/pir/operator/base_pir.h"
namespace primihub::pir {
retcode BasePirOperator::Execute(const PirDataType& input,
                                 PirDataType* result) {
  return OnExecute(input, result);
}
}  // namespace primihub::pir
