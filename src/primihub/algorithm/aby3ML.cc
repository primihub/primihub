// "Copyright [2021] <Primihub>"

#include "src/primihub/algorithm/aby3ML.h"
#include <glog/logging.h>
#include <memory>

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
void aby3ML::init(u64 partyIdx, Session& prev,
                  Session& next, block seed) {
  // mPreproPrev = prev.addChannel();
  // mPreproNext = next.addChannel();
  comm_pkg_ = std::make_unique<aby3::CommPkg>();
  auto mPrev = prev.addChannel();
  auto mNext = next.addChannel();

  // auto commPtr = std::make_shared<CommPkg>(mPrev, mNext);
  comm_pkg_->mNext = next.addChannel();
  comm_pkg_->mPrev = prev.addChannel();
  mRt.init(partyIdx, *comm_pkg_);

  osuCrypto::PRNG prng(seed);
  mEnc.init(partyIdx, *comm_pkg_, prng.get<block>());
  mEval.init(partyIdx, *comm_pkg_, prng.get<block>());
}

void aby3ML::init(u64 partyIdx, std::unique_ptr<aby3::CommPkg> comm_pkg, block seed) {
  comm_pkg_ = std::move(comm_pkg);
  mRt.init(partyIdx, *comm_pkg_);
  LOG(INFO) << "Runtime init finish.";
  osuCrypto::PRNG prng(seed);
  mEnc.init(partyIdx, *comm_pkg_, prng.get<block>());
  LOG(INFO) << "Encryptor init finish.";
  mEval.init(partyIdx, *comm_pkg_, prng.get<block>());
  LOG(INFO) << "Evaluator init finish.";
}

void aby3ML::fini(void) {
  // mPreproPrev.close();
  // mPreproNext.close();
  // mPrev.close();
  // mNext.close();
  comm_pkg_->mNext.close();
  comm_pkg_->mPrev.close();
}
#else
void aby3ML::init(u64 partyIdx, MpcChannel &prev,
                  MpcChannel &next, block seed) {
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
