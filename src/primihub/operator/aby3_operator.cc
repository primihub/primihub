
#include "src/primihub/operator/aby3_operator.h"

namespace primihub {
int MPCOperator::setup(std::string ip, u32 next_port, u32 prev_port) {
  CommPkg comm = CommPkg();
  Session ep_next_;
  Session ep_prev_;

  switch (partyIdx) {
  case 0:
    ep_next_.start(ios, ip, next_port, SessionMode::Server, next_name);
    ep_prev_.start(ios, ip, prev_port, SessionMode::Server, prev_name);

    VLOG(3) << "Start server session, listen port " << next_port << ".";
    VLOG(3) << "Start server session, listen port " << prev_port << ".";

    break;
  case 1:
    ep_next_.start(ios, ip, next_port, SessionMode::Server, next_name);
    ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);

    VLOG(3) << "Start server session, listen port " << next_port << ".";
    VLOG(3) << "Start client session, connect to " << ip << ":" << prev_port
              << ".";

    break;
  default:
    ep_next_.start(ios, ip, next_port, SessionMode::Client, next_name);
    ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);

    VLOG(3) << "Start client session, connect to " << ip << ":" << next_port
              << ".";
    VLOG(3) << "Start client session, connect to " << ip << ":" << prev_port
              << ".";

    break;
  }

  comm.setNext(ep_next_.addChannel());
  comm.setPrev(ep_prev_.addChannel());
  comm.mNext().waitForConnection();
  comm.mPrev().waitForConnection();
  comm.mNext().send(partyIdx);
  comm.mPrev().send(partyIdx);

  u64 prev_party = 0;
  u64 next_party = 0;
  comm.mNext().recv(next_party);
  comm.mPrev().recv(prev_party);
  if (next_party != (partyIdx + 1) % 3) {
    LOG(ERROR) << "Party " << partyIdx << ", expect next party id "
               << (partyIdx + 1) % 3 << ", but give " << next_party << ".";
    return -3;
  }

  if (prev_party != (partyIdx + 2) % 3) {
    LOG(ERROR) << "Party " << partyIdx << ", expect prev party id "
               << (partyIdx + 2) % 3 << ", but give " << prev_party << ".";
    return -3;
  }

  // Establishes some shared randomness needed for the later protocols
  enc.init(partyIdx, comm, sysRandomSeed());

  // Establishes some shared randomness needed for the later protocols
  eval.init(partyIdx, comm, sysRandomSeed());

  binEval.mPrng.SetSeed(toBlock(partyIdx));
  // Copies the Channels and will use them for later protcols.
  auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
  runtime.init(partyIdx, commPtr);
  return 1;
}

void MPCOperator::createShares(const i64Matrix &vals,
                               si64Matrix &sharedMatrix) {
  enc.localIntMatrix(runtime, vals, sharedMatrix).get();
}

void MPCOperator::createShares(si64Matrix &sharedMatrix) {
  enc.remoteIntMatrix(runtime, sharedMatrix).get();
}

void MPCOperator::createBinShares(i64Matrix &vals, sbMatrix &ret) {
  enc.localBinMatrix(runtime.noDependencies(), vals, ret).get();
}
void MPCOperator::createBinShares(sbMatrix &ret) {
  enc.remoteBinMatrix(runtime.noDependencies(), ret).get();
}

i64Matrix MPCOperator::revealAll(const si64Matrix &vals) {
  i64Matrix ret(vals.rows(), vals.cols());
  enc.revealAll(runtime, vals, ret).get();
  return ret;
}

i64Matrix MPCOperator::reveal(const si64Matrix &vals) {
  i64Matrix ret(vals.rows(), vals.cols());
  enc.reveal(runtime, vals, ret).get();
  return ret;
}

void MPCOperator::reveal(const si64Matrix &vals, u64 Idx) {
  enc.reveal(runtime, Idx, vals).get();
}

si64Matrix MPCOperator::MPC_Add(std::vector<si64Matrix> sharedInt) {
  si64Matrix sum;
  sum = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); i++) {
    sum = sum + sharedInt[i];
  }
  return sum;
}

si64Matrix MPCOperator::MPC_Sub(si64Matrix minuend,
                                std::vector<si64Matrix> subtrahends) {
  si64Matrix difference;
  difference = minuend;
  for (u64 i = 0; i < subtrahends.size(); i++) {
    difference = difference - subtrahends[i];
  }
  return difference;
}

si64Matrix MPCOperator::MPC_Mul(std::vector<si64Matrix> sharedInt) {
  si64Matrix prod;
  prod = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); ++i)
    eval.asyncMul(runtime, prod, sharedInt[i], prod).get();
  return prod;
}
} // namespace primihub
