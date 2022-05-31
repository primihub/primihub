
#include "src/primihub/protocol/aby3/evaluator/piecewise.h"

namespace primihub {
  i64 mul(i64 op1, i64 op2, i64 dec) {
    uint64_t u1 = (op1 & 0xffffffff);
    uint64_t v1 = (op2 & 0xffffffff);
    uint64_t t = (u1 * v1);
    uint64_t w3 = (t & 0xffffffff);
    uint64_t k = (t >> 32);

    op1 >>= 32;
    t = (op1 * v1) + k;
    k = (t & 0xffffffff);
    uint64_t w1 = (t >> 32);

    op2 >>= 32;
    t = (u1 * op2) + k;
    k = (t >> 32);

    auto hi = (op1 * op2) + w1 + k;
    auto lo = (t << 32) + w3;

    return  (lo >> dec) + (hi << (64 - dec));
  }

  void Sh3Piecewise::eval(
    const eMatrix<double> & inputs,
    eMatrix<double> & outputs,
    u64 D,
    bool print) {
    i64Matrix in, out;
    in.resizeLike(inputs);
    out.resizeLike(inputs);
    for (u64 i = 0; i < in.size(); ++i)
      in(i) = static_cast<i64>(inputs(i) * (1ull << D));

    eval(in, out, D, print);

    for (u64 i = 0; i < in.size(); ++i)
      outputs(i) = out(i) / double(1ull << D);
  }

  Matrix<u8> Sh3Piecewise::getInputRegions(
    const i64Matrix & inputs,
    u64 decimal) {
    Matrix<u8> inputRegions(inputs.rows(), mThresholds.size() + 1);
    std::vector<u8> thresholds(mThresholds.size());
    for (u64 i = 0; i < inputs.rows(); ++i) {
      auto& in = inputs(i);

      for (u64 t = 0; t < mThresholds.size(); ++t) {
        thresholds[t] = in < mThresholds[t].getFixedPoint(decimal) ? 1 : 0;
      }

      inputRegions(i, 0) = thresholds[0];
      for (u64 t = 1; t < mThresholds.size(); ++t) {
        inputRegions(i, t) = (1 ^ thresholds[t - 1])
          * thresholds[t];
      }

      inputRegions(i, mThresholds.size()) = (1 ^
        thresholds.back());
    }

    return inputRegions;
  }

  void Sh3Piecewise::eval(
    const i64Matrix & inputs,
    i64Matrix & outputs,
    u64 decimal,
    bool print) {
    // print = true;
    if (inputs.cols() != 1 || outputs.cols() != 1)
      throw std::runtime_error(LOCATION);

    if (outputs.size() != inputs.size())
      throw std::runtime_error(LOCATION);

    if (mThresholds.size() == 0)
      throw std::runtime_error(LOCATION);

    if (mCoefficients.size() != mThresholds.size() + 1)
      throw std::runtime_error(LOCATION);

    if (print) {
      std::cout << "thresholds: ";
      for (size_t i = 0; i < mThresholds.size(); i++) {
        std::cout << mThresholds[i].getFixedPoint(decimal) << "  ";
      }
      std::cout << std::endl;
    }

    Matrix<u8> inputRegions = getInputRegions(inputs, decimal);

    if (print) {
      for (u64 i = 0; i < inputs.rows(); ++i) {
        std::cout << i << ":  ";
        for (u64 t = 0; t < mCoefficients.size(); ++t) {
          std::cout << u32(inputRegions(i, t)) << ", ";
        }

        std::cout << "  ~~  " << inputs(i) << " ("
          << (inputs(i) / double(1ull << decimal))
          << ")  ~~  ";
        for (u64 t = 0; t < mThresholds.size(); ++t) {
          std::cout << mThresholds[t].getFixedPoint(decimal)
            << ", ";
        }

        std::cout << std::endl;
      }
    }

    for (u64 i = 0; i < inputs.rows(); ++i) {
      auto in = inputs(i);
      auto& out = outputs(i);
      out = 0;

      if (print)
        std::cout << i << ":  " << in << "\n";

      for (u64 t = 0; t < mCoefficients.size(); ++t) {
        i64 ft = 0;
        i64 inPower = (1ll << decimal);
        if (print)
          std::cout << "   " << int(inputRegions(i, t)) << " * (";

        for (u64 c = 0; c < mCoefficients[t].size(); ++c) {
          auto coef = mCoefficients[t][c].getFixedPoint(decimal);

          if (print)
            std::cout << coef << " * " << inPower << " + ";

          ft += mul(coef, inPower, decimal);

          inPower = mul(in, inPower, decimal);
        }

        if (print) {
          std::cout << ")" << std::endl;
        }

        out += inputRegions(i, t) * ft;
      }
    }
  }
#define UPDATE

