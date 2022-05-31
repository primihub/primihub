
#ifndef SRC_primihub_PROTOCOL_ABY3_EVALUATOR_BINARY_EVALUATOR_H_
#define SRC_primihub_PROTOCOL_ABY3_EVALUATOR_BINARY_EVALUATOR_H_

#include <vector>
#include <iomanip>
// #include <immintrin.h>

#include "boost/align/aligned_allocator.hpp"

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/common/type/matrix.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/protocol/aby3/transpose.h"
#include "src/primihub/primitive/circuit/beta_circuit.h"
#include "src/primihub/util/crypto/random_oracle.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/log.h"

namespace primihub {

class Sh3BinaryEvaluator {
 public:
#if defined(__AVX2__) || defined(_MSC_VER)
        using block_type = __m256i;
#else
        using block_type = block;
#endif

#define BINARY_ENGINE_DEBUG

#ifdef BINARY_ENGINE_DEBUG

 private:
  bool mDebug = false;

 public:
  void enableDebug(u64 partyIdx, Channel debugPrev, Channel debugNext) {
      mDebug = true;
      mDebugPartyIdx = partyIdx;

      mDebugPrev = debugPrev;
      mDebugNext = debugNext;
  }

  struct DEBUG_Triple {
    std::array<u8, 3> mBits;
    bool mIsSet;
    u16 val()const { return mBits[0] ^ mBits[1] ^ mBits[2]; }

    void assign(const DEBUG_Triple& in0, const DEBUG_Triple& in1,
      GateType type);
  };

  std::vector<std::vector<DEBUG_Triple>> mPlainWires_DEBUG;
  u64 mDebugPartyIdx = -1;
  Channel mDebugPrev, mDebugNext;

  u8 extractBitShare(u64 rowIdx, u64 wireIdx, u64 shareIdx);

  void validateMemory();
  void validateWire(u64 wireIdx);
  void distributeInputs();

  block hashDebugState();
  std::stringstream mLog;
#endif

  BetaCircuit* mCir;
  std::vector<BetaGate>::iterator mGateIter;
  u64 mLevel;
  std::vector<u32> mRecvLocs;
  std::vector<u8> mRecvData;
  std::array<std::vector<u8>, 2> mSendBuffs;

  std::vector<std::future<void>> mRecvFutr;
  sPackedBinBase<block_type> mMem;

  Sh3Task asyncEvaluate(
    Sh3Task dependency,
    BetaCircuit* cir,
    Sh3ShareGen& gen,
    std::vector<const sbMatrix*> inputs,
    std::vector<sbMatrix*> outputs);

  void setCir(BetaCircuit* cir, u64 width, Sh3ShareGen& gen) {
    block p, n;
    p = gen.mPrevCommon.get();
    n = gen.mNextCommon.get();
    setCir(cir, width, p, n);
  }

  void setCir(BetaCircuit* cir, u64 width, block prevSeed, block nextSeed);

  void setReplicatedInput(u64 i, const sbMatrix& in);
  void setInput(u64 i, const sbMatrix& in);
  void setInput(u64 i, const sPackedBin& in);

  Sh3Task asyncEvaluate(Sh3Task dependency);

  void roundCallback(CommPkg& comms, Sh3Task task);

  void getOutput(u64 i, sPackedBin& out);
  void getOutput(const std::vector<BetaWire>& wires,
    sPackedBin& out);

  void getOutput(u64 i, sbMatrix& out);
  void getOutput(const std::vector<BetaWire>& wires, sbMatrix& out);

  bool hasMoreRounds() const {
    return mLevel <= mCir->mLevelCounts.size();
  }

  block hashState() {
    RandomOracle h(sizeof(block));
    h.Update(mMem.mShares[0].data(), mMem.mShares[0].size());
    h.Update(mMem.mShares[1].data(), mMem.mShares[1].size());

    block b;
    h.Final(b);
    return b;
  }

  // std::unique_ptr<block_type[]> mShareBacking;
  std::vector<block_type,boost::alignment::aligned_allocator<block_type>> mShareBuff;
  // span<block_type> mShareBuff;
  u64 mShareIdx;
  std::array<AES_Type, 2> mShareAES;

  block_type* getShares();
  Sh3ShareGen mShareGen;
  block_type mCheckBlock;

  PRNG mPrng;
  // std::array<PRNG, 2> mGens;
};

}  // namespace primihub

#endif  // SRC_primihub_PROTOCOL_ABY3_EVALUATOR_BINARY_EVALUATOR_H_
