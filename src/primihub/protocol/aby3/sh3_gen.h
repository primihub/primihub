
#ifndef SRC_primihub_PROTOCOL_ABY3_SH3_GEN_H_
#define SRC_primihub_PROTOCOL_ABY3_SH3_GEN_H_

#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/crypto/prng.h"

namespace primihub {

using PackedBin = PackedBinBase<i64>;
using PackedBin128 = PackedBinBase<block>;

struct Sh3ShareGen {
  u64 mShareIdx = 0, mShareGenIdx = 0;
  PRNG mNextCommon, mPrevCommon, mCommon;
  std::array<AES_Type, 2> mShareGen;
  std::array<std::vector<block>, 2> mShareBuff;

  void init(block prevSeed, block nextSeed, u64 buffSize = 256) {
    mCommon.SetSeed(toBlock(3488535245, 2454523));
    mNextCommon.SetSeed(nextSeed);
    mPrevCommon.SetSeed(prevSeed);

    mShareGenIdx = 0;
    mShareBuff[0].resize(buffSize);
    mShareBuff[1].resize(buffSize);

    mShareGen[0].setKey(mPrevCommon.get<block>());
    mShareGen[1].setKey(mNextCommon.get<block>());

    refillBuffer();
  }

  void init(CommPkg& comm, block& seed, u64 buffSize = 256) {
    comm.mNext().asyncSendCopy(seed);
    block prevSeed;
    comm.mPrev().recv(prevSeed);
    
    init(prevSeed, seed, buffSize);
  }

  void refillBuffer() {
    mShareGen[0].ecbEncCounterMode(mShareGenIdx, mShareBuff[0].size(),
                                   mShareBuff[0].data());
    mShareGen[1].ecbEncCounterMode(mShareGenIdx, mShareBuff[1].size(),
                                   mShareBuff[1].data());
    mShareGenIdx += mShareBuff[0].size();
    mShareIdx = 0;
  }

  i64 getShare() {
    if (mShareIdx + sizeof(i64) > mShareBuff[0].size() * sizeof(block)) {
      refillBuffer();
    }

    i64 ret = *reinterpret_cast<u64*>(reinterpret_cast<u8*>(mShareBuff[0].data()) + mShareIdx)
      - *reinterpret_cast<u64*>(reinterpret_cast<u8*>(mShareBuff[1].data()) + mShareIdx);

    mShareIdx += sizeof(i64);

    // ret = 0;
    return ret;
  }

  i64 getBinaryShare() {
    if (mShareIdx + sizeof(i64) > mShareBuff[0].size() * sizeof(block)) {
      refillBuffer();
    }

    i64 ret = *reinterpret_cast<u64*>(reinterpret_cast<u8*>(mShareBuff[0].data()) + mShareIdx)
      ^ *reinterpret_cast<u64*>(reinterpret_cast<u8*>(mShareBuff[1].data()) + mShareIdx);

    mShareIdx += sizeof(i64);

    // ret = 0;
    return ret;
  }

  si64 getRandIntShare() {
    if (mShareIdx + sizeof(i64) > mShareBuff[0].size() * sizeof(block)) {
      refillBuffer();
    }

    si64 r;
    r[0] = *reinterpret_cast<u64*>(
      reinterpret_cast<u8*>(mShareBuff[1].data()) + mShareIdx);
    r[1] = *reinterpret_cast<u64*>(
      reinterpret_cast<u8*>(mShareBuff[0].data()) + mShareIdx);

    mShareIdx += sizeof(i64);

    return r;
  }

  sb64 getRandBinaryShare() {
    auto i = getRandIntShare();
    return { { i[0], i[1] } };
  }

};

}

#endif  // SRC_primihub_PROTOCOL_ABY3_SH3_GEN_H_