  Sh3Task Sh3Piecewise::eval(
    Sh3Task dep,
    const si64Matrix & inputs,
    si64Matrix & outputs,
    u64 D,
    Sh3Evaluator& evaluator,
    bool print) {
    UPDATE;
      if (inputs.cols() != 1 || outputs.cols() != 1)
        throw std::runtime_error(LOCATION);

      if (outputs.size() != inputs.size())
        throw std::runtime_error(LOCATION);

      if (mThresholds.size() == 0)
        throw std::runtime_error(LOCATION);

      if (mCoefficients.size() != mThresholds.size() + 1)
        throw std::runtime_error(LOCATION);

      mInputRegions.resize(mCoefficients.size());
      UPDATE;
      auto rangeTestTask = getInputRegions(inputs, D,
        dynamic_cast<CommPkg&>(*dep.getRuntime().mCommPtr), dep, evaluator.mShareGen, print);

// #define Sh3Piecewise_DEBUG
#ifdef Sh3Piecewise_DEBUG
          i64Matrix plain_inputs(inputs.rows(), inputs.cols());
          DebugEnc.revealAll(DebugRt.noDependencies(), inputs,
            plain_inputs).get();

          i64Matrix plain_outputs(outputs.rows(), outputs.cols());
          eval(plain_inputs, plain_outputs, D);

          auto true_inputRegions = getInputRegions(plain_inputs, D);

          std::vector<i64Matrix> inputRegions__(
            mCoefficients.size());
          for (u64 i = 0; i < mCoefficients.size(); ++i) {
            inputRegions__[i].resize(mInputRegions[i].rows(), 1);
            DebugEnc.revealAll(DebugRt.noDependencies(),
              mInputRegions[i], inputRegions__[i]).get();
          }

          for (u64 i = 0; i < inputs.size(); ++i) {
            ostreamLock oo(std::cout);

            if (print) std::cout << i << ":  ";

            for (u64 t = 0; t < mInputRegions.size(); ++t) {
              if (print) std::cout << inputRegions__[t](i) << ", ";


              if (true_inputRegions(i, t) != inputRegions__[t](i)) {
                std::cout << "bad input region " << i << " " << t << std::endl;
              }
            }
            if (print) std::cout << std::endl;
          }
    UPDATE;
#endif
    UPDATE;


    UPDATE;

    UPDATE;

      functionOutputs.resize(mCoefficients.size());
      UPDATE;
      auto fxEvalTask = getFunctionValues(inputs,
        dynamic_cast<CommPkg&>(*dep.getRuntime().mCommPtr), dep, D, functionOutputs);
      auto combineTask = (fxEvalTask);

      outputs.mShares[0].setZero();
      outputs.mShares[1].setZero();

      auto multTask = combineTask;

      for (u64 c = 0; c < mCoefficients.size(); ++c) {
        if (mCoefficients[c].size()) {
          if (mCoefficients[c].size() > 1) {
            evaluator.asyncMul(
              combineTask,
              functionOutputs[c],
              mInputRegions[c],
              functionOutputs[c]).then([&outputs, this, c](Sh3Task self) {
                  outputs = outputs + functionOutputs[c];
                }
            ).get();
          } else {
            // multiplication by a public constant
            functionOutputs[c].resize(inputs.rows(), inputs.cols());

            evaluator.asyncMul(
              combineTask,
              mCoefficients[c][0].getFixedPoint(D),
              mInputRegions[c],
              functionOutputs[c]).then([&outputs, this, c](Sh3Task self) {
                  outputs = outputs + functionOutputs[c];
                }).get();
          }

#ifdef Sh3Piecewise_DEBUG
          if (print) {
            i64Matrix plain_inputs(inputs.rows(), inputs.cols());
            DebugEnc.revealAll(DebugRt.noDependencies(),
              inputs, plain_inputs).get();

            std::cout << "coef[" << c << "] = ";
            if (mCoefficients[c].size() > 1)
              std::cout << mCoefficients[c][1].getInteger()
                << " " << plain_inputs(0) << " + ";

            if (mCoefficients[c].size() > 0)
              std::cout << mCoefficients[c][0].getFixedPoint(D) << std::endl;
          }
#endif
        }
      }
    UPDATE;
      return dep;
  }


