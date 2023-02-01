
#include "src/primihub/operator/aby3_operator.h"
namespace primihub {
int MPCOperator::setup(std::string next_ip, std::string prev_ip, u32 next_port,
                       u32 prev_port) {
  CommPkg comm = CommPkg();
  Session ep_next_;
  Session ep_prev_;

  switch (partyIdx) {
  case 0:
    ep_next_.start(ios, next_ip, next_port, SessionMode::Server, next_name);
    ep_prev_.start(ios, prev_ip, prev_port, SessionMode::Server, prev_name);

    VLOG(3) << "Start server session, listen port " << next_port << ".";
    VLOG(3) << "Start server session, listen port " << prev_port << ".";

    break;
  case 1:
    ep_next_.start(ios, next_ip, next_port, SessionMode::Server, next_name);
    ep_prev_.start(ios, prev_ip, prev_port, SessionMode::Client, prev_name);

    VLOG(3) << "Start server session, listen port " << next_port << ".";
    VLOG(3) << "Start client session, connect to " << prev_ip << ":"
            << prev_port << ".";

    break;
  default:
    ep_next_.start(ios, next_ip, next_port, SessionMode::Client, next_name);
    ep_prev_.start(ios, prev_ip, prev_port, SessionMode::Client, prev_name);

    VLOG(3) << "Start client session, connect to " << next_ip << ":"
            << next_port << ".";
    VLOG(3) << "Start client session, connect to " << prev_ip << ":"
            << prev_port << ".";

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
  gen.init(toBlock(partyIdx), toBlock((partyIdx + 1) % 3));

  // Copies the Channels and will use them for later protcols.
  mNext = comm.mNext();
  mPrev = comm.mPrev();
  auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
  runtime.init(partyIdx, commPtr);
  return 1;
}

void MPCOperator::fini() {
  mPrev.close();
  mNext.close();
}
void MPCOperator::createShares(const i64Matrix &vals,
                               si64Matrix &sharedMatrix) {
  enc.localIntMatrix(runtime, vals, sharedMatrix).get();
}

void MPCOperator::createShares(int vals, si64 &sharedInt) {
  enc.localInt(runtime, vals, sharedInt).get();
}

void MPCOperator::createShares(si64 &sharedInt) {
  enc.remoteInt(runtime, sharedInt).get();
}

void MPCOperator::createShares(si64Matrix &sharedMatrix) {
  enc.remoteIntMatrix(runtime, sharedMatrix).get();
}
si64Matrix MPCOperator::createSharesByShape(const i64Matrix &val) {
  std::array<u64, 2> size{val.rows(), val.cols()};
  mNext.asyncSendCopy(size);
  mPrev.asyncSendCopy(size);
  si64Matrix dest(size[0], size[1]);
  enc.localIntMatrix(runtime, val, dest).get();
  return dest;
}

si64Matrix MPCOperator::createSharesByShape(u64 pIdx) {
  std::array<u64, 2> size;
  if (pIdx == (partyIdx + 1) % 3)
    mNext.recv(size);
  else if (pIdx == (partyIdx + 2) % 3)
    mPrev.recv(size);
  else
    throw RTE_LOC;

  si64Matrix dest(size[0], size[1]);
  enc.remoteIntMatrix(runtime, dest).get();
  return dest;
}

// only support val is column vector
sbMatrix MPCOperator::createBinSharesByShape(i64Matrix &val, u64 bitCount) {
  std::array<u64, 2> size{val.rows(), bitCount};
  mNext.asyncSendCopy(size);
  mPrev.asyncSendCopy(size);
  sbMatrix dest(size[0], size[1]);
  enc.localBinMatrix(runtime, val, dest).get();
  return dest;
}

sbMatrix MPCOperator::createBinSharesByShape(u64 pIdx) {
  std::array<u64, 2> size;
  if (pIdx == (partyIdx + 1) % 3)
    mNext.recv(size);
  else if (pIdx == (partyIdx + 2) % 3)
    mPrev.recv(size);
  else
    throw RTE_LOC;

  sbMatrix dest(size[0], size[1]);
  enc.remoteBinMatrix(runtime, dest).get();
  return dest;
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

i64Matrix MPCOperator::reveal(const sbMatrix &sh_res) {
  i64Matrix res(sh_res.i64Size(), 1);
  enc.reveal(runtime, sh_res, res).get();
  return res;
}

void MPCOperator::reveal(const sbMatrix &sh_res, uint64_t party_id) {
  enc.reveal(runtime, party_id, sh_res).get();
}

si64Matrix MPCOperator::MPC_Add(std::vector<si64Matrix> sharedInt) {
  si64Matrix sum;
  sum = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); i++) {
    sum = sum + sharedInt[i];
  }
  return sum;
}

si64 MPCOperator::MPC_Add_Const(i64 constInt, si64 &sharedInt) {
  si64 temp = sharedInt;
  if (partyIdx == 0)
    temp[0] = sharedInt[0] + constInt;
  else if (partyIdx == 1)
    temp[1] = sharedInt[1] + constInt;
  return temp;
}

si64Matrix MPCOperator::MPC_Add_Const(i64 constInt,
                                      si64Matrix &sharedIntMatrix) {
  si64Matrix temp = sharedIntMatrix;
  if (partyIdx == 0)
    for (i64 i = 0; i < sharedIntMatrix.rows(); i++)
      for (i64 j = 0; j < sharedIntMatrix.cols(); j++)
        temp[0](i, j) = sharedIntMatrix[0](i, j) + constInt;
  else if (partyIdx == 1)
    for (i64 i = 0; i < sharedIntMatrix.rows(); i++)
      for (i64 j = 0; j < sharedIntMatrix.cols(); j++)
        temp[1](i, j) = sharedIntMatrix[1](i, j) + constInt;
  return temp;
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

si64 MPCOperator::MPC_Sub_Const(i64 constInt, si64 &sharedInt, bool mode) {
  si64 temp = sharedInt;
  if (partyIdx == 0)
    temp[0] = sharedInt[0] - constInt;
  else if (partyIdx == 1)
    temp[1] = sharedInt[1] - constInt;
  if (mode != true) {
    temp[0] = -temp[0];
    temp[1] = -temp[1];
  }
  return temp;
}

si64Matrix MPCOperator::MPC_Sub_Const(i64 constInt, si64Matrix &sharedIntMatrix,
                                      bool mode) {
  si64Matrix temp = sharedIntMatrix;
  if (partyIdx == 0)
    for (i64 i = 0; i < sharedIntMatrix.rows(); i++)
      for (i64 j = 0; j < sharedIntMatrix.cols(); j++)
        temp[0](i, j) = sharedIntMatrix[0](i, j) - constInt;
  else if (partyIdx == 1)
    for (i64 i = 0; i < sharedIntMatrix.rows(); i++)
      for (i64 j = 0; j < sharedIntMatrix.cols(); j++)
        temp[1](i, j) = sharedIntMatrix[1](i, j) - constInt;
  if (mode != true) {
    temp[0] = -temp[0];
    temp[1] = -temp[1];
  }
  return temp;
}

si64Matrix MPCOperator::MPC_Mul(std::vector<si64Matrix> sharedInt) {
  si64Matrix prod;
  prod = sharedInt[0];
  for (u64 i = 1; i < sharedInt.size(); ++i)
    eval.asyncMul(runtime, prod, sharedInt[i], prod).get();
  return prod;
}

si64Matrix MPCOperator::MPC_Dot_Mul(const si64Matrix &A, const si64Matrix &B) {
  if (A.cols() != B.cols() || A.rows() != B.rows())
    throw std::runtime_error(LOCATION);

  si64Matrix ret(A.rows(), A.cols());
  eval.asyncDotMul(runtime, A, B, ret).get();
  return ret;
}

si64 MPCOperator::MPC_Mul_Const(const i64 &constInt, const si64 &sharedInt) {
  si64 ret;
  eval.asyncConstMul(constInt, sharedInt, ret);
  return ret;
}

si64Matrix MPCOperator::MPC_Mul_Const(const i64 &constInt,
                                      const si64Matrix &sharedIntMatrix) {
  si64Matrix ret(sharedIntMatrix.rows(), sharedIntMatrix.cols());
  eval.asyncConstMul(constInt, sharedIntMatrix, ret);
  return ret;
}

void MPCOperator::MPC_Compare(i64Matrix &m, sbMatrix &sh_res) {
  // Get matrix shape of all party.
  std::vector<std::array<uint64_t, 2>> all_party_shape;
  for (uint64_t i = 0; i < 3; i++) {
    std::array<uint64_t, 2> shape;
    if (partyIdx == i) {
      shape[0] = m.rows();
      shape[1] = m.cols();
      mNext.asyncSendCopy(shape);
      mPrev.asyncSendCopy(shape);
    } else {
      if (partyIdx == (i + 1) % 3)
        mPrev.recv(shape);
      else if (partyIdx == (i + 2) % 3)
        mNext.recv(shape);
      else
        throw std::runtime_error("Message recv logic error.");
    }

    all_party_shape.emplace_back(shape);
  }

  {
    LOG(INFO) << "Dump shape of all party's matrix:";
    for (uint64_t i = 0; i < 3; i++)
      LOG(INFO) << "Party " << i << ": (" << all_party_shape[i][0] << ", "
                << all_party_shape[i][1] << ")";
    LOG(INFO) << "Dump finish.";
  }

  // The compare is a binary operator, so only two party will provide value
  // for compare, find out which party don't provide value.
  int skip_index = -1;
  for (uint64_t i = 0; i < 3; i++) {
    if (all_party_shape[i][0] == 0 && all_party_shape[i][1] == 0) {
      if (skip_index != -1) {
        throw std::runtime_error(
            "There are two party that don't provide any value at last, but "
            "compare operator require value from two party.");
      } else {
        skip_index = i;
      }
    }
  }

  if (skip_index == -1)
    throw std::runtime_error(
        "This operator is binary, can only handle value from two party.");

  // Shape of matrix in two party shoubld be the same.
  if (skip_index == 0) {
    all_party_shape[0][0] = all_party_shape[1][0];
    all_party_shape[0][1] = all_party_shape[1][1];
    all_party_shape[1][0] = all_party_shape[2][0];
    all_party_shape[1][1] = all_party_shape[2][1];
  } else if (skip_index != 2) {
    all_party_shape[1][0] = all_party_shape[2][0];
    all_party_shape[1][1] = all_party_shape[2][1];
  }

  if ((all_party_shape[0][0] != all_party_shape[1][0]) ||
      (all_party_shape[0][1] != all_party_shape[1][1]))
    throw std::runtime_error("Shape of matrix in two party must be the same.");

  // Set value to it's negative for some party.
  if (skip_index == 0 || skip_index == 1) {
    if (partyIdx == 2)
      m = m.array() * -1;
  } else {
    if (partyIdx == 1)
      m = m.array() * -1;
  }

  LOG(INFO) << "Party " << (skip_index + 1) % 3 << " and party "
            << (skip_index + 2) % 3 << " provide value for MPC compare.";

  // Create binary shares.
  uint64_t num_elem = all_party_shape[0][0] * all_party_shape[0][1];
  m.resize(num_elem, 1);

  std::vector<sbMatrix> sh_m_vec;
  for (uint64_t i = 0; i < 3; i++) {
    if (static_cast<int>(i) == skip_index)
      continue;
    if (i == partyIdx) {
      sbMatrix sh_m(num_elem, VAL_BITCOUNT);
      auto task = runtime.noDependencies();
      task.get();
      enc.localBinMatrix(task, m, sh_m).get();
      sh_m_vec.emplace_back(sh_m);
    } else {
      sbMatrix sh_m(num_elem, VAL_BITCOUNT);
      auto task = runtime.noDependencies();
      task.get();
      enc.remoteBinMatrix(task, sh_m).get();
      sh_m_vec.emplace_back(sh_m);
    }
  }

  LOG(INFO) << "Create binary share for value from two party finish.";

  // Build then run MSB circuit.
  KoggeStoneLibrary lib;
  uint64_t size = 64;
  BetaCircuit *cir = lib.int_int_add_msb(size);
  cir->levelByAndDepth();

  sh_res.resize(m.size(), 1);
  std::vector<const sbMatrix *> input = {&sh_m_vec[0], &sh_m_vec[1]};
  std::vector<sbMatrix *> output = {&sh_res};
  auto task = runtime.noDependencies();
  task = binEval.asyncEvaluate(task, cir, gen, input, output);
  task.get();

  // Recover original value.
  if (skip_index == 0 || skip_index == 1) {
    if (partyIdx == 2)
      m = m.array() * -1;
  } else {
    if (partyIdx == 1)
      m = m.array() * -1;
  }

  LOG(INFO) << "Finish evalute MSB circuit.";
}

void MPCOperator::MPC_Compare(sbMatrix &sh_res) {
  std::vector<std::array<uint64_t, 2>> all_party_shape;
  // Get matrix shape of all party.
  for (uint64_t i = 0; i < 3; i++) {
    std::array<uint64_t, 2> shape;
    if (partyIdx == i) {
      shape[0] = 0;
      shape[1] = 0;
      mNext.asyncSendCopy(shape);
      mPrev.asyncSendCopy(shape);
    } else {
      if (partyIdx == (i + 1) % 3)
        mPrev.recv(shape);
      else
        mNext.recv(shape);

      all_party_shape.emplace_back(shape);
    }
  }

  if (VLOG_IS_ON(2)) {
    VLOG(2) << "Dump shape of all party's matrix:";
    for (uint64_t i = 0; i < 3; i++)
      VLOG(2) << "Party 0: (" << all_party_shape[i][0] << ", "
              << all_party_shape[i][1] << ")";
    VLOG(2) << "Dump finish.";
  }

  // Shape of matrix in two party shoubld be the same.
  if ((all_party_shape[0][0] != all_party_shape[1][0]) ||
      (all_party_shape[0][1] != all_party_shape[1][1]))
    throw std::runtime_error("Shape of matrix in two party must be the same.");

  LOG(INFO) << "Party " << (partyIdx + 1) % 3 << " and party "
            << (partyIdx + 2) % 3 << " provide value for MPC compare.";

  // Create binary share.
  uint32_t num_elem = all_party_shape[0][0] * all_party_shape[0][1];
  std::vector<sbMatrix> sh_m_vec;
  for (uint8_t i = 0; i < 2; i++) {
    sbMatrix sh_m(num_elem, VAL_BITCOUNT);
    enc.remoteBinMatrix(runtime.noDependencies(), sh_m).get();
    sh_m_vec.emplace_back(sh_m);
  }

  LOG(INFO) << "Create binary share for value from two party finish.";

  // Build then run MSB circuit.
  KoggeStoneLibrary lib;
  uint64_t size = 64;
  BetaCircuit *cir = lib.int_int_add_msb(size);
  cir->levelByAndDepth();
  auto task = runtime.noDependencies();
  task.get();

  sh_res.resize(sh_m_vec[0].i64Size(), 1);
  std::vector<const sbMatrix *> input = {&sh_m_vec[0], &sh_m_vec[1]};
  std::vector<sbMatrix *> output = {&sh_res};
  task = binEval.asyncEvaluate(task, cir, gen, input, output);
  task.get();

  LOG(INFO) << "Finish evaluate MSB circuit.";
}

} // namespace primihub
