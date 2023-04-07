
#ifndef SRC_primihub_PRIMITIVE_CIRCUIT_CIRCUIT_LIBRARY_H_
#define SRC_primihub_PRIMITIVE_CIRCUIT_CIRCUIT_LIBRARY_H_


#include <unordered_map>
#include <vector>
#include <utility>

#include "src/primihub/common/defines.h"
#include "src/primihub/primitive/circuit/beta_circuit.h"
#include "src/primihub/primitive/circuit/beta_library.h"

namespace primihub {

class CircuitLibrary : public BetaLibrary {
 public:
  BetaCircuit* int_Sh3Piecewise_helper(u64 aSize, u64 numThesholds);

  BetaCircuit* convert_arith_to_bin(u64 n, u64 bits);

  static void int_Sh3Piecewise_build_do(
    BetaCircuit& cd,
    span<BetaBundle> aa,
    const BetaBundle & b,
    span<BetaBundle> c);

  static void Preproc_build(BetaCircuit& cd, u64 dec);
  static void argMax_build(BetaCircuit& cd, u64 dec, u64 numArgs);

};

}  // namespace primihub

#endif  // SRC_primihub_PRIMITIVE_CIRCUIT_CIRCUIT_LIBRARY_H_