  Sh3Task Sh3Piecewise::getInputRegions(
    const si64Matrix & inputs, u64 decimal,
    CommPkg & comm, Sh3Task & self,
    Sh3ShareGen& gen,
    bool print) {

    // First we want to transform the input into a more efficient prepresentation. 
    // Currently we have x0,x1,x2 which sum to the input. We are going to add
    // x0 and x1 together so that we have a 2-out-of-2 sharing of the input. Party 0 
    // who holds both of these shares will do the addition. After this, we are 
    // going to input these two shares into a circuit to compute the threshold bits

    // This is a bit stange but for each theshold computation, input0 will
    // be different but input1 will be the same. This is a small optimization.
    circuitInput0.resize(mThresholds.size());
    circuitInput1.resize(inputs.size(), 64);
    for (auto& c : circuitInput0) c.resize(inputs.size(), 64);


    // Lets us construct two sharings (x0+x1) and x2. The former will be a normal sharing
    // and input by party 0. This will be stored in the first matrix of circuitInput0.
    // The second shring will be special in that the sharing [x2]=(0,x2,0). In general this
    // can be insecure. However, in this case its ok.

    auto pIdx = self.getRuntime().mPartyIdx;
    auto & c0s0 = circuitInput0[0].mShares[0];
    auto & c0s1 = circuitInput0[0].mShares[1];
    // std::cout << "before switch (pIdx)" << std::endl;
    switch (pIdx) {
      case 0: {
        // have party 0 add the two shares together.
        c0s0.resize(inputs.rows(), inputs.cols());

        // std::cout << "pIdx:" << pIdx << ", before for (u64 i = 0; i < c0s0.size(); ++i)" << std::endl;
        for (u64 i = 0; i < c0s0.size(); ++i)
          c0s0(i) = inputs.mShares[0](i)
          + inputs.mShares[1](i);

        TODO("Radomize the shares....");
        // We need to randomize the share
        //for (u64 i = 0; i < circuitInput0[0].size(); ++i)
        //	c0s0(i) += eng.getBinaryShare();

        circuitInput1.mShares[0].setZero();
        circuitInput1.mShares[1].setZero();
        break;
      }
      case 1:
        circuitInput1.mShares[0].resize(inputs.rows(), inputs.cols());
        circuitInput1.mShares[1].setZero();
        memcpy(circuitInput1.mShares[0].data(), inputs.mShares[0].data(), inputs.size() * sizeof(i64));
        //= std::move(inputs.mShares[0]);

        // std::cout << "pIdx:" << pIdx << ", before c0s0.setZero();" << std::endl;
        TODO("Radomize the shares....");
        c0s0.setZero();
        //for (u64 i = 0; i < inputs.size(); ++i)
        //	c0s0(i) += eng.getBinaryShare();
        break;
      default:// case 2:
        circuitInput1.mShares[0].setZero();
        circuitInput1.mShares[1].resize(inputs.rows(), inputs.cols());// = std::move(inputs.mShares[1]);
        memcpy(circuitInput1.mShares[1].data(), inputs.mShares[1].data(), inputs.size() * sizeof(i64));

        // std::cout << "pIdx:" << pIdx << ", before c0s0.setZero();" << std::endl;
        TODO("Radomize the shares....");
        c0s0.setZero();
        //for (u64 i = 0; i < inputs.size(); ++i)
        //	c0s0(i) += eng.getBinaryShare();
    }

    comm.mNext().asyncSend(c0s0.data(), c0s0.size());
    auto fu = comm.mPrev().asyncRecv(c0s1.data(), c0s1.size());

    UPDATE;
    //auto ret = self.then([&inputs, pIdx, this, decimal, fu = std::move(fu)](CommPkg & comm, Sh3Task self) mutable {

      fu.get();
      // Now we need to augment the circuitInput0 shares for each of the thresholds.
      // Currently we have the two shares (x0+x1), x2 and we want the pairs
      //   (x0+x1)-t0, x2
      //    ...
      //   (x0+x1)-tn, x2
      // We will then add all of these pairs together to get a binary sharing of
      //   x - t0
      //    ...
      //   x - tn
      // Since we are only interested in the sign of these differences, we only 
      // compute the MSB of the difference. Also, notice that x2 is shared between
      // all of the shares. As such we input have one circuitInput1=x2 while we have 
      // n circuitInputs0.

      for (u64 t = 1; t < mThresholds.size(); ++t)
        circuitInput0[t] = circuitInput0[0];

      for (u64 t = 0; t < mThresholds.size(); ++t) {
        if (pIdx < 2) {
          auto threshold = mThresholds[t].getFixedPoint(decimal);

          auto& v = circuitInput0[t].mShares[pIdx];
          for (auto& vv : v)
            vv -= threshold;
        }
      }

      // std::cout << "before lib.int_Sh3Piecewise_helper, mThresholds.size():" << mThresholds.size() << ", sizeof(i64) * 8:" << sizeof(i64) * 8 << std::endl;
      auto cir = lib.int_Sh3Piecewise_helper(sizeof(i64) * 8,
        mThresholds.size());
      // std::cout << "after lib.int_Sh3Piecewise_helper" << std::endl;

      binEng.setCir(cir, inputs.size(), gen);
      binEng.setInput(mThresholds.size(), circuitInput1);

      // std::cout << "before binEng.setInput" << std::endl;
      // set the inputs for all of the circuits
      for (u64 t = 0; t < mThresholds.size(); ++t)
        binEng.setInput(t, circuitInput0[t]);

      // std::cout << "before binEng.asyncEvaluate" << std::endl;
      binEng.asyncEvaluate(self).then([&](Sh3Task self) {
        for (u64 t = 0; t < mInputRegions.size(); ++t) {
          mInputRegions[t].resize(inputs.rows(), inputs.cols());
          binEng.getOutput(t, mInputRegions[t]);
        }
      } , "binEval-continuation").get();
      UPDATE;

    // std::cout << "after binEng.asyncEvaluate" << std::endl;
    //}, "getInputRegions-part2");
    //auto closure = ret.getClosure("getInputRegions-closure");
    //return closure;

    return self.getRuntime();
  }

