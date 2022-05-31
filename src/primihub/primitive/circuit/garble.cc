
#include "src/primihub/primitive/circuit/garble.h"

namespace primihub {

#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS

  const std::array<block, 2> Garble::mPublicLabels{
    toBlock(0, 0), toBlock(~0, ~0)};

  bool Garble::isConstLabel(const block & b) {
    return eq(mPublicLabels[0], b) ||
      eq(mPublicLabels[1], b);
  }

  block Garble::garbleConstGate(bool constA, bool constB,
    const std::array<block, 2>& in, const GateType& gt,
    const block& xorOffset) {
    auto aa = PermuteBit(in[0]);
    auto bb = PermuteBit(in[1]);

    if (constA && constB) {
      return Garble::mPublicLabels[GateEval(gt, aa, bb)];
    } else {
      auto v = subGate(constB, aa, bb, gt);
      return Garble::mPublicLabels[v / 3] |
        ((in[constA] & zeroAndAllOne[v > 0])) ^
        (zeroAndAllOne[v == 1] & xorOffset);
    }
  }

  block Garble::evaluateConstGate(bool constA, bool constB,
    const std::array<block, 2>& in, const GateType& gt) {
    auto aa = PermuteBit(in[0]);
    auto bb = PermuteBit(in[1]);
    if (constA && constB) {
      return Garble::mPublicLabels[GateEval(gt, aa, bb)];
    } else {
      auto v = subGate(constB, aa, bb, gt);
      return Garble::mPublicLabels[v / 3] |
        ((in[constA] & zeroAndAllOne[v > 0]));
    }
  }

#endif

  u8 subGate(bool constB, bool aa, bool bb, GateType gt) {
    u8 g = static_cast<u8>(gt);
    auto g1 = (aa) * (((g & 2) >> 1) | ((g & 8) >> 2))
      + (1 ^ aa) * ((g & 1) | ((g & 4) >> 1));
    auto g2 = u8(gt) >> (2 * bb) & 3;
    auto ret = ((constB ^ 1) * g1 | constB * g2); {
      u8 subgate;

      if (constB) {
        subgate = u8(gt) >> (2 * bb) & 3;
      } else {
        u8 g = static_cast<u8>(gt);

        auto val = aa;
        subgate = val
          ? ((g & 2) >> 1) | ((g & 8) >> 2)
          : (g & 1) | ((g & 4) >> 1);
      }
      if (subgate != ret)
        throw std::runtime_error(LOCATION);
    }
    return ret;
  }

