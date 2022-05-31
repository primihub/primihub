
#include "src/primihub/algorithm/aby3ML.h"

namespace primihub {

void aby3ML::init(u64 partyIdx, Session& prev, Session& next,
  block seed) {
  mPreproPrev = prev.addChannel(); 
  mPreproNext = next.addChannel();
  mPrev = prev.addChannel();
  mNext = next.addChannel();

  auto commPtr = std::make_shared<CommPkg>(mPrev, mNext);
  
  mRt.init(partyIdx, commPtr);

  PRNG prng(seed);
  mEnc.init(partyIdx, *commPtr.get(), prng.get<block>());
  mEval.init(partyIdx, *commPtr.get(), prng.get<block>());
}

void aby3ML::fini(void) {
  mPreproPrev.close();
  mPreproNext.close();
  mPrev.close();
  mNext.close();
}

}  // namespace primihub
