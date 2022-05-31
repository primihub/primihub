
#include "src/primihub/primitive/circuit/beta_circuit.h"
#ifdef ENABLE_CIRCUITS

#ifdef USE_JSON
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#endif

namespace primihub {

BetaCircuit::BetaCircuit()
    :mNonlinearGateCount(0),
    mWireCount(0) {}

BetaCircuit::~BetaCircuit() {}

void BetaCircuit::addTempWire(BetaWire& in) {
  in = mWireCount++;
  mWireFlags.resize(mWireCount, BetaWireFlag::Uninitialized);
}

void BetaCircuit::addTempWireBundle(BetaBundle & in) {
  for (u64 i = 0; i < in.mWires.size(); ++i) {
    in.mWires[i] = mWireCount++;
  }

  mWireFlags.resize(mWireCount, BetaWireFlag::Uninitialized);
}

void BetaCircuit::addInputBundle(BetaBundle & in) {
  for (u64 i = 0; i < in.mWires.size(); ++i) {
    in.mWires[i] = mWireCount++;
  }

  mWireFlags.resize(mWireCount, BetaWireFlag::Wire);

  mInputs.push_back(in);

  if (mGates.size())
    throw std::runtime_error("inputs must be defined before gates are added. " LOCATION);

  if (mOutputs.size())
    throw std::runtime_error("inputs must be defined before outputs are added. " LOCATION);
}

void BetaCircuit::addOutputBundle(BetaBundle& out) {
  for (u64 i = 0; i < out.mWires.size(); ++i) {
    out.mWires[i] = mWireCount++;
  }
  mWireFlags.resize(mWireCount, BetaWireFlag::Uninitialized);

  mOutputs.push_back(out);

  if (mGates.size())
    throw std::runtime_error("outputs must be defined before gates are added. " LOCATION);
}

void BetaCircuit::addConstBundle(BetaBundle & in, const BitVector& val) {
  mWireFlags.resize(mWireCount + in.mWires.size(), BetaWireFlag::Wire);

  for (u64 i = 0; i < in.mWires.size(); ++i) {
    in.mWires[i] = mWireCount++;
    mWireFlags[in.mWires[i]] = val[i] ? 
      BetaWireFlag::One : BetaWireFlag::Zero;
  }
}

GateType invertInputWire(u64 wirePosition, const GateType& oldGateType) {
  if (wirePosition == 0) {
    // swap bit 0/1 and 2/3
    auto s = u8(oldGateType);

    return GateType(
      (s & 1) << 1 |  // bit 0 -> bit 1
      (s & 2) >> 1 |  // bit 1 -> bit 0
      (s & 4) << 1 |  // bit 3 -> bit 4
      (s & 8) >> 1);  // bit 4 -> bit 3
  } else if (wirePosition == 1) {
    // swap bit (0,1)/(2,3)
    auto s = u8(oldGateType);

    return GateType(
      (s & 3) << 2 |  // bits (0,1) -> bits (2,3)
      (s & 12) >> 2);  // bits (2,3) -> bits (0,1)
  } else {
    throw std::runtime_error("");
  }
}

void BetaCircuit::addGate(
  BetaWire aIdx,
  BetaWire bIdx,
  GateType gt,
  BetaWire out) {
  if (gt == GateType::a ||
    gt == GateType::b ||
    gt == GateType::na ||
    gt == GateType::nb ||
    gt == GateType::One ||
    gt == GateType::Zero)
    throw std::runtime_error("");

  if (aIdx >= mWireCount ||
    mWireFlags[aIdx] == BetaWireFlag::Uninitialized)
    throw std::runtime_error(LOCATION);

  if (bIdx >= mWireCount ||
    mWireFlags[bIdx] == BetaWireFlag::Uninitialized)
    throw std::runtime_error(LOCATION);

  if (out >= mWireCount)
    throw std::runtime_error(LOCATION);

  if (aIdx == bIdx)
    throw std::runtime_error(LOCATION);

  auto constA = isConst(aIdx), constB = isConst(bIdx);

  if (constA || constB) {
    if (constA && constB) {
      u8 val = GateEval(gt, constVal(aIdx) != 0, constVal(bIdx) != 0);
      addConst(out, val);
    } else {
      u8 subgate;
      const BetaWire* wireIdx;

      if (constB) {
        wireIdx = &aIdx;
        subgate = u8(gt) >> (2 * constVal(bIdx)) & 3;
      } else {
        wireIdx = &bIdx;
        u8 g = static_cast<u8>(gt);

        auto val = constVal(aIdx);
        subgate = val ? ((g & 2) >> 1) | ((g & 8) >> 2)
          : (g & 1) | ((g & 4) >> 1);
      }

      switch (subgate) {
        case 0:
          addConst(out, 0);
          break;
        case 1:
          addCopy(*wireIdx, out);
          addInvert(out);
          break;
        case 2:
          addCopy(*wireIdx, out);
          break;
        case 3:
          addConst(out, 1);
          break;
        default:
          throw std::runtime_error(LOCATION);
          break;
      }
    }
  } else {
    if (isInvert(aIdx))
      gt = invertInputWire(0, gt);
    if (isInvert(bIdx))
      gt = invertInputWire(1, gt);

    if (gt != GateType::Xor && gt != GateType::Nxor)
      ++mNonlinearGateCount;
    mGates.emplace_back(aIdx, bIdx, gt, out);

    mWireFlags[out] = BetaWireFlag::Wire;
  }
}

void BetaCircuit::addConst(BetaWire wire, u8 val) {
  mWireFlags[wire] = val ? BetaWireFlag::One : BetaWireFlag::Zero;
}

void BetaCircuit::addInvert(BetaWire wire) {
  switch (mWireFlags[wire]) {
    case BetaWireFlag::Zero:
      mWireFlags[wire] = BetaWireFlag::One;
      break;
    case BetaWireFlag::One:
      mWireFlags[wire] = BetaWireFlag::Zero;
      break;
    case BetaWireFlag::Wire:
      mWireFlags[wire] = BetaWireFlag::InvWire;
      break;
    case BetaWireFlag::InvWire:
      mWireFlags[wire] = BetaWireFlag::Wire;
      break;
    default:
      throw std::runtime_error(LOCATION);
      break;
  }
}

void BetaCircuit::addInvert(BetaWire src, BetaWire dest) {
  addCopy(src, dest);
  addInvert(dest);
}

void BetaCircuit::addCopy(BetaWire src, BetaWire dest) {
  // copy 1 wire label starting at src to dest
  // memcpy(dest, src, sizeof(block));
  mGates.emplace_back(src, 1, GateType::a, dest);
  mWireFlags[dest] = mWireFlags[src];
}

void BetaCircuit::addCopy(const BetaBundle & src, const BetaBundle & dest) {
  auto d = dest.mWires.begin();
  auto dd = dest.mWires.end();
  auto s = src.mWires.begin();
  auto i = src.mWires.begin();

  while (d != dest.mWires.end()) {
    ++i;
    mWireFlags[*d] = mWireFlags[*s];

    u64 rem = (dd - d);
    u64 len = 1;
    while (len < rem && *i == *(i - 1) + 1) {
      ++i;
      mWireFlags[*(d + len)] = mWireFlags[*(s + len)];
      ++len;
    }

    mGates.emplace_back(*s, u32(len), GateType::a, *d);
    d += len;
    s += len;
  }
}

bool BetaCircuit::isConst(BetaWire wire) const {
  return mWireFlags[wire] == BetaWireFlag::One
    || mWireFlags[wire] == BetaWireFlag::Zero;
}

bool BetaCircuit::isInvert(BetaWire wire)const {
  return mWireFlags[wire] == BetaWireFlag::InvWire;
}

u8 BetaCircuit::constVal(BetaWire wire) {
  if (mWireFlags[wire] == BetaWireFlag::Wire)
    throw std::runtime_error(LOCATION);

  return mWireFlags[wire] == BetaWireFlag::One ? 1 : 0;
}

void BetaCircuit::addPrint(BetaBundle in) {
  for (u64 j = in.mWires.size() - 1; j < in.mWires.size(); --j) {
    addPrint(in[j]);
  }
}

void BetaCircuit::addPrint(BetaWire wire) {
  if (isConst(wire)) {
    addPrint(std::to_string(u64(constVal(wire))));
  } else {
    mPrints.emplace_back(mGates.size(), wire, "", isInvert(wire));
  }
}

void BetaCircuit::addPrint(std::string str) {
  mPrints.emplace_back(mGates.size(), -1, str, false);
}

void BetaCircuit::evaluate(
  span<BitVector> input,
  span<BitVector> output,
  bool print) {
  std::vector<u8> mem(mWireCount), outOfDate(mWireCount, 0);

  if (static_cast<u64>(input.size()) != mInputs.size()) {
    throw std::runtime_error(LOCATION);
  }

  for (u64 i = 0; i < static_cast<u64>(input.size()); ++i) {
    if (input[i].size() != mInputs[i].mWires.size())
      throw std::runtime_error(LOCATION);

    for (u64 j = 0; j < input[i].size(); ++j) {
      mem[mInputs[i].mWires[j]] = input[i][j];
    }
  }
  auto iter = mPrints.begin();

  auto levelRemIter = mLevelCounts.begin();
  auto curLevelRem = mLevelCounts.size() ? *levelRemIter++ : u64(-1);
  u64 level = 0;

  for (u64 i = 0; i < mGates.size(); ++i) {
    auto& gate = mGates[i];
    while (print && iter != mPrints.end() && (*iter).mGateIdx == i) {
      auto wireIdx = (*iter).mWire;
      auto str = (*iter).mMsg;
      auto invert = (*iter).mInvert;

      if (wireIdx != static_cast<u32>(~0))
          std::cout << (u64)(mem[wireIdx] ^ (invert ? 1 : 0));
      if (str.size())
          std::cout << str;

      ++iter;
    }

    if (gate.mType != GateType::a) {
      u64 idx0 = gate.mInput[0];
      u64 idx1 = gate.mInput[1];
      u64 idx2 = gate.mOutput;

      auto error = gate.mType == GateType::a ||
        gate.mType == GateType::b ||
        gate.mType == GateType::na ||
        gate.mType == GateType::nb ||
        gate.mType == GateType::One ||
        gate.mType == GateType::Zero ||
        outOfDate[idx0] || outOfDate[idx1];

      if (!error) {
        u8 a = mem[idx0];
        u8 b = mem[idx1];

        mem[idx2] = GateEval(gate.mType, (bool)a, (bool)b);

        if (mLevelCounts.size() && isLinear(gate.mType) == false)
          outOfDate[idx2] = 1;
      } else {
          throw std::runtime_error(LOCATION);
      }
    } else {
      u64 src = gate.mInput[0];
      u64 len = gate.mInput[1];
      u64 dest = gate.mOutput;

      memcpy(&*(mem.begin() + dest), &*(mem.begin() + src), len);
    }

    if (--curLevelRem == 0 && i != mGates.size() - 1) {
      std::fill(outOfDate.begin(), outOfDate.end(), 0);
      curLevelRem = *levelRemIter++;
      ++level;
    }
  }

  while (print && iter != mPrints.end()) {
    auto wireIdx = (*iter).mWire;
    auto str = (*iter).mMsg;
    auto invert =  (*iter).mInvert;

    if (wireIdx != ~u32(0))
      std::cout << (u64)(mem[wireIdx] ^ (invert ? 1 : 0));
    if (str.size())
      std::cout << str;

    ++iter;
  }

  if (static_cast<u64>(output.size()) != mOutputs.size()) {
    throw std::runtime_error(LOCATION);
  }

  for (u64 i = 0; i < static_cast<u64>(output.size()); ++i) {
    if (output[i].size() != mOutputs[i].mWires.size())
      throw std::runtime_error(LOCATION);

    for (u64 j = 0; j < output[i].size(); ++j) {
      output[i][j] = mem[mOutputs[i].mWires[j]] ^
      (isInvert(mOutputs[i].mWires[j]) ? 1 : 0);
    }
  }
}

void BetaCircuit::levelByAndDepth() {
  static const i64 freed(-3), uninit(-2); /*, inputWire(-1)*/
  struct Node {
    Node() : mIdx(uninit)
      , mDepth(uninit)
      , mWire(uninit)
      , mFixedWireValue(false) {}

    BetaGate mGate = {};
    i64 mIdx, mDepth, mWire;
    bool mFixedWireValue;
    std::array<Node*, 2> mInput = { nullptr, nullptr };
    std::vector<Node*> mOutputs;
  };

  mPrints.clear();

  mLevelCounts.clear();
  mLevelAndCounts.clear();

  // make the inputs as level zero
  u64 numInputs(0);
  for (u64 i = 0; i < mInputs.size(); ++i)
    numInputs += mInputs[i].mWires.size();

  std::vector<Node*> wireOwner(mWireCount, nullptr);
  std::vector<Node> nodes(mGates.size() + numInputs);
  std::vector<std::vector<Node*>> nodesByLevel;

  for (u64 i = 0; i < numInputs; ++i) {
    auto& curNode = nodes[i];
    curNode.mIdx = i;
    curNode.mDepth = -1;
    wireOwner[i] = &curNode;
    curNode.mWire = i;
    curNode.mFixedWireValue = true;
  }

  for (u64 i = numInputs; i < nodes.size(); i++) {
    auto& curNode = nodes[i];

    curNode.mGate = mGates[i - numInputs];
    curNode.mIdx = i;
    curNode.mDepth = 0;

    for (u64 in = 0; in < 2; ++in) {
      auto& inNode = *wireOwner[curNode.mGate.mInput[in]];
      curNode.mInput[in] = &inNode;

      for (auto& oo : inNode.mOutputs) {
        if (oo->mIdx == curNode.mIdx)
          throw std::runtime_error(LOCATION);
      }

      inNode.mOutputs.emplace_back(&curNode);

      if (inNode.mDepth == -1 || isLinear(inNode.mGate.mType) == false)
        curNode.mDepth = std::max(curNode.mDepth, inNode.mDepth + 1);
      else
        curNode.mDepth = std::max(curNode.mDepth, inNode.mDepth);
    }

    if (static_cast<u64>(curNode.mDepth) >= mLevelCounts.size()) {
      mLevelCounts.resize(curNode.mDepth + 1, 0);
      mLevelAndCounts.resize(curNode.mDepth + 1, 0);
      nodesByLevel.resize(curNode.mDepth + 1);
    }

    wireOwner[curNode.mGate.mOutput] = &curNode;

    nodesByLevel[curNode.mDepth].push_back(&curNode);
    ++mLevelCounts[curNode.mDepth];
    if (isLinear(curNode.mGate.mType) == false)
      ++mLevelAndCounts[curNode.mDepth];
  }

  i64 nextWire = numInputs;
  std::vector<i64> freeWires, nextLvlFreeWires;
  auto getOutWire = [&](Node& node) -> u32 {
    if (node.mFixedWireValue) {
      node.mWire = node.mGate.mOutput;
      return static_cast<u32>(node.mWire);
    } else if (node.mWire == uninit) {
      if (freeWires.size()) {
        node.mWire = freeWires.back();
        freeWires.pop_back();
      } else {
        node.mWire = nextWire++;
      }
      return static_cast<u32>(node.mWire);
    }
    throw std::runtime_error(LOCATION);
  };

  auto getInWire = [&](Node& node, u64 i) -> u32 {
    auto& depList = node.mInput[i]->mOutputs;

    auto iter = std::find(depList.begin(), depList.end(), &node);

    if (iter == depList.end() ||
      std::find(iter + 1, depList.end(), &node) != depList.end())
      throw std::runtime_error(LOCATION);

    auto wire = node.mInput[i]->mWire;
    if (wire < 0)
      throw std::runtime_error(LOCATION);

    if (depList.size() == 1 && node.mInput[i]->mFixedWireValue == false) {
      nextLvlFreeWires.push_back(wire);
      node.mInput[i]->mWire = freed;
    }

    std::swap(*iter, depList.back());
    depList.pop_back();

    return static_cast<u32>(wire);
  };

  for (u64 i = 0; i < mOutputs.size(); ++i) {
    for (u64 j = 0; j < mOutputs[i].mWires.size(); ++j) {
      nextWire = std::max<i64>(nextWire, mOutputs[i].mWires[j] + 1);

      if (wireOwner[mOutputs[i].mWires[j]])
        wireOwner[mOutputs[i].mWires[j]]->mFixedWireValue = true;
    }
  }

  std::vector<i64> nextPosByLevel(mLevelCounts.size());
  for (u64 i = 1; i < nextPosByLevel.size(); ++i) {
    nextPosByLevel[i] += mLevelCounts[i - 1] + nextPosByLevel[i - 1];
  }

  std::vector<BetaGate> sortedGates(mGates.size());

  // u64 ii = 0;
  auto iter = sortedGates.begin();
  for (auto& level : nodesByLevel) {
    for (auto& node : level)  {
      *iter = node->mGate;
      iter->mInput[0] = getInWire(*node, 0);
      iter->mInput[1] = getInWire(*node, 1);
      iter->mOutput = getOutWire(*node);
      ++iter;
    }
    freeWires.insert(freeWires.begin(), nextLvlFreeWires.begin(),
      nextLvlFreeWires.end());
    nextLvlFreeWires.clear();
  }

  // replace the gates with the sorted version
  mWireCount = static_cast<BetaWire>(nextWire);
  mWireFlags.resize(mWireCount, BetaWireFlag::Wire);
  mGates = std::move(sortedGates);
}

void BetaCircuit::levelByAndDepth(LevelizeType type) {
  if (type == LevelizeType::Reorder) {
    levelByAndDepth();
  } else {
    auto iter = mGates.begin();

    u64 curCount = 0;
    while (iter != mGates.end()) {
      ++curCount;
      if (isLinear(iter->mType) == false) {
        mLevelCounts.push_back(curCount);
        mLevelAndCounts.push_back(1);
        curCount = 0;
      } else if (type == LevelizeType::SingleNoReorder) {
        mLevelCounts.push_back(curCount);
        mLevelAndCounts.push_back(0);
        curCount = 0;
      }
      ++iter;
    }

    if (curCount) {
      mLevelCounts.push_back(curCount);
      mLevelAndCounts.push_back(0);
    }
  }
}

#ifdef USE_JSON
void BetaCircuit::writeJson(std::ostream & out) {
  json j;
  if (mName.size())
    j["name"] = mName;

  j["format"] = {
    {"name", "BetaCircuit"},
    {"version", 1}
  };

  auto& inputs = j["inputs"];
  for (u64 i = 0; i < mInputs.size(); ++i) {
    inputs[i] = mInputs[i].mWires;
  }

  std::vector<i32> inverts;
  auto& outputs = j["outputs"];
  for (u64 i = 0; i < mOutputs.size(); ++i) {
    outputs[i] = mOutputs[i].mWires;
  }

  j["wire-count"] = mWireCount;

  auto& gates = j["gates"];
  for (u64 i = 0; i < mGates.size(); ++i) {
    gates[i] = {
      (i32)mGates[i].mType,
      mGates[i].mInput[0] ,
      mGates[i].mInput[1] ,
      mGates[i].mOutput };
  }

  if (mWireFlags.size() != mWireCount)
    throw std::runtime_error(LOCATION);

  j["wire-flags"] = mWireFlags;

  if (mLevelCounts.size())
    j["levels"] = mLevelCounts;

  // if (mPrints.size())
    // j["prints"] = mPrints;


  auto hashVal = hash();
  j["hash"] = { ((u64*)&hashVal)[0] , ((u64*)&hashVal)[1] };

  out << j;
}

void BetaCircuit::readJson(std::istream & in) {
  json j;
  in >> j;

  if (j["name"].empty() == false)
    mName = j["name"].get<std::string>();

  auto& format = j["format"];

  if (format["name"] != "BetaCircuit")
    throw std::runtime_error("wrong format name");

  if (format["version"] != 1)
    throw std::runtime_error("wrong format version");

  auto& inputs = j["inputs"];
  for (u64 i = 0; i < inputs.size(); ++i) {
    BetaBundle input(inputs[i].size());
    addInputBundle(input);

    for (u64 j = 0; j < input.size(); ++j) {
      if (inputs[i][j].is_number_integer() == false)
        throw std::runtime_error("input vals must be integer");

      if (inputs[i][j].get<i32>() != input.mWires[j])
        throw std::runtime_error("input wire index must be monotimicly increasing by 1");
    }
  }

  auto& outputs = j["outputs"];
  for (u64 i = 0; i < outputs.size(); ++i) {
    BetaBundle output(outputs[i].size());
    addOutputBundle(output);

    for (u64 j = 0; j < output.size(); ++j) {
      if (outputs[i][j].is_number_integer() == false)
        throw std::runtime_error("output vals must be integer");

      auto act = outputs[i][j].get<i32>();
      auto exp = output.mWires[j];
      if (act != exp)
        throw std::runtime_error("output wire index must be monotimicly increasing by 1");
    }
  }

  i32 wireCount = j["wire-count"];
  if (wireCount < mWireCount)
    throw std::runtime_error("bad wire count");

  BetaBundle temp(wireCount - mWireCount);
  addTempWireBundle(temp);
  mNonlinearGateCount = 0;

  auto& gates = j["gates"];
  for (u64 i = 0; i < gates.size(); ++i) {
    std::array<u32, 4> vals = gates[i];
    if (vals[0] > (u32)GateType::One)
      throw std::runtime_error("bad gate type");

    GateType gt = (GateType)vals[0];

    if (vals[1] == vals[2])
      throw std::runtime_error(LOCATION);

    if (vals[3] >= mWireCount)
      throw std::runtime_error(LOCATION);

    mGates.emplace_back(vals[1], vals[2], gt, vals[3]);

    mNonlinearGateCount += (isLinear(gt) == false);
  }

  auto& flags = j["wire-flags"];
  if (flags.size() != mWireCount) {
    std::cout << "bad wire flag count. act: " << flags.size()
      << ", exp: " << mWireCount << std::endl;
    throw std::runtime_error("bad wire flags");
  }
  mWireFlags.resize(mWireCount);

  for (u64 i = 0; i < mWireCount; ++i)
    mWireFlags[i] = flags[i];

  auto& levels = j["levels"];
  mLevelCounts.resize(levels.size());
  mLevelAndCounts.resize(levels.size());
  u32 gateIdx = 0;
  for (u64 i = 0; i < mLevelCounts.size(); ++i) {
    mLevelCounts[i] = levels[i];
    mLevelAndCounts[i] = 0;

    auto end = gateIdx + mLevelCounts[i];
    while (gateIdx != end) {
      if (isLinear(mGates[gateIdx].mType) == false)
        ++mLevelAndCounts[i];

      ++gateIdx;
    }
  }

  // auto& prints = j["prints"];
  // mPrints.resize(prints.size());
  // for (u64 i = 0; i < mPrints.size(); ++i) {
  //   mPrints[i] = prints[i];
  // }

  u64 h0 = j["hash"][0];
  u64 h1 = j["hash"][1];
  auto fileHashVal = toBlock(h1, h0);
  auto hashVal = hash();

  if (neq(hashVal, fileHashVal)) {
    std::cout << "hash values do not match, file:" << fileHashVal
      << ", cur: " << hashVal << std::endl;

    throw std::runtime_error("hash values do not match");
  }
}  // namespace primihub

#endif

template<typename T>
void write(T* ptr, u64 size, std::ostream& o) {
  static_assert(std::is_pod<T>::value, "must be pod");
  o.write((char*)ptr, size * sizeof(T));
}

void write(u64 v, std::ostream& o) {
  o.write((char*)&v, sizeof(u64));
}

void write(const std::string& str, std::ostream& o) {
  write(str.size(), o);
  o.write(str.data(), str.size());
}

void BetaCircuit::writeBin(std::ostream & out) {
  // name
  write(mName, out);

  write("BetaCircuit", out);
  write(1, out);

  write(mInputs.size(), out);
  for (u64 i = 0; i < mInputs.size(); ++i) {
    write(mInputs[i].size(), out);
    write(mInputs[i].mWires.data(), mInputs[i].mWires.size(), out);
  }

  write(mOutputs.size(), out);
  for (u64 i = 0; i < mOutputs.size(); ++i) {
    write(mOutputs[i].size(), out);
    write(mOutputs[i].mWires.data(), mOutputs[i].mWires.size(), out);
  }

  write(mWireCount, out);

  write(mGates.size(), out);
  write(mGates.data(), mGates.size(), out);

  if (mWireFlags.size() != mWireCount)
    throw std::runtime_error(LOCATION);

  write(mWireFlags.data(), mWireFlags.size(), out);

  write(mLevelCounts.size(), out);
  write(mLevelCounts.data(), mLevelCounts.size(), out);
  write(mLevelAndCounts.data(), mLevelAndCounts.size(), out);

  write(mPrints.size(), out);
  for (u64 i = 0; i < mPrints.size(); ++i) {
    // std::tuple<u64, BetaWire, std::string, bool>
    write((mPrints[i].mGateIdx), out);
    write((mPrints[i].mWire), out);
    write((mPrints[i].mMsg), out);
    write((mPrints[i].mInvert), out);
  }

  auto hashVal = hash();
  write(&hashVal, 1, out);
}

void read(u64& v, std::istream& in) {
  in.read((char*)&v, sizeof(u64));
}

void read(std::string& str, std::istream& in) {
  u64 size;
  read(size, in);
  str.resize(size);
  in.read((char*)str.data(), size);
}

template<typename T>
void read(T* dest, u64 size, std::istream& in) {
  static_assert(std::is_pod<T>::value, "must be pod");
  in.read((char*)dest, size * sizeof(T));
}

void BetaCircuit::readBin(std::istream & in) {
  // name
  read(mName, in);

  std::string format;
  read(format, in);
  if (format != "BetaCircuit")
      throw std::runtime_error(LOCATION);

  u64 version;
  read(version, in);
  if (version != 1)
      throw std::runtime_error(LOCATION);

  u64 count;
  read(count, in);
  mInputs.resize(count);
  for (u64 i = 0; i < mInputs.size(); ++i) {
    read(count, in);
    mInputs[i].mWires.resize(count);
    read(mInputs[i].mWires.data(), mInputs[i].mWires.size(), in);
  }

  read(count, in);
  mOutputs.resize(count);
  for (u64 i = 0; i < mOutputs.size(); ++i) {
    read(count, in);
    mOutputs[i].mWires.resize(count);
    read(mOutputs[i].mWires.data(), mOutputs[i].mWires.size(), in);
  }

  read(count, in);
  mWireCount = static_cast<u32>(count);

  read(count, in);
  mGates.resize(count);
  read(mGates.data(), mGates.size(), in);

  mWireFlags.resize(mWireCount);
  read(mWireFlags.data(), mWireFlags.size(), in);

  read(count, in);
  mLevelCounts.resize(count);
  mLevelAndCounts.resize(count);
  read(mLevelCounts.data(), mLevelCounts.size(), in);
  read(mLevelAndCounts.data(), mLevelAndCounts.size(), in);

  mNonlinearGateCount = std::accumulate(mLevelAndCounts.begin(),
    mLevelAndCounts.end(), 0ull);

  std::string msg;
  read(count, in);
  mPrints.resize(count);
  for (u64 i = 0; i < mPrints.size(); ++i) {
    u64 gateIdx, wireIdx, flag;
    read(gateIdx, in);
    read(wireIdx, in);
    read(msg, in);
    read(flag, in);

    (mPrints[i].mGateIdx) = gateIdx;
    (mPrints[i].mWire) = static_cast<u32>(wireIdx);
    (mPrints[i].mMsg) = msg;
    (mPrints[i].mInvert) = flag;
  }

  block fileHashValue;
  read(&fileHashValue, 1, in);
  auto hashVal = hash();
  if (neq(fileHashValue, hashVal)) {
    throw std::runtime_error(LOCATION);
  }
}

struct GateNode {
  GateType mType;
  std::array<GateNode*, 2> mInputs;
  i64 mNewOutput = -1;
};

void BetaCircuit::writeBristol(std::ostream & out)const {
  if (mInputs.size() != 2)
    throw std::runtime_error("bristol only supports two inputs. Merge into for a single party...");

  auto inputCount = (mInputs[0].size() + mInputs[1].size());
  auto numOut = 0ull;
  for (auto o : mOutputs)
    numOut += o.size();

  std::list<GateNode> nodes;
  std::vector<GateNode*> wireOwners(mWireCount, nullptr);

  for (u64 i = 0; i < inputCount; ++i) {
    nodes.emplace_back();
    nodes.back().mNewOutput = i;
    wireOwners[i] = &nodes.back();
  }

  for (u64 i = 0; i < mGates.size(); ++i) {
    switch (mGates[i].mType) {
      case GateType::And:
      case GateType::Xor: {
        nodes.emplace_back();
        auto node = &nodes.back();

        node->mInputs[0] = wireOwners[mGates[i].mInput[0]];
        node->mInputs[1] = wireOwners[mGates[i].mInput[1]];
        node->mType = mGates[i].mType;
        wireOwners[mGates[i].mOutput] = node;
        break;
      }
      case GateType::na: {
        nodes.emplace_back();
        auto node = &nodes.back();

        node->mInputs[0] = wireOwners[mGates[i].mInput[0]];
        node->mType = GateType::na;
        wireOwners[mGates[i].mOutput] = node;
        break;
      }
      case GateType::Nxor: {
        nodes.emplace_back();
        auto node1 = &nodes.back();
        nodes.emplace_back();
        auto node2 = &nodes.back();

        node1->mInputs[0] = wireOwners[mGates[i].mInput[0]];
        node1->mInputs[1] = wireOwners[mGates[i].mInput[1]];
        node1->mType = GateType::Xor;

        node2->mInputs[0] = node1;
        node2->mType = GateType::na;
        wireOwners[mGates[i].mOutput] = node2;
        break;
      }
      case GateType::a: {
        for (u64 j = 0; j < mGates[i].mInput[1]; ++j) {
          nodes.emplace_back();
          auto node1 = &nodes.back();
          nodes.emplace_back();
          auto node2 = &nodes.back();

          node1->mInputs[0] = wireOwners[mGates[i].mInput[0]];
          node1->mType = GateType::na;

          node2->mInputs[0] = node1;
          node2->mType = GateType::na;
          wireOwners[mGates[i].mOutput] = node2;
        }
        break;
      }
      default:
        std::cout << gateToString(mGates[i].mType)
          << " not supported" << std::endl;
        throw std::runtime_error("Currently only supports {AND, XOR, NXOR, NOT, Copy} gates. " LOCATION);
        break;
    }
  }

  for (u64 i = 0; i < mOutputs.size(); ++i) {
    for (u64 j = 0; j < mOutputs[i].size(); ++j) {
      if (isInvert(mOutputs[i].mWires[j])) {
        nodes.emplace_back();
        auto node = &nodes.back();

        node->mInputs[0] = wireOwners[mOutputs[i].mWires[j]];
        node->mType = GateType::na;
        wireOwners[mOutputs[i].mWires[j]] = node;
      }
    }
  }

  auto gateCount = nodes.size() - inputCount;
  u64 nextWire = nodes.size() - numOut;
  for (u64 i = 0; i < mOutputs.size(); ++i) {
    for (u64 j = 0; j < mOutputs[i].size(); ++j) {
      auto node = wireOwners[mOutputs[i].mWires[j]];
      node->mNewOutput = nextWire++;
    }
  }

  nextWire = inputCount;

  out << gateCount << " " << nodes.size() << "\n";
  out << mInputs[0].size() << " " << mInputs[1].size() << " "
    << numOut << "\n\n";

  u64 inIdx0, inIdx1, outIdx;
  auto nodeIter = nodes.begin();
  std::advance(nodeIter, inputCount);
  for (u64 i = inputCount; i < nodes.size(); ++i, ++nodeIter) {
    if (nodeIter->mNewOutput == -1)
      nodeIter->mNewOutput = nextWire++;

    inIdx0 = nodeIter->mInputs[0]->mNewOutput;
    outIdx = nodeIter->mNewOutput;

    switch (nodeIter->mType) {
      case GateType::And:
        inIdx1 = nodeIter->mInputs[1]->mNewOutput;
        out << "2 1 " << inIdx0 << " " << inIdx1 << " " << outIdx << " AND\n";
        break;
      case GateType::Xor:
        inIdx1 = nodeIter->mInputs[1]->mNewOutput;
        out << "2 1 " << inIdx0 << " " << inIdx1 << " " << outIdx << " XOR\n";
        break;
      case GateType::na:
        out << "1 1 " << inIdx0 << " " << outIdx << " INV\n";
        break;
      default:
        throw RTE_LOC;
    }
  }
}

void BetaCircuit::readBristol(std::istream & in) {
  // call the destructor and constructor to make sure this object is empty;
  // BetaCircuit::~BetaCircuit();
  // new (this)BetaCircuit();

  if (in.good() == false)
    throw RTE_LOC;

  u64 numGates, wireCount, numInput0, numInput1, numOutputs;

  in >> numGates >> wireCount >> numInput0 >> numInput1 >> numOutputs;

  if (!numGates || !wireCount || !numInput0 || !numInput1 || !numOutputs)
    throw RTE_LOC;

  BetaBundle input0(numInput0);
  BetaBundle input1(numInput1);
  BetaBundle output(numOutputs);
  BetaBundle internal(wireCount - numInput0 - numInput1 - numOutputs);
  addInputBundle(input0);
  addInputBundle(input1);
  addOutputBundle(output);
  addTempWireBundle(internal);

  // our outputs are the wires right after the inputs while bristol's 
  // are at the end. This function fixes this by offsetting the wire 
  // idxs approperatly.
  auto translate = [&](u64 idx) {
    // the first (numInput0 + numInput1) wires are inputs.
    auto isInput = idx < (numInput0 + numInput1);
    if (isInput)
      return idx;

    // the last numOutputs wires are outputs.
    auto isOutput = idx >= (wireCount - numOutputs);
    if (isOutput)
      return idx - internal.size();

    return idx + numOutputs;
  };

  std::string gate;
  u64 fanIn, fanOut, inIdx0, inIdx1, outIdx;
  GateType gt;
  for (u64 i = 0; i < numGates; ++i) {
    in >> fanIn >> fanOut;

    if (fanIn - 1 > 1 || fanOut != 1)
      throw RTE_LOC;

    if (fanIn == 1) {
      in >> inIdx0 >> outIdx >> gate;

      inIdx0 = translate(inIdx0);
      outIdx = translate(outIdx);

      if (gate != "INV")
        throw RTE_LOC;

      if (inIdx0 == outIdx)
        addInvert(static_cast<u32>(inIdx0));
      else
        addInvert(static_cast<u32>(inIdx0), static_cast<u32>(outIdx));
    } else {
      in >> inIdx0 >> inIdx1 >> outIdx >> gate;

      inIdx0 = translate(inIdx0);
      inIdx1 = translate(inIdx1);
      outIdx = translate(outIdx);

      if (gate == "AND")
          gt = GateType::And;
      else if (gate == "XOR")
          gt = GateType::Xor;
      else
          throw RTE_LOC;

      addGate(static_cast<u32>(inIdx0), static_cast<u32>(inIdx1),
          gt, static_cast<u32>(outIdx));
    }
  }
}

block BetaCircuit::hash() const {
  RandomOracle sha(sizeof(block));
  sha.Update(mWireCount);

  sha.Update(mGates.data(), mGates.size());

  for (u64 i = 0; i < mPrints.size(); ++i) {
    // std::tuple<u64, BetaWire, std::string, bool>
    auto v0 = (mPrints[i].mGateIdx);
    auto v1 = (mPrints[i].mWire);
    auto v2 = (mPrints[i].mMsg);
    auto v3 = (mPrints[i].mInvert);

    sha.Update(v0);
    sha.Update(v1);
    sha.Update(v2.data(), v2.size());
    sha.Update(v3);
  }

  sha.Update(mWireFlags.data(), mWireFlags.size());

  sha.Update(mLevelCounts.size());
  sha.Update(mLevelCounts.data(), mLevelCounts.size());
  sha.Update(mLevelAndCounts.data(), mLevelAndCounts.size());

  sha.Update(mInputs.size());
  for (u64 i = 0; i < mInputs.size(); ++i)
      sha.Update(mInputs[i].mWires.data(), mInputs[i].mWires.size());

  sha.Update(mOutputs.size());
  for (u64 i = 0; i < mOutputs.size(); ++i)
      sha.Update(mOutputs[i].mWires.data(), mOutputs[i].mWires.size());

  block r;
  sha.Final(r);
  return r;
}

bool BetaCircuit::operator!=(const BetaCircuit& rhs) const {
  if (mWireCount != rhs.mWireCount)
    return true;

  if (mGates.size() != rhs.mGates.size())
    return true;

  for (u64 i = 0; i < mGates.size(); ++i) {
    if (mGates[i] != rhs.mGates[i])
      return true;
  }

  if (mPrints.size() != rhs.mPrints.size())
    return true;

  for (u64 i = 0; i < mPrints.size(); ++i) {
    if (mPrints[i] != rhs.mPrints[i])
      return true;
  }

  if (mWireFlags.size() != rhs.mWireFlags.size())
    return true;

  for (u64 i = 0; i < mWireFlags.size(); ++i) {
    if (mWireFlags[i] != rhs.mWireFlags[i])
      return true;
  }

  if (mLevelCounts.size() != rhs.mLevelCounts.size())
    return true;

  for (u64 i = 0; i < mLevelCounts.size(); ++i) {
    if (mLevelCounts[i] != rhs.mLevelCounts[i])
      return true;
  }

  if (mLevelAndCounts.size() != rhs.mLevelAndCounts.size())
    return true;

  for (u64 i = 0; i < mLevelAndCounts.size(); ++i) {
    if (mLevelAndCounts[i] != rhs.mLevelAndCounts[i])
      return true;
  }

  if (mInputs.size() != rhs.mInputs.size())
    return true;

  for (u64 i = 0; i < mInputs.size(); ++i) {
    for (u64 j = 0; j < mInputs[i].mWires.size(); ++j) {
      if (mInputs[i].mWires[j] != rhs.mInputs[i].mWires[j])
        return true;
    }
  }

  if (mOutputs.size() != rhs.mOutputs.size())
    return true;

  for (u64 i = 0; i < mOutputs.size(); ++i) {
    for (u64 j = 0; j < mOutputs[i].mWires.size(); ++j) {
      if (mOutputs[i].mWires[j] != rhs.mOutputs[i].mWires[j])
        return true;
    }
  }

  return false;
}

}  // namespace primihub

#endif
