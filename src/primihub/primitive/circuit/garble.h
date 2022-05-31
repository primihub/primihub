
#ifndef SRC_primihub_PRIMITIVE_CIRCUIT_GARBLE_H_
#define SRC_primihub_PRIMITIVE_CIRCUIT_GARBLE_H_

#include <vector>
#include <array>
#include <functional>

#include "src/primihub/primitive/circuit/beta_circuit.h"
#include "src/primihub/util/crypto/aes/aes.h"

namespace primihub {

class Garble {
 public:
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
  static const std::array<block, 2> mPublicLabels;
  static bool isConstLabel(const block& b);
  static block evaluateConstGate(bool constA, bool constB,
    const std::array<block, 2>& in, const GateType& gt);
  static block garbleConstGate(bool constA, bool constB,
    const std::array<block, 2>& in, const GateType& gt, const block& xorOffset);
#endif  // OC_ENABLE_PUBLIC_WIRE_LABELS
  static void evaluate(
    const BetaCircuit& cir,
    span<block> memory,
    span<GarbledGate<2>> garbledGates,
    block& tweaks,
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
    const std::function<bool()>& getAuxilaryBit,
#endif
    span<block> DEBUG_labels = {});

  static void garble(
    const BetaCircuit& cir,
    span<block> memory,
    span<GarbledGate<2>> garbledGates,
    block& tweak,
    block& freeXorOffset,
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
    std::vector<u8>& auxilaryBits,
#endif
    span<block> DEBUG_labels = {});
};

}  // namespace primihub

#endif  // SRC_primihub_PRIMITIVE_CIRCUIT_GARBLE_H_
