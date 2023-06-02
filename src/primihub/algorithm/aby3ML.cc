
#include "src/primihub/algorithm/aby3ML.h"

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
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
#else
void aby3ML::init(u64 partyIdx, MpcChannel &prev, MpcChannel &next, block seed) {
  mNext = next;
  mPrev = prev;

  auto commPtr = std::make_shared<CommPkg>(mPrev, mNext);
  mRt.init(partyIdx, commPtr);
  LOG(INFO) << "Runtime init finish.";

  PRNG prng(seed);
  mEnc.init(partyIdx, *commPtr.get(), prng.get<block>());
  LOG(INFO) << "Encryptor init finish.";

  mEval.init(partyIdx, *commPtr.get(), prng.get<block>());
  LOG(INFO) << "Evaluator init finish.";
}
void aby3ML::fini(void) {}
#endif

}  // namespace primihub
