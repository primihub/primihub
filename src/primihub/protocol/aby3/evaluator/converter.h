
#ifndef SRC_primihub_PROTOCOL_ABY3_EVALUATOR_CONVERTER_H_
#define SRC_primihub_PROTOCOL_ABY3_EVALUATOR_CONVERTER_H_

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/primitive/circuit/circuit_library.h"

namespace primihub {

class Sh3Converter {
 public:
  CircuitLibrary mLib;

  void toPackedBin(const sbMatrix& in, sPackedBin& dest);

  void toBinaryMatrix(const sPackedBin& in, sbMatrix& dest);

  Sh3Task toPackedBin(Sh3Task dep, Sh3ShareGen& gen,
    const si64Matrix& in, sPackedBin& dest);
  Sh3Task toBinaryMatrix(Sh3Task dep, const si64Matrix& in,
    sbMatrix& dest);

  Sh3Task toSi64Matrix(Sh3Task dep, const sbMatrix& in,
    si64Matrix& dest);
  Sh3Task toSi64Matrix(Sh3Task dep, const sPackedBin& in,
    si64Matrix& dest);
};

}  // namespace primihub

#endif  // SRC_primihub_PROTOCOL_ABY3_EVALUATOR_CONVERTER_H_
