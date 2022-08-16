
#ifndef SRC_primihub_operator_ABY3_operator_H
#define SRC_primihub_operator_ABY3_operator_H

#include <algorithm>
#include <random>
#include <vector>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/piecewise.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/session.h"

#include "src/primihub/common/type/type.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"

namespace primihub {

const Decimal D = D16;

class MPCOperator {
public:
  IOService ios;
  Sh3Encryptor enc;
  Sh3Evaluator eval;
  Sh3Runtime runtime;
  u64 partyIdx;
  string next_name;
  string prev_name;

  MPCOperator(u64 partyIdx_, string NextName, string PrevName)
      : partyIdx(partyIdx_), next_name(NextName), prev_name(PrevName) {}

  int setup(std::string ip, u32 next_port, u32 prev_port);

  template <Decimal D>
  sf64Matrix<D> createShares(const eMatrix<double> &vals,
                             sf64Matrix<D> &sharedMatrix);

  template <Decimal D> sf64Matrix<D> createShares(sf64Matrix<D> &sharedMatrix);

  si64Matrix createShares(const i64Matrix &vals, si64Matrix &sharedMatrix);

  si64Matrix createShares(si64Matrix &sharedMatrix);

  template <Decimal D>
  sf64<D> createShares(double vals, sf64<D> &sharedFixedInt);

  template <Decimal D> sf64<D> createShares(sf64<D> &sharedFixedInt);

  template <Decimal D> eMatrix<double> revealAll(const sf64Matrix<D> &vals);

  i64Matrix revealAll(const si64Matrix &vals);

  template <Decimal D> double revealAll(const sf64<D> &vals);

  template <Decimal D> eMatrix<double> reveal(const sf64Matrix<D> &vals);

  template <Decimal D> void reveal(const sf64Matrix<D> &vals, u64 Idx);

  i64Matrix reveal(const si64Matrix &vals);

  void reveal(const si64Matrix &vals, u64 Idx);

  template <Decimal D> double reveal(const sf64<D> &vals, u64 Idx);

  template <Decimal D>
  sf64<D> MPC_Add(std::vector<sf64<D>> sharedFixedInt, sf64<D> &sum);

  template <Decimal D>
  sf64Matrix<D> MPC_Add(std::vector<sf64Matrix<D>> sharedFixedInt,
                        sf64Matrix<D> &sum);

  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum);

  template <Decimal D>
  sf64<D> MPC_Sub(sf64<D> minuend, std::vector<sf64<D>> subtrahends,
                  sf64<D> &difference);

  template <Decimal D>
  sf64Matrix<D> MPC_Sub(sf64Matrix<D> minuend,
                        std::vector<sf64Matrix<D>> subtrahends,
                        sf64Matrix<D> &difference);

  si64Matrix MPC_Sub(si64Matrix minuend, std::vector<si64Matrix> subtrahends,
                     si64Matrix &difference);

  template <Decimal D>
  sf64<D> MPC_Mul(std::vector<sf64<D>> sharedFixedInt, sf64<D> &prod);

  template <Decimal D>
  sf64Matrix<D> MPC_Mul(std::vector<sf64Matrix<D>> sharedFixedInt,
                        sf64Matrix<D> &prod);

  si64Matrix MPC_Mul(std::vector<si64Matrix> sharedInt, si64Matrix &prod);
};

#endif 
} // namespace primihub