  void Garble::evaluate(
    const BetaCircuit& cir,
    span<block> wires,
    span<GarbledGate<2>> garbledGates,
    block& tweak,
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
    const std::function<bool()>& getAuxilaryBit,
#endif
    span<block> DEBUG_labels) {
      std::array<block, 2> tweaks{ tweak, tweak ^ CCBlock };
      u64 i = 0;
      auto garbledGateIter = garbledGates.begin();
      std::array<block, 2> in;

      block hashs[2], temp[2],
        zeroAndGarbledTable[2][2]
      { { ZeroBlock, ZeroBlock }, { ZeroBlock, ZeroBlock } };

    for (const auto& gate : cir.mGates) {
      auto& gt = gate.mType;

      if (GSL_LIKELY(gt != GateType::a)) {
        auto a = wires[gate.mInput[0]];
        auto b = wires[gate.mInput[1]];
        auto& c = wires[gate.mOutput];
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
        auto constA = isConstLabel(a);
        auto constB = isConstLabel(b);
        auto constAB = constA || constB;
#else
        static const bool constAB = 0;
#endif
        if (GSL_LIKELY(!constAB)) {
          if (GSL_LIKELY(gt == GateType::Xor ||
            gt == GateType::Nxor)) {
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
            if (GSL_LIKELY(neq(a, b))) {
              c = a ^ b;
            } else {
              c = mPublicLabels[getAuxilaryBit()];
            }
#else
            c = a ^ b;
#endif
#ifdef GARBLE_DEBUG
            if (DEBUG_labels.size()) DEBUG_labels[i++] = c;
#endif
          } else {
            // compute the hashs
            hashs[0] = (a << 1) ^ tweaks[0];
            hashs[1] = (b << 1) ^ tweaks[1];
            mAesFixedKey.ecbEncTwoBlocks(hashs, temp);
            hashs[0] = temp[0] ^ hashs[0];
            hashs[1] = temp[1] ^ hashs[1];

            // increment the tweaks
            tweaks[0] = tweaks[0] + OneBlock;
            tweaks[1] = tweaks[1] + OneBlock;

            auto& garbledTable = garbledGateIter++->mGarbledTable;
            zeroAndGarbledTable[1][0] = garbledTable[0];
            zeroAndGarbledTable[1][1] = garbledTable[1] ^ a;

            // compute the output wire label
            c = hashs[0] ^
              hashs[1] ^
              zeroAndGarbledTable[PermuteBit(a)][0] ^
              zeroAndGarbledTable[PermuteBit(b)][1];
#ifdef GARBLE_DEBUG
            if (DEBUG_labels.size())
              DEBUG_labels[i++] = c;
#endif
          }
        } else {
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
          in[0] = a;
          in[1] = b;
          c = evaluateConstGate(constA, constB, in, gt);
#ifdef GARBLE_DEBUG
          auto ab = constA ? b : a;
          if (isConstLabel(c) == false && neq(c, ab))
            throw std::runtime_error(LOCATION);
          if (DEBUG_labels.size()) DEBUG_labels[i++] = c;
#endif
#endif
        }
      } else {
        u64 src = gate.mInput[0];
        u64 len = gate.mInput[1];
        u64 dest = gate.mOutput;

      memcpy(&*(wires.begin() + dest),
        &*(wires.begin() + src), i32(len * sizeof(block)));
      }
    }
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
    for (u64 i = 0; i < cir.mOutputs.size(); ++i) {
      auto& out = cir.mOutputs[i].mWires;

      for (u64 j = 0; j < out.size(); ++j) {
        if (cir.mWireFlags[out[j]] ==
          BetaWireFlag::InvWire) {
          if (isConstLabel(wires[out[j]]))
            wires[out[j]] = wires[out[j]] ^ mPublicLabels[1];
        }
      }
    }
#endif

    tweak = tweaks[0];
  }