  Sh3Task Sh3Piecewise::getFunctionValues(
    const si64Matrix & inputs,
    CommPkg & comm,
    Sh3Task self,
    u64 decimal,
    span<si64Matrix> outputs) {
    // degree:
    //  -1:  the zero function, f(x)=0
    //   0:  the constant function, f(x) = c
    //   1:  the linear function, f(x) = ax+c
    // ...
    i64 maxDegree = 0;
    for (auto& c : mCoefficients) {
      if (c.size() > maxDegree)
        maxDegree = c.size();
    }

    --maxDegree;

    if (maxDegree > 1)
      throw std::runtime_error("not implemented" LOCATION);
    auto pIdx = self.getRuntime().mPartyIdx;

    for (u64 c = 0; c < mCoefficients.size(); ++c) {
      if (mCoefficients[c].size() > 1) {
        outputs[c].mShares[0].resizeLike(inputs.mShares[0]);
        outputs[c].mShares[1].resizeLike(inputs.mShares[1]);
        outputs[c].mShares[0].setZero();
        outputs[c].mShares[1].setZero();

        auto constant = mCoefficients[c][0].getFixedPoint(decimal);
        if (constant && pIdx < 2)
          outputs[c].mShares[pIdx].setConstant(constant);

        if (mCoefficients[c][1].mIsInteger == false)
          throw std::runtime_error("not implemented" LOCATION);

        i64 integerCoeff = mCoefficients[c][1].getInteger();
        outputs[c].mShares[0] += integerCoeff * inputs.mShares[0];
        outputs[c].mShares[1] += integerCoeff * inputs.mShares[1];
      }
    }

    return self;
  }
}  // namespace primihub
