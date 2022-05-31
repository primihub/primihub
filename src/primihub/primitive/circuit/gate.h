
#ifndef SRC_primihub_PRIMITIVE_CIRCUIT_GATE_H_
#define SRC_primihub_PRIMITIVE_CIRCUIT_GATE_H_

#include <string>

#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/block.h"
#ifdef ENABLE_CIRCUITS
#include <vector>
#include <array>

namespace primihub {

typedef u64 Wire;

inline u8 PermuteBit(const block& b) {
  return *(u8*)&(b)& 1;
}

enum class GateType : u8 {
  Zero = 0,    // 0000,
  Nor = 1,     // 0001
  nb_And = 2,  // 0010
  nb = 3,      // 0011
  na_And = 4,  // 0100
  na = 5,      // 0101
  Xor = 6,     // 0110
  Nand = 7,    // 0111
  And = 8,     // 1000
  Nxor = 9,    // 1001
  a = 10,      // 1010
  nb_Or = 11,  // 1011
  b = 12,      // 1100
  na_Or = 13,  // 1101
  Or = 14,     // 1110
  One = 15     // 1111
};

inline std::string gateToString(GateType type) {
  if (type == GateType::Zero)
    return "Zero  ";
  if(type == 	   GateType::Nor   )return "Nor	  ";
  if(type == 	   GateType::nb_And)return "nb_And";
  if(type == 	   GateType::nb    )return "nb 	  ";
  if(type == 	   GateType::na_And)return "na_And";
  if(type == 	   GateType::na    )return "na 	  ";
  if(type == 	   GateType::Xor   )return "Xor   ";
  if(type == 	   GateType::Nand  )return "Nand  ";
  if(type == 	   GateType::And   )return "And   ";
  if(type == 	   GateType::Nxor  )return "Nxor  ";
  if(type == 	   GateType::a 	   )return "a 	  ";
  if(type == 	   GateType::nb_Or )return "nb_Or ";
  if(type == 	   GateType::b 	   )return "b 	  ";
  if(type == 	   GateType::na_Or )return "na_Or ";
  if(type == 	   GateType::Or    )return "Or 	  ";
  if(type == 	   GateType::One   )return "One	  ";
  return "";
}

inline bool isLinear(GateType type) {
  return type == GateType::Xor  ||
    type == GateType::Nxor ||
    type == GateType::a    ||
    type == GateType::Zero ||
    type == GateType::nb   ||
    type == GateType::na   ||
    type == GateType::b    ||
    type == GateType::One;
}

inline u8 GateEval(GateType type, bool a, bool b) {
  u8 v = ((u8(a) & 1) | (u8(b) << 1));
  return ((u8)type & (1 << v)) ? 1 : 0;
}

struct Gate {
  u8 eval(u64 i) const {
    return ((u8)mType & (1 << i))? 1 : 0;
  }

  Gate(u64 input0, u64 input1, u64 output, GateType gt) {
    mInput = { { input0, input1 } };
    mType = gt;
    mWireIdx = output;

    // compute the gate modifier variables
    mAAlpha = (gt == GateType::Nor || gt == GateType::na_And
      || gt == GateType::nb_Or || gt == GateType::Or);
    mBAlpha = (gt == GateType::Nor || gt == GateType::nb_And
      || gt == GateType::na_Or || gt == GateType::Or);
    mCAlpha = (gt == GateType::Nand || gt == GateType::nb_Or
      || gt == GateType::na_Or || gt == GateType::Or);
  }

  // truth table padded to be 64 bits
  // std::array<u8, 4> mLgicTable;
  std::array<u64, 2> mInput;
  u64 mWireIdx;
  inline const GateType& Type() const { return mType; }
  inline const u8& AAlpha() const { return mAAlpha; }
  inline const u8& BAlpha() const { return mBAlpha; }
  inline const u8& CAlpha() const { return mCAlpha; }

 private:
  GateType mType;
  u8 mAAlpha, mBAlpha, mCAlpha;
};

template<u32 tableSize>
struct GarbledGate{
 public:
  std::array<block, tableSize> mGarbledTable;
};

}

#endif

#endif  // SRC_primihub_PRIMITIVE_CIRCUIT_GATE_H_