  void Garble::garble(
    const BetaCircuit& cir,
    span<block> wires,
    span<GarbledGate<2>>  gates,
    block& tweak,
    block& freeXorOffset,
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
    std::vector<u8>& auxilaryBits,
#endif
    span<block> DEBUG_labels) {
      std::array<block, 2> tweaks{ tweak, tweak ^ CCBlock };
      std::array<block, 2> mZeroAndGlobalOffset{ ZeroBlock, freeXorOffset};
      u64 i = 0;
      auto gateIter = gates.begin();
      std::array<block, 2> in;
      auto& mGlobalOffset = mZeroAndGlobalOffset[1];

      u8 aPermuteBit, bPermuteBit, bAlphaBPermute, cPermuteBit;
      block hash[4], temp[4];

      for (const auto& gate : cir.mGates) {
        auto& gt = gate.mType;
        if (GSL_LIKELY(gt != GateType::a)) {
          auto a = wires[gate.mInput[0]];
          auto b = wires[gate.mInput[1]];
          auto bNot = b ^ mGlobalOffset;

          auto& c = wires[gate.mOutput];
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
          auto constA = isConstLabel(a);
          auto constB = isConstLabel(b);
          auto constAB = constA || constB;
#else
          static const bool constAB = 0;
#endif
          if (GSL_LIKELY(!constAB)) {
            if (GSL_LIKELY(gt == GateType::Xor
              || gt == GateType::Nxor)) {
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
              // is a == b^1
              auto oneEq = eq(a, bNot);
              if (GSL_LIKELY(!(eq(a, b) || oneEq))) {
                c = a ^ b ^ mZeroAndGlobalOffset[(u8)gt & 1];
              } else {
                u8 bit = oneEq ^ ((u8)gt & 1);
                c = mPublicLabels[bit];

                // must tell the evaluator what the bit is.
                auxilaryBits.push_back(bit);
              }
#else
              c = a ^ b ^ mZeroAndGlobalOffset[(u8)gt & 1];
#endif

#ifdef GARBLE_DEBUG
              if (DEBUG_labels.size())
                DEBUG_labels[i++] = c;
#endif
            } else {
#ifdef GARBLE_DEBUG
              Expects(!(gt == GateType::a ||
                gt == GateType::b ||
                gt == GateType::na ||
                gt == GateType::nb ||
                gt == GateType::One ||
                gt == GateType::Zero));
#endif  // ! NDEBUG

              // compute the gate modifier variables
              auto& aAlpha = gate.mAAlpha;
              auto& bAlpha = gate.mBAlpha;
              auto& cAlpha = gate.mCAlpha;

              // signal bits of wire 0 of input0 and wire 0 of input1
              aPermuteBit = PermuteBit(a);
              bPermuteBit = PermuteBit(b);
              bAlphaBPermute = bAlpha ^ bPermuteBit;
              cPermuteBit = ((aPermuteBit ^ aAlpha)
                && (bAlphaBPermute)) ^ cAlpha;

              hash[0] = (a << 1) ^ tweaks[0];
              hash[1] = ((a ^ mGlobalOffset) << 1) ^ tweaks[0];
              hash[2] = (b << 1) ^ tweaks[1];
              hash[3] = ((bNot)<< 1) ^ tweaks[1];
              mAesFixedKey.ecbEncFourBlocks(hash, temp);
              hash[0] = hash[0] ^ temp[0];  // H( a0 )
              hash[1] = hash[1] ^ temp[1];  // H( a1 )
              hash[2] = hash[2] ^ temp[2];  // H( b0 )
              hash[3] = hash[3] ^ temp[3];  // H( b1 )

              // increment the tweaks
              tweaks[0] = tweaks[0] + OneBlock;
              tweaks[1] = tweaks[1] + OneBlock;

              // generate the garbled table
              auto& garbledTable = gateIter++->mGarbledTable;

              // compute the table entries
              garbledTable[0] = hash[0] ^ hash[1]
                ^ mZeroAndGlobalOffset[bAlphaBPermute];
              garbledTable[1] = hash[2] ^ hash[3] ^ a
                ^ mZeroAndGlobalOffset[aAlpha];

              // compute the out wire
              c = hash[aPermuteBit] ^
                hash[2 ^ bPermuteBit] ^
                mZeroAndGlobalOffset[cPermuteBit];

#ifdef GARBLE_DEBUG
              if (DEBUG_labels.size())
                DEBUG_labels[i++] = c;
#endif
          }
        } else {
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
          auto ab = constA ? b : a;
          in[0] = a;
          in[1] = b;
          c = garbleConstGate(constA, constB, in, gt,
            mGlobalOffset);
#ifdef GARBLE_DEBUG
          if (isConstLabel(c) == false &&
            neq(c, ab) &&
            neq(c, ab ^ mGlobalOffset))
            throw std::runtime_error(LOCATION);

          if (DEBUG_labels.size())
            DEBUG_labels[i++] = c;
#endif
#endif
       }

      } else {
        u64 src = gate.mInput[0];
        u64 len = gate.mInput[1];
        u64 dest = gate.mOutput;

        memcpy(&*(wires.begin() + dest),
          &*(wires.begin() + src), u32(len * sizeof(block)));
      }
    }

    for (u64 i = 0; i < cir.mOutputs.size(); ++i) {
      auto& out = cir.mOutputs[i].mWires;
      for (u64 j = 0; j < out.size(); ++j) {
        if (cir.mWireFlags[out[j]] == BetaWireFlag::InvWire) {
#ifdef OC_ENABLE_PUBLIC_WIRE_LABELS
          if (isConstLabel(wires[out[j]]))
            wires[out[j]] = wires[out[j]] ^ mPublicLabels[1];
          else
#endif
            wires[out[j]] = wires[out[j]] ^ mGlobalOffset;
        }
      }
    }
    tweak = tweaks[0];
  }

}  // namespace primihub
