
#include "src/primihub/primitive/circuit/circuit_library.h"
#include "src/primihub/primitive/ppa/kogge_stone.h"

namespace primihub {

BetaCircuit *CircuitLibrary::int_Sh3Piecewise_helper(u64 size,
                                                     u64 numThesholds) {
  auto key = "Sh3Piecewise_helper" + std::to_string(size) + "_" +
             std::to_string(numThesholds);

  auto iter = mCirMap.find(key);

  if (iter == mCirMap.end()) {
    auto *cd = new BetaCircuit;

    std::vector<BetaBundle> aa(numThesholds);
    for (auto &a : aa) {
      a.mWires.resize(size);
      cd->addInputBundle(a);
    }

    BetaBundle b(size);
    cd->addInputBundle(b);

    std::vector<BetaBundle> cc(numThesholds + 1);
    for (auto &c : cc) {
      c.mWires.resize(1);
      cd->addOutputBundle(c);
    }

    int_Sh3Piecewise_build_do(*cd, aa, b, cc);

    iter = mCirMap.insert(std::make_pair(key, cd)).first;
  }

  return iter->second;
}

void CircuitLibrary::int_Sh3Piecewise_build_do(BetaCircuit &cd,
                                               span<BetaBundle> aa,
                                               const BetaBundle &b,
                                               span<BetaBundle> cc) {
  std::vector<BetaBundle> temps(aa.size()), thresholds(aa.size());

  for (u64 t = 0; t < thresholds.size(); ++t) {
    thresholds[t].mWires.resize(1);
    temps[t].mWires.resize(b.mWires.size() * 2);
    cd.addTempWireBundle(temps[t]);
  }

  for (u64 t = 1; t < thresholds.size() - 1; ++t)
    cd.addTempWireBundle(thresholds[t]);

  // the first region bit is just the thrshold.
  thresholds[0] = cc[0];

  // compute all the signs
  for (u64 t = 0; t < thresholds.size(); ++t) {
    KoggeStoneLibrary lib;
    lib.int_int_add_msb_build_optimized(&cd, aa[t], b, thresholds[t], temps[t]);
  }

  // Take the and of the signs
  for (u64 t = 1; t < thresholds.size(); ++t) {
    cd.addGate(thresholds[t - 1].mWires[0], thresholds[t].mWires[0],
               GateType::na_And, cc[t].mWires[0]);
  }

  // mark the last region as the inverse of the last threshold bit.
  cd.addInvert(thresholds.back().mWires[0], cc[thresholds.size()].mWires[0]);
}

void CircuitLibrary::Preproc_build(BetaCircuit &cd, u64 dec) {
  const auto size = sizeof(i64) * 8;
  BetaBundle a(size);
  BetaBundle b0(size);
  BetaBundle c0(size);
  BetaBundle b1(size - dec);
  BetaBundle c1(size - dec);

  cd.addInputBundle(a);
  // std::cout << "a " << a.mWires[0] << "-" << a.mWires.back() << std::endl;
  cd.addInputBundle(b0);
  cd.addInputBundle(b1);
  cd.addOutputBundle(c0);
  cd.addOutputBundle(c1);

  BetaBundle t(3);
  cd.addTempWireBundle(t);

  BetaLibrary lib;
  lib.int_int_add_build_so(cd, a, b0, c0, t);

  a.mWires.erase(a.mWires.begin(), a.mWires.begin() + dec);

  lib.int_int_add_build_so(cd, a, b1, c1, t);

  if (cd.mNonlinearGateCount != 2 * (size - 1) - dec)
    throw std::runtime_error(LOCATION);
}

void CircuitLibrary::argMax_build(BetaCircuit &cd, u64 dec, u64 numArgs) {
  auto size = 64 - dec;
  std::vector<BetaBundle> a0(numArgs), a1(numArgs);

  std::vector<BetaBundle> a(numArgs);

  // std::vector<std::vector<BetaBundle>> select(log2ceil(numArgs));

  for (u64 i = 0; i < numArgs; ++i) {
    a[i].mWires.resize(size);
    a0[i].mWires.resize(size);
    a1[i].mWires.resize(size);

    cd.addInputBundle(a0[i]);
    cd.addInputBundle(a1[i]);
  }

  BetaBundle argMax(log2ceil(numArgs));
  cd.addOutputBundle(argMax);

  // set argMax to zero initially...
  for (auto &w : argMax.mWires)
    cd.addConst(w, 0);

  BetaBundle t(3);
  cd.addTempWireBundle(t);
  BetaLibrary lib;
  for (u64 i = 0; i < numArgs; ++i) {
    cd.addTempWireBundle(a[i]);
    lib.int_int_add_build_so(cd, a0[i], a1[i], a[i], t);

    cd.addPrint("a[" + std::to_string(i) + "] = ");
    cd.addPrint(a[i]);
    cd.addPrint("\n");
  }

  // maxPointer will equal 1 if the the rhs if greater.
  BetaBundle maxPointer(1);
  maxPointer[0] = argMax[0];
  auto &max = a[0];

  lib.int_int_lt_build(cd, max, a[1], maxPointer);

  for (u64 i = 2; i < numArgs; ++i) {
    // currently, max = max(a[0],...,a[i-2]). Now make
    // max = max(a[0],...,a[i-1])
    // max = maxPointer ? a[i - 1] : max;
    lib.int_int_multiplex_build(cd, a[i - 1], max, maxPointer, max, t);

    cd.addPrint("max({0, ...," + std::to_string(i - 1) + " }) = ");
    cd.addPrint(max);
    cd.addPrint(" @ ");
    cd.addPrint(argMax);
    cd.addPrint(" * ");
    cd.addPrint(maxPointer);
    cd.addPrint("\n");

    // now compute which is greater (a[i], max) and store the result
    // in maxPointer
    cd.addTempWireBundle(maxPointer);
    lib.int_int_lt_build(cd, max, a[i], maxPointer);

    // construct a const wire bundle that encodes the index i.
    BetaBundle idx(log2ceil(numArgs));
    cd.addConstBundle(idx, BitVector((u8 *)&i, log2ceil(numArgs)));

    // argMax = (max < a[i]) ? i : argMax ;
    // argMax =   maxPointer ? i : argMax ;
    lib.int_int_multiplex_build(cd, idx, argMax, maxPointer, argMax, t);
  }

  cd.addPrint("max({0, ...," + std::to_string(numArgs) + " }) = _____ @ ");
  cd.addPrint(argMax);
  cd.addPrint("\n");
}

BetaCircuit *CircuitLibrary::convert_arith_to_bin(u64 n, u64 bits) {
  auto key =
      "convert_arith_to_bin" + std::to_string(n) + "_" + std::to_string(bits);

  auto iter = mCirMap.find(key);

  if (iter == mCirMap.end()) {
    auto *cd = new BetaCircuit;

    std::array<BetaBundle, 2> inputs, inSubs;
    BetaBundle outputs, outSub;

    inputs[0].mWires.resize(bits * n);
    inputs[1].mWires.resize(bits * n);
    outputs.mWires.resize(bits * n);
    cd->addInputBundle(inputs[0]);
    cd->addInputBundle(inputs[1]);
    cd->addOutputBundle(outputs);

    BetaBundle temp(bits * 2);
    auto iter0 = inputs[0].mWires.begin();
    auto iter1 = inputs[1].mWires.begin();
    auto iter2 = outputs.mWires.begin();
    for (u64 i = 0; i < inputs.size(); ++i) {
      inSubs[0].mWires.clear();
      inSubs[1].mWires.clear();
      outSub.mWires.clear();

      inSubs[0].mWires.insert(inSubs[0].mWires.end(), iter0, iter0 + bits);
      inSubs[1].mWires.insert(inSubs[1].mWires.end(), iter1, iter1 + bits);
      outSub.mWires.insert(outSub.mWires.end(), iter2, iter2 + bits);

      iter0 += bits;
      iter1 += bits;
      iter2 += bits;

      cd->addTempWireBundle(temp);
      int_int_add_build_do(*cd, inSubs[0], inSubs[1], outSub, temp);
    }

    iter = mCirMap.insert(std::make_pair(key, cd)).first;
  }

  return iter->second;
}

} // namespace primihub
