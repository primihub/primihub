// Copyright [2021] <primihub.com>
#include "src/primihub/util/crypto/prng.h"

namespace primihub {

PRNG::PRNG(const block& seed, uint64_t bufferSize) :
  mBytesIdx(0),
  mBlockIdx(0) {
  SetSeed(seed, bufferSize);
}

PRNG::PRNG(PRNG && s) :
  mBuffer(std::move(s.mBuffer)),
  mAes(std::move(s.mAes)),
  mBytesIdx(s.mBytesIdx),
  mBlockIdx(s.mBlockIdx),
  mBufferByteCapacity(s.mBufferByteCapacity) {
  s.mBuffer.resize(0);
  s.mBytesIdx = 0;
  s.mBlockIdx = 0;
  s.mBufferByteCapacity = 0;
}

void PRNG::operator=(PRNG&&s)  {
  mBuffer = (std::move(s.mBuffer));
  mAes = (std::move(s.mAes));
  mBytesIdx = (s.mBytesIdx);
  mBlockIdx = (s.mBlockIdx);
  mBufferByteCapacity = (s.mBufferByteCapacity);

  s.mBuffer.resize(0);
  s.mBytesIdx = 0;
  s.mBlockIdx = 0;
  s.mBufferByteCapacity = 0;
}

void PRNG::SetSeed(const block& seed, uint64_t bufferSize) {
  mAes.setKey(seed);
  mBlockIdx = 0;

  if (mBuffer.size() == 0) {
    mBuffer.resize(bufferSize);
    mBufferByteCapacity = (sizeof(block) * bufferSize);
  }

  refillBuffer();
}

void PRNG::implGet(u8* destu8, uint64_t lengthu8) {
  while (lengthu8) {
    uint64_t step = std::min(lengthu8, mBufferByteCapacity - mBytesIdx);
    memcpy(destu8, ((u8*)mBuffer.data()) + mBytesIdx, step);
    destu8 += step;
    lengthu8 -= step;
    mBytesIdx += step;
    if (mBytesIdx == mBufferByteCapacity) {
      if (lengthu8 >= 8 * sizeof(block)) {
        span<block> b((block*)destu8, lengthu8 / sizeof(block));
        mAes.ecbEncCounterMode(mBlockIdx, b.size(), b.data());
        mBlockIdx += b.size(); 
        step = b.size() * sizeof(block);
        destu8 += step;
        lengthu8 -= step;
      }
      refillBuffer();
    }
  }
}

u8 PRNG::getBit() {
  return get<bool>();
}

const block PRNG::getSeed() const {
  if(mBuffer.size())
    return mAes.mRoundKey[0];
  throw std::runtime_error("PRNG has not been keyed " LOCATION);
}

void PRNG::refillBuffer() {
  if (mBuffer.size() == 0)
    throw std::runtime_error("PRNG has not been keyed " LOCATION);

  mAes.ecbEncCounterMode(mBlockIdx, mBuffer.size(), mBuffer.data());
  mBlockIdx += mBuffer.size();
  mBytesIdx = 0;
}

}
