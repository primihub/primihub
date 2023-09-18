// "Copyright [2021] <PrimiHub>"

#include "src/primihub/algorithm/aby3ML.h"
#include <glog/logging.h>
#include <memory>
#include <utility>

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
void aby3ML::init(u64 partyIdx, Session& prev,
                  Session& next, block seed) {
  auto comm_pkg_ = std::make_unique<aby3::CommPkg>();
  comm_pkg_->mNext = next.addChannel();
  comm_pkg_->mPrev = prev.addChannel();
  mRt.init(partyIdx, *comm_pkg_);

  osuCrypto::PRNG prng(seed);
  mEnc.init(partyIdx, *comm_pkg_, prng.get<block>());
  mEval.init(partyIdx, *comm_pkg_, prng.get<block>());
}
#endif  // MPC_SOCKET_CHANNEL

void aby3ML::init(u64 partyIdx,
                  std::unique_ptr<aby3::CommPkg> comm_pkg,
                  block seed) {
  auto comm_pkg_ = std::move(comm_pkg);
  mRt.init(partyIdx, *comm_pkg_);
  LOG(INFO) << "Runtime init finish.";
  osuCrypto::PRNG prng(seed);
  mEnc.init(partyIdx, *comm_pkg_, prng.get<block>());
  LOG(INFO) << "Encryptor init finish.";
  mEval.init(partyIdx, *comm_pkg_, prng.get<block>());
  LOG(INFO) << "Evaluator init finish.";
}

void aby3ML::init(u64 partyIdx, aby3::CommPkg* comm_pkg, block seed) {
  this->comm_pkg_ref_ = comm_pkg;
  mRt.init(partyIdx, *comm_pkg_ref_);
  LOG(INFO) << "Runtime init finish.";
  osuCrypto::PRNG prng(seed);
  mEnc.init(partyIdx, *comm_pkg_ref_, prng.get<block>());
  LOG(INFO) << "Encryptor init finish.";
  mEval.init(partyIdx, *comm_pkg_ref_, prng.get<block>());
  LOG(INFO) << "Evaluator init finish.";
}
void aby3ML::fini(void) {
  // this->mNext().close();
  // this->mPrev().close();
}

}  // namespace primihub
