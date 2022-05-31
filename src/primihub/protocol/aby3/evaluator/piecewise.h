
#ifndef SRC_primihub_PROTOCOL_ABY3_EVALUATOR_PIECEWISE_H_
#define SRC_primihub_PROTOCOL_ABY3_EVALUATOR_PIECEWISE_H_

#include <vector>
#include <cstring>

#include "src/primihub/common/type/matrix.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/binary_evaluator.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/primitive/circuit/circuit_library.h"
#include "src/primihub/util/log.h"

namespace primihub {

class Sh3Piecewise {
 public:
  struct Coef {
    Coef() = default;
    Coef(const int& i) { *this = i64(i); }
    Coef(const i64& i) { *this = i; }
    Coef(const double& d) { *this = d; }

    bool mIsInteger = false;
    i64 mInt = 0;
    double mDouble = 0;

    void operator=(const i64& i) {
      mIsInteger = true;
      mInt = i;
    }

    void operator=(const int& i) {
      mIsInteger = true;
      mInt = i;
    }

    void operator=(const double& d) {
      mIsInteger = false;
      mDouble = d;
    }

    double getDouble(const u64& decPlace) {
      if (mIsInteger)
        return static_cast<double>(mInt);
      else
        return mDouble;
    }

    i64 getFixedPoint(const u64& decPlace) {
      if (mIsInteger)
        return mInt * (1ull << decPlace);
      else
        return static_cast<i64>(mDouble * (1ull << decPlace));
    }

    i64 getInteger() {
      if (mIsInteger == false)
        throw std::runtime_error(LOCATION);

      return mInt;
    }
  };

  std::vector<Coef> mThresholds;
  std::vector<std::vector<Coef>> mCoefficients;

  void eval(const eMatrix<double>& inputs,
    eMatrix<double>& outputs, u64 D, bool print = false);

  template<Decimal D>
  void eval(const f64Matrix<D>& inputs, f64Matrix<D>& outputs,
    bool print = false) {
    eval(inputs.i64Cast(), outputs.i64Cast(), D, print);
  }

  void eval(
    const i64Matrix& inputs,
    i64Matrix& outputs,
    u64 D,
    bool print = false);

  Sh3Task eval(
    Sh3Task dep,
    const si64Matrix& input,
    si64Matrix& output,
    u64 D,
    Sh3Evaluator& evaluator,
    bool print = false);


  template<Decimal D>
  Sh3Task eval(
    Sh3Task dep,
    const sf64Matrix<D>& inputs,
    sf64Matrix<D>& outputs,
    Sh3Evaluator& evaluator,
    bool print = false) {
    print = true;
    return eval(dep, inputs.i64Cast(), outputs.i64Cast(), D, evaluator, print);
  }

  Sh3Task eval(
    Sh3Task dep,
    const si64Matrix& inputs,
    si64Matrix& outputs,
    Sh3Evaluator& evaluator,
    bool print = true) {
    return eval(dep, inputs, outputs, 16, evaluator, print);
  }

  std::vector<sbMatrix> mInputRegions;
  std::vector<sbMatrix> circuitInput0;
  sbMatrix circuitInput1;
  Sh3BinaryEvaluator binEng;
  CircuitLibrary lib;
  std::vector<si64Matrix> functionOutputs;

  Sh3Encryptor DebugEnc;
  Sh3Runtime DebugRt;

  Sh3Task getInputRegions(
    const si64Matrix& inputThresholds, u64 decimal,
    CommPkg& comm, Sh3Task& task,
    Sh3ShareGen& gen,
    bool print = false);

  Matrix<u8> getInputRegions(const i64Matrix& inputs, u64);

  Sh3Task getFunctionValues(
    const si64Matrix& inputs,
    CommPkg& comm,
    Sh3Task self,
    u64 decimal,
    span<si64Matrix> outputs);
};

}  // namespace primihub

#endif  // SRC_primihub_EVALUATOR_PIECEWISE_H_
