
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
    break;
  case 1:
    ep_next_.start(ios, ip, next_port, SessionMode::Server, next_name);
    ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);
    break;
  default:
    ep_next_.start(ios, ip, next_port, SessionMode::Client, next_name);
    ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);
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

  // Copies the Channels and will use them for later protcols.
  auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
  runtime.init(partyIdx, commPtr);
  return 1;
}
template <Decimal D>
sf64Matrix<D> MPCOperator::createShares(const eMatrix<double> &vals,
                                        sf64Matrix<D> &sharedMatrix) {
  f64Matrix<D> fixedMatrix(vals.rows(), vals.cols());
  for (u64 i = 0; i < vals.size(); ++i)
    fixedMatrix(i) = vals(i);
  enc.localFixedMatrix(runtime, fixedMatrix, sharedMatrix).get();
  return sharedMatrix;
}

template <Decimal D>
sf64Matrix<D> MPCOperator::createShares(sf64Matrix<D> &sharedMatrix) {
  enc.remoteFixedMatrix(runtime, sharedMatrix).get();
  return sharedMatrix;
}

si64Matrix MPCOperator::createShares(const i64Matrix &vals,
                                     si64Matrix &sharedMatrix) {
  enc.localIntMatrix(runtime, vals, sharedMatrix).get();
  return sharedMatrix;
}

si64Matrix MPCOperator::createShares(si64Matrix &sharedMatrix) {
  enc.remoteIntMatrix(runtime, sharedMatrix).get();
  return sharedMatrix;
}

template <Decimal D>
sf64<D> MPCOperator::createShares(double vals, sf64<D> &sharedFixedInt) {
  enc.localFixed<D>(runtime, vals, sharedFixedInt).get();
  return sharedFixedInt;
}

template <Decimal D>
sf64<D> MPCOperator::createShares(sf64<D> &sharedFixedInt) {
  enc.remoteFixed<D>(runtime, sharedFixedInt).get();
  return sharedFixedInt;
}

template <Decimal D>
eMatrix<double> MPCOperator::revealAll(const sf64Matrix<D> &vals) {
  f64Matrix<D> temp(vals.rows(), vals.cols());
  enc.revealAll(runtime, vals, temp).get();

  eMatrix<double> ret(vals.rows(), vals.cols());
  for (u64 i = 0; i < ret.size(); ++i)
    ret(i) = static_cast<double>(temp(i));
  return ret;
}

i64Matrix MPCOperator::revealAll(const si64Matrix &vals) {
  i64Matrix ret(vals.rows(), vals.cols());
  enc.revealAll(runtime, vals, ret).get();
  return ret;
}

template <Decimal D> double MPCOperator::revealAll(const sf64<D> &vals) {
  f64<D> ret;
  enc.revealAll(runtime, vals, ret).get();
  return static_cast<double>(ret);
}

template <Decimal D>
eMatrix<double> MPCOperator::reveal(const sf64Matrix<D> &vals) {
  f64Matrix<D> temp(vals.rows(), vals.cols());
  enc.reveal(runtime, vals, temp).get();
  eMatrix<double> ret(vals.rows(), vals.cols());
  for (u64 i = 0; i < ret.size(); ++i)
    ret(i) = static_cast<double>(temp(i));
  return ret;
}

template <Decimal D>
void MPCOperator::reveal(const sf64Matrix<D> &vals, u64 Idx) {
  enc.reveal(runtime, Idx, vals).get();
}

i64Matrix MPCOperator::reveal(const si64Matrix &vals) {
  i64Matrix ret(vals.rows(), vals.cols());
  enc.reveal(runtime, vals, ret).get();
  return ret;
}

void MPCOperator::reveal(const si64Matrix &vals, u64 Idx) {
  enc.reveal(runtime, Idx, vals).get();
}

template <Decimal D> double MPCOperator::reveal(const sf64<D> &vals, u64 Idx) {
  if (Idx == partyIdx) {
    f64<D> ret;
    enc.reveal(runtime, vals, ret).get();
    return static_cast<double>(ret);
  } else {
    enc.reveal(runtime, Idx, vals).get();
  }
}

template <Decimal D>
sf64<D> MPCOperator::MPC_Add(std::vector<sf64<D>> sharedFixedInt,
                             sf64<D> &sum) {
  sum = sharedFixedInt[0];
  for (u64 i = 1; i < sharedFixedInt.size(); i++) {
    sum = sum + sharedFixedInt[i];
  }
  return sum;
}

template <Decimal D>
sf64Matrix<D> MPCOperator::MPC_Add(std::vector<sf64Matrix<D>> sharedFixedInt,
                                   sf64Matrix<D> &sum) {
  sum = sharedFixedInt[0];
  for (u64 i = 1; i < sharedFixedInt.size(); i++) {
    sum = sum + sharedFixedInt[i];
  }
  return sum;
}

si64Matrix MPCOperator::MPC_Add(std::vector<si64Matrix> sharedInt,
                                si64Matrix &sum) {
  sum = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); i++) {
    sum = sum + sharedInt[i];
  }
  return sum;
}

template <Decimal D>
sf64<D> MPCOperator::MPC_Sub(sf64<D> minuend, std::vector<sf64<D>> subtrahends,
                             sf64<D> &difference) {
  difference = minuend;
  for (u64 i = 0; i < subtrahends.size(); i++) {
    difference = difference - subtrahends[i];
  }
  return difference;
}

template <Decimal D>
sf64Matrix<D> MPCOperator::MPC_Sub(sf64Matrix<D> minuend,
                                   std::vector<sf64Matrix<D>> subtrahends,
                                   sf64Matrix<D> &difference) {
  difference = minuend;
  for (u64 i = 0; i < subtrahends.size(); i++) {
    difference = difference - subtrahends[i];
  }
  return difference;
}

si64Matrix MPCOperator::MPC_Sub(si64Matrix minuend,
                                std::vector<si64Matrix> subtrahends,
                                si64Matrix &difference) {
  difference = minuend;
  for (u64 i = 0; i < subtrahends.size(); i++) {
    difference = difference - subtrahends[i];
  }
  return difference;
}

template <Decimal D>
sf64<D> MPCOperator::MPC_Mul(std::vector<sf64<D>> sharedFixedInt,
                             sf64<D> &prod) {
  prod = sharedFixedInt[0];
  for (u64 i = 1; i < sharedFixedInt.size(); ++i)
    eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
  return prod;
}

template <Decimal D>
sf64Matrix<D> MPCOperator::MPC_Mul(std::vector<sf64Matrix<D>> sharedFixedInt,
                                   sf64Matrix<D> &prod) {
  prod = sharedFixedInt[0];
  for (u64 i = 1; i < sharedFixedInt.size(); ++i)
    eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
  return prod;
}

si64Matrix MPCOperator::MPC_Mul(std::vector<si64Matrix> sharedInt,
                                si64Matrix &prod) {
  prod = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); ++i)
    eval.asyncMul(runtime, prod, sharedInt[i], prod).get();
  return prod;
}
} // namespace primihub
