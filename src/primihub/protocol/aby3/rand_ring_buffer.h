
#ifndef SRC_PRIMIHUB_PROTOCOL_RAND_RING_BUFFER_H_
#define SRC_PRIMIHUB_PROTOCOL_RAND_RING_BUFFER_H_

#include "src/primihub/common/type/type.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/crypto/block.h"

namespace primihub {

struct RandRingBuffer {
  u64 mIdx = 0, mNonce = 0;
  PRNG mLocalPrng, mFixedPrng;
  AES_Type aesInst;
  std::vector<block> mRingBuff;

  void init(const block& seed, u64 buffSize = 256) {
    // not used
    mFixedPrng.SetSeed(toBlock(3488535245, 2454523));
    
    mLocalPrng.SetSeed(seed);

    mNonce = 0;
    mRingBuff.resize(buffSize);

    aesInst.setKey(mLocalPrng.get<block>());

    refillBuffer();
  }

  void refillBuffer() {
    aesInst.ecbEncCounterMode(mNonce, mRingBuff.size(), mRingBuff.data());
    mNonce += mRingBuff.size();
    mIdx = 0;
  }

  i64 getShare() {
    if (mIdx + sizeof(i64) > mRingBuff.size() * sizeof(block)) {
      refillBuffer();
    }

    i64 ret = *reinterpret_cast<u64*>(reinterpret_cast<u8*>(mRingBuff.data()) + mIdx);

    mIdx += sizeof(i64);
    return ret;
  }

};

struct ABY3RandBuffer {
  RandRingBuffer prevRingBuffer, localRingBuffer;

  void init(const block& prevSeed, const block& seed, u64 buffSize = 256) {
    prevRingBuffer.init(prevSeed, buffSize);
    localRingBuffer.init(seed, buffSize);
  }

  i64 getShare() {
    i64 ret = prevRingBuffer.getShare() - localRingBuffer.getShare();
    return ret;
  }

  i64 getBinaryShare() {

    i64 ret = prevRingBuffer.getShare() ^ localRingBuffer.getShare();
    return ret;
  }

  si64 getRandIntShare() {
    si64 r;
    r[0] = prevRingBuffer.getShare();
    r[1] = localRingBuffer.getShare();

    return r;
  }

  sb64 getRandBinaryShare() {
    auto i = getRandIntShare();
    return { { i[0], i[1] } };
  }
};

}

#endif  // SRC_primihub_PROTOCOL_ABY3_SH3_GEN_H_
