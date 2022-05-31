
#include "src/primihub/primitive/circuit/beta_library.h"
#ifdef ENABLE_CIRCUITS

#include "src/primihub/primitive/circuit/gate.h"

namespace primihub {

BetaLibrary::BetaLibrary() {}

BetaLibrary::~BetaLibrary() {
  for (auto cir : mCirMap) {
    delete cir.second;
  }
}

BetaCircuit * BetaLibrary::int_int_add(u64 aSize, u64 bSize, u64 cSize,
  Optimized op) {
  auto key = "add" + std::to_string(int(op)) + "_"
    + std::to_string(aSize) + "x" + std::to_string(bSize)
    + "x" + std::to_string(cSize);

  auto iter = mCirMap.find(key);

  if (iter == mCirMap.end()) {
    auto* cd = new BetaCircuit;

    BetaBundle a(aSize);
    BetaBundle b(bSize);
    BetaBundle c(cSize);

    cd->addInputBundle(a);
    cd->addInputBundle(b);

    cd->addOutputBundle(c);

    if (op == Optimized::Size) {
      BetaBundle t(3);
      cd->addTempWireBundle(t);
      int_int_add_build_so(*cd, a, b, c, t);
    } else {
      BetaBundle t(aSize * 2);
      cd->addTempWireBundle(t);
      int_int_add_build_do(*cd, a, b, c, t);
    }

    iter = mCirMap.insert(std::make_pair(key, cd)).first;
  }
  return iter->second;
}

BetaCircuit * BetaLibrary::int_int_add_msb(u64 aSize) {
  auto key = "add_msb_" + std::to_string(aSize);
  auto iter = mCirMap.find(key);
  if (iter == mCirMap.end()) {
    auto* cd = new BetaCircuit;

    BetaBundle a(aSize);
    BetaBundle b(aSize);
    BetaBundle c(1);

    cd->addInputBundle(a);
    cd->addInputBundle(b);

    cd->addOutputBundle(c);

    BetaBundle t(aSize * 2);
    cd->addTempWireBundle(t);
    int_int_add_msb_build_do(*cd, a, b, c, t);

    iter = mCirMap.insert(std::make_pair(key, cd)).first;
  }
  return iter->second;
}

    BetaCircuit * BetaLibrary::uint_uint_add(u64 aSize, u64 bSize, u64 cSize)
    {
        auto key = "uintAdd" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);
            BetaBundle t(3);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            uint_uint_add_build(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_intConst_add(
        u64 aSize,
        u64 bSize,
        i64 bVal,
        u64 cSize)
    {
        auto key = "add" + std::to_string(aSize) + "xConst" + std::to_string(bSize) + "v" + std::to_string(bVal) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);
            BetaBundle t(3);

            cd->addInputBundle(a);

            BitVector bb((u8*)&bVal, bSize);
            cd->addConstBundle(b, bb);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            int_int_add_build_so(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_int_subtract(u64 aSize, u64 bSize, u64 cSize)
    {
        auto key = "subtract" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);
            BetaBundle t(3);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            int_int_subtract_build(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit* BetaLibrary::int_int_sub_msb(u64 aSize, u64 bSize, u64 cSize)
    {
        auto key = "sub_msb_" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(1);
            BetaBundle t(2* aSize + 3);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            int_int_sub_msb_build_do(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::uint_uint_subtract(u64 aSize, u64 bSize, u64 cSize)
    {

        auto key = "uintSubtract" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);
            BetaBundle t(3);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            uint_uint_subtract_build(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_intConst_subtract(u64 aSize, u64 bSize, i64 bVal, u64 cSize)
    {
        auto key = "subtract" + std::to_string(aSize) + "xConst" + std::to_string(bSize) + "v" + std::to_string(bVal) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);
            BetaBundle t(3);

            cd->addInputBundle(a);

            BitVector bb((u8*)&bVal, bSize);
            cd->addConstBundle(b, bb);

            cd->addOutputBundle(c);

            cd->addTempWireBundle(t);

            int_int_subtract_build(*cd, a, b, c, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_int_mult(u64 aSize, u64 bSize, u64 cSize, Optimized op)
    {
        auto key = "mult" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize) + "x" + std::to_string(int(op));

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            int_int_mult_build(*cd, a, b, c, op, true);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;

    }

    BetaCircuit * BetaLibrary::uint_uint_mult(u64 aSize, u64 bSize, u64 cSize, Optimized op)
    {
        auto key = "umult" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize) + "x" + std::to_string(int(op));

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            int_int_mult_build(*cd, a, b, c, op, false);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;

    }


    BetaCircuit * BetaLibrary::int_int_div(u64 aSize, u64 bSize, u64 cSize)
    {

        auto key = "div" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle q(cSize);
            BetaBundle r(0);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(q);
            //cd->addOutputBundle(r);

            int_int_div_rem_build(*cd, a, b, q, r);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_bitInvert(u64 aSize)
    {
        auto key = "bitInvert" + std::to_string(aSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle c(aSize);

            cd->addInputBundle(a);
            cd->addOutputBundle(c);

            int_bitInvert_build(*cd, a, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

BetaCircuit * BetaLibrary::int_int_bitwiseAnd(u64 aSize, u64 bSize,
  u64 cSize) {
  auto key = "bitwiseAnd" + std::to_string(aSize) + "x"
    + std::to_string(bSize) + "x" + std::to_string(cSize);

  auto iter = mCirMap.find(key);

  if (iter == mCirMap.end()) {
    auto* cd = new BetaCircuit;

    BetaBundle a(aSize);
    BetaBundle b(bSize);
    BetaBundle c(cSize);

    cd->addInputBundle(a);
    cd->addInputBundle(b);

    cd->addOutputBundle(c);

    int_int_bitwiseAnd_build(*cd, a, b, c);

    iter = mCirMap.insert(std::make_pair(key, cd)).first;
  }

  return iter->second;
}

    BetaCircuit * BetaLibrary::int_int_bitwiseOr(u64 aSize, u64 bSize, u64 cSize)
    {
        auto key = "bitwiseOr" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            int_int_bitwiseOr_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }
    BetaCircuit * BetaLibrary::int_int_bitwiseXor(u64 aSize, u64 bSize, u64 cSize)
    {
        auto key = "bitwiseXor" + std::to_string(aSize) + "x" + std::to_string(bSize) + "x" + std::to_string(cSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(cSize);

            cd->addInputBundle(a);
            cd->addInputBundle(b);

            cd->addOutputBundle(c);

            int_int_bitwiseXor_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::aes_exapnded(u64 rounds)
    {

        auto key = "aes" + std::to_string(rounds);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle m(128);
            BetaBundle k(128 * rounds + 128);
            BetaBundle c(128);

            cd->addInputBundle(m);
            cd->addInputBundle(k);

            cd->addOutputBundle(c);

            aes_exapnded_build(*cd, m, k, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }



    BetaCircuit * BetaLibrary::int_int_lt(u64 aSize, u64 bSize)
    {
        auto key = "lessThan" + std::to_string(aSize) + "x" + std::to_string(bSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            int_int_lt_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_int_gteq(u64 aSize, u64 bSize)
    {
        auto key = "greatThanEq" + std::to_string(aSize) + "x" + std::to_string(bSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            int_int_gteq_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }


    BetaCircuit * BetaLibrary::int_eq(u64 aSize)
    {
        auto key = "eq" + std::to_string(aSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(aSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            int_int_eq_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }
    BetaCircuit * BetaLibrary::int_neq(u64 aSize)
    {
        auto key = "neq" + std::to_string(aSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(aSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            int_int_eq_build(*cd, a, b, c);
            cd->addInvert(c.mWires[0]);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }


    BetaCircuit * BetaLibrary::int_isZero(u64 aSize)
    {
        auto key = "iz" + std::to_string(aSize);
        auto iter = mCirMap.find(key);
        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle c(aSize);

            cd->addInputBundle(a);
            cd->addOutputBundle(c);

            int_isZero_build(*cd, a, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::uint_uint_lt(u64 aSize, u64 bSize)
    {
        auto key = "uintLessThan" + std::to_string(aSize) + "x" + std::to_string(bSize);
        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            uint_uint_lt_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::uint_uint_gteq(u64 aSize, u64 bSize)
    {
        auto key = "uintGreatThanEq" + std::to_string(aSize) + "x" + std::to_string(bSize);
        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(bSize);
            BetaBundle c(1);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addOutputBundle(c);

            uint_uint_gteq_build(*cd, a, b, c);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_int_multiplex(u64 aSize)
    {

        auto key = "multiplex" + std::to_string(aSize);

        auto iter = mCirMap.find(key);

        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle b(aSize);
            BetaBundle c(1);
            BetaBundle d(aSize);
            BetaBundle t(3);

            cd->addInputBundle(a);
            cd->addInputBundle(b);
            cd->addInputBundle(c);
            cd->addOutputBundle(d);
            cd->addTempWireBundle(t);

            int_int_multiplex_build(*cd, a, b, c, d, t);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_removeSign(u64 aSize)
    {
        auto key = "removeSign" + std::to_string(aSize);
        auto iter = mCirMap.find(key);
        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle c(aSize);
            BetaBundle temp(3);

            cd->addInputBundle(a);
            cd->addOutputBundle(c);
            cd->addTempWireBundle(temp);

            int_removeSign_build(*cd, a, c, temp);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_addSign(u64 aSize)
    {
        auto key = "addSign" + std::to_string(aSize);
        auto iter = mCirMap.find(key);
        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle c(aSize);
            BetaBundle sign(1);
            BetaBundle temp(3);

            cd->addInputBundle(a);
            cd->addInputBundle(sign);
            cd->addOutputBundle(c);
            cd->addTempWireBundle(temp);

            int_addSign_build(*cd, a, sign, c, temp);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }

    BetaCircuit * BetaLibrary::int_negate(u64 aSize)
    {
        auto key = "negate" + std::to_string(aSize);
        auto iter = mCirMap.find(key);
        if (iter == mCirMap.end())
        {
            auto* cd = new BetaCircuit;

            BetaBundle a(aSize);
            BetaBundle c(aSize);
            BetaBundle temp(3);

            cd->addInputBundle(a);
            cd->addOutputBundle(c);
            cd->addTempWireBundle(temp);

            int_negate_build(*cd, a, c, temp);

            iter = mCirMap.insert(std::make_pair(key, cd)).first;
        }

        return iter->second;
    }


    void BetaLibrary::int_int_add_build_so(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & sum,
        const BetaBundle & temps)
    {

        if (temps.mWires.size() < 3)
            throw std::runtime_error(LOCATION);

        if (sum.mWires.size() > std::max<u64>(a1.mWires.size(), a2.mWires.size()) + 1)
            throw std::runtime_error(LOCATION);

        const BetaWire& carry = temps.mWires[0];
        const BetaWire& aXorC = temps.mWires[1];
        const BetaWire& temp = temps.mWires[2];

        if (!areDistint(a2, sum) || !areDistint(a1, sum))
            throw std::runtime_error("must be distinct" LOCATION);

        u64 a1Size = a1.mWires.size();
        u64 a2Size = a2.mWires.size();
        u64 minSize = sum.mWires.size();

        // sum is computed as a1[i] ^ a2[i] ^ carry[i-1]
        // carry[i] is computed as
        //
        //  carry[i-1] -*--*--------------------*
        //              |  |                    |
        //              |  >= xor ---*          >= xor --- carry[i]
        //              |  |         |          |
        //  a2[i] ------|--*         >== and ---*
        //              |            |
        //              >==== xor ---*
        //              |
        //  a1[i] ------*

        // half adder
        cd.addGate(a1.mWires[0], a2.mWires[0], GateType::Xor, sum.mWires[0]);

        // now do the full adder while we have inputs from both a1,a2
        u64 i = 1;
        if (minSize > 1)
        {

            // compute the carry from the 0 bits (special case)
            cd.addGate(a1.mWires[i - 1], a2.mWires[i - 1], GateType::And, carry);

            u64 a1Idx = std::min<u64>(i, a1Size - 1);
            u64 a2Idx = std::min<u64>(i, a2Size - 1);

            // compute the sum
            cd.addGate(a1.mWires[a1Idx], carry, GateType::Xor, aXorC);
            cd.addGate(a2.mWires[a2Idx], aXorC, GateType::Xor, sum.mWires[i]);

            // general case.
            for (i = 2; i < minSize; ++i)
            {
                // compute the previous carry
                cd.addGate(a2.mWires[a2Idx], carry, GateType::Xor, temp);
                cd.addGate(temp, aXorC, GateType::And, temp);
                cd.addGate(temp, carry, GateType::Xor, carry);


                a1Idx = std::min<u64>(i, a1Size - 1);
                a2Idx = std::min<u64>(i, a2Size - 1);

                cd.addGate(a1.mWires[a1Idx], carry, GateType::Xor, aXorC);
                cd.addGate(a2.mWires[a2Idx], aXorC, GateType::Xor, sum.mWires[i]);
            }
        }

    }

void BetaLibrary::int_int_add_build_do(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & sum,
  const BetaBundle & temps) {
  u64 a1Size = a1.mWires.size();
  u64 a2Size = a2.mWires.size();
  u64 sSize = sum.mWires.size();

  if (a1Size != a2Size || a1Size != sSize)
      throw std::runtime_error("inputs must be same size " LOCATION);

  if (temps.mWires.size() < a1Size * 2)
      throw std::runtime_error("need more temp bits " LOCATION);

  if (!areDistint(a2, sum) || !areDistint(a1, sum))
      throw std::runtime_error("must be distinct" LOCATION);

  // This is a Parallel Prefix Adder where we use generate & propegate
  // The main idea is that for each bit position, we first compute two
  // bits, propegate P[i] and generate G[i]. P[i] denote that at bit
  // position i, if there is a "carry-in", then this position will propegate
  // the carry to position i+1. G[i] denotes that position i will always
  // result in a carry.  The sum is then S[i] = P[i] ^ G[i-1]. Importantly,
  // the P[i], G[i] bits can be computed in a tree structure. First observe
  // that a region of bits [i,i-1,...,j+1,j] = [i:j] themselves can generate
  // a pair P[*], G[*]. This will denote wether that region as a block will
  // properate or generate out the most siginificant postion.
  // We can therefore compute:
  //
  //  P[i] = P[0:i] = P[j:i] & P[0:j-1].
  //  G[i] = G[0:i] = G[j:i] or (G[0:j-1] & P[j:i])
  //                = G[j:i] ^  (G[0:j-1] & P[j:i])
  //
  // Note that this holds for all regions, not just [0:i]. We then compute a
  // binary tree of these bits. For the first level of the tree (leaves)
  // we compute:
  //
  //  P[i:i] = A[i] ^ B[i]
  //  G[i:i] = A[i] & B[i]
  //
  // Also see: Harris, D. A taxonomy of parallel prefix networks. In IEEE ASILOMAR (2003).

  BetaBundle PP, GG;
  auto& P = PP.mWires;
  auto& G = GG.mWires;

  P.insert(P.end(), temps.mWires.begin(), temps.mWires.begin() + a1Size);
  G.insert(G.end(), temps.mWires.begin() + a1Size, temps.mWires.begin() + a1Size * 2 - 1);

  auto tempWire = temps.mWires.back();
  P[0] = sum.mWires[0];

  //cd.addPrint("\n\nA: ");
  //cd.addPrint(a1);
  //cd.addPrint("\nB: ");
  //cd.addPrint(a2);

  for (u64 i = 0; i < a1Size; ++i) {
    cd.addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);

    if (i < a1Size - 1)
      cd.addGate(a1.mWires[i], a2.mWires[i], GateType::And, G[i]);
  }

  // Sklansky algorthm
  auto d = log2ceil(a1Size);
  for (u64 level = 0; level < d; ++level) {
    //cd.addPrint("\nG " + std::to_string(level) + ": ");
    //cd.addPrint(GG);
    //cd.addPrint("\nP " + std::to_string(level) + ": ");
    //cd.addPrint(PP);

    // 1,2,4,8,16,32,64,...
    auto startPos = 1ull << level;
    auto step = 1 << (level + 1);

    bool first = true;
    for (u64 i = startPos; i < a1Size; i += step) {
      auto lowWire = i - 1;

      auto endPos = std::min<u64>(i + startPos, a1Size);
      for (auto curWire = i; curWire < endPos; ++curWire) {
        auto P0 = P[lowWire];
        auto G0 = G[lowWire];

        auto P1 = P[curWire];

        if (curWire < a1Size - 1) {
          auto G1 = G[curWire];

          // G1 = G1 ^ P1 & G0
          cd.addGate(P1, G0, GateType::And, tempWire);
          cd.addGate(tempWire, G1, GateType::Xor, G1);
        }

        // If this is the first merge of this level, then we know the
        // propegate in is pointless since there is no global carry in.
        if (!first) {
          // P1 = P1 & P0
          cd.addGate(P0, P1, GateType::And, P1);
        }
      }
      first = false;
    }
  }

  //cd.addPrint("\nG " + std::to_string(d) + ": ");
  //cd.addPrint(GG);
  //cd.addPrint("\nP " + std::to_string(d) + ": ");
  //cd.addPrint(PP);
  //cd.addPrint("\n");

  for (u64 i = 1; i < a1Size; ++i) {
    // s[i] = P[i] ^ G[i-1]
    cd.addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
    cd.addGate(P[i], G[i - 1], GateType::Xor, sum.mWires[i]);
  }

  //cd.addPrint("\nS: ");
  //cd.addPrint(sum);
  //cd.addPrint("\n\n");
}

void BetaLibrary::int_int_add_msb_build_do(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & msb,
  const BetaBundle & temps) {
  u64 a1Size = a1.mWires.size();
  u64 a2Size = a2.mWires.size();
  u64 sSize = msb.mWires.size();

  if (a1Size != a2Size || 1 != sSize)
    throw std::runtime_error("inputs must be same size " LOCATION);

  if (temps.mWires.size() < a1Size * 2)
    throw std::runtime_error("need more temp bits " LOCATION);

    //if (!areDistint(a2, sum) || !areDistint(a1, sum))
    //	throw std::runtime_error("must be distinct" LOCATION);

    // This is a Parallel Prefix Adder where we use generate & propegate
    // The main idea is that for each bit position, we first compute two
    // bits, propegate P[i] and generate G[i]. P[i] denote that at bit
    // position i, if there is a "carry-in", then this position will propegate
    // the carry to position i+1. G[i] denotes that position i will always
    // result in a carry.  The sum is then S[i] = P[i] ^ G[i-1]. Importantly,
    // the P[i], G[i] bits can be computed in a tree structure. First observe
    // that a region of bits [i,i-1,...,j+1,j] = [i:j] themselves can generate
    // a pair P[*], G[*]. This will denote wether that region as a block will
    // properate or generate out the most siginificant postion.
    // We can therefore compute:
    //
    //  P[i] = P[i:0] = P[i:j] & P[j-1:0].
    //  G[i] = G[i:0] = G[i:j] ^ G[j-1:0] & P[j:0]
    //
    // Note that this holds for all regions, not just [i:0]. We then compute a
    // binary tree of these bits. For the first level of the tree (leaves)
    // we compute:
    //
    //  P[i:i] = A[i] ^ B[i]
    //  G[i:i] = A[i] & B[i]
    //
    // Also see: Harris, D. A taxonomy of parallel prefix networks. In IEEE ASILOMAR (2003).


    BetaBundle PP, GG;
    auto& P = PP.mWires;
    auto& G = GG.mWires;

    P.insert(P.end(), temps.mWires.begin(), temps.mWires.begin() + a1Size);
    G.insert(G.end(), temps.mWires.begin() + a1Size,
      temps.mWires.begin() + a1Size * 2 - 1);

    auto tempWire = temps.mWires.back();

    if (a1Size == 1) {
      P[0] = msb.mWires[0];
    }

    //cd.addPrint("\n\nA: ");
    //cd.addPrint(a1);
    //cd.addPrint("\nB: ");
    //cd.addPrint(a2);

    for (u64 i = 0; i < a1Size - 1; ++i) {
      if (i) cd.addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
      cd.addGate(a1.mWires[i], a2.mWires[i], GateType::And, G[i]);
    }

    // Sklansky algorthm
    auto d = log2ceil(a1Size);
    for (u64 level = 0; level < d; ++level) {
        //cd.addPrint("\nG " + std::to_string(level) + ": ");
        //cd.addPrint(GG);
        //cd.addPrint("\nP " + std::to_string(level) + ": ");
        //cd.addPrint(PP);

        // 1,2,4,8,16,32,64,...
        auto startPos = 1 << level;
        auto step = 1 << (level + 1);

        bool first = true;
        for (u64 i = startPos; i < a1Size; i += step) {
          auto lowWire = i - 1;
          auto curWire = std::min(lowWire + startPos, a1Size - 2);

          if (curWire != lowWire) {
            auto G0 = G[lowWire];

            auto P1 = P[curWire];
            auto G1 = G[curWire];

            // G1 = G1 ^ P1 & G0
            cd.addGate(P1, G0, GateType::And, tempWire);
            cd.addGate(tempWire, G1, GateType::Xor, G1);

            // If this is the first merge of this level, then we know the
            // propegate in is pointless since there is no global carry in.
            if (!first) {
              auto P0 = P[lowWire];
              auto P1 = P[curWire];
              // P1 = P1 & P0
              cd.addGate(P0, P1, GateType::And, P1);
            }
          }
          first = false;
        }
    }

    //cd.addPrint("\nG " + std::to_string(d) + ": ");
    //cd.addPrint(GG);
    //cd.addPrint("\nP " + std::to_string(d) + ": ");
    //cd.addPrint(PP);
    //cd.addPrint("\n");

    // s[i] = P[i] ^ G[i-1]
    cd.addGate(a1.mWires.back(), a2.mWires.back(), GateType::Xor, P.back());
    cd.addGate(P.back(), G.back(), GateType::Xor, msb.mWires.back());

    //cd.addPrint("\nS: ");
    //cd.addPrint(sum);
    //cd.addPrint("\n\n");
}

void BetaLibrary::uint_uint_add_build(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & sum,
  const BetaBundle & temps) {
  if (sum.mWires.size() >
    std::max<u64>(a1.mWires.size(), a2.mWires.size()) + 1)
    throw std::runtime_error(LOCATION);

  BetaBundle extra(2);
  cd.addConstBundle(extra, BitVector(2));

  BetaBundle aa1 = a1;
  BetaBundle aa2 = a2;

  aa1.mWires.push_back(extra.mWires[0]);
  aa2.mWires.push_back(extra.mWires[1]);

  int_int_add_build_so(cd, aa1, aa2, sum, temps);
}

void BetaLibrary::int_int_subtract_build(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & diff,
  const BetaBundle & temps) {
  if (diff.mWires.size() >
    std::max<u64>(a1.mWires.size(), a2.mWires.size()) + 1)
    throw std::runtime_error(LOCATION);

  if (!areDistint(a2, diff) || !areDistint(a1, diff))
    throw std::runtime_error("must be distinct " LOCATION);

  u64 a1Size = a1.mWires.size();
  u64 a2Size = a2.mWires.size();
  u64 minSize = diff.mWires.size();

  BetaWire borrow = temps.mWires[0];
  BetaWire aXorBorrow = temps.mWires[1];
  BetaWire temp = temps.mWires[2];
  const std::vector<BetaWire>& d = diff.mWires;

  // we are computing a1 - a2 = diff
  // diff is computed as a1[i] ^ a2[i] ^ borrow[i-1]
  // borrow[i] is computed as
  //
  //  a1[i] ------*--*-------------------*
  //              |  |                   |
  //              |  >= xor ---*         >= xor --- borrow[i]
  //              |  |         |         |
  //  a2[i] ------|--*          >= or ---*
  //              |            |
  //              >==== xor ---*
  //              |
  // borrow[i-1] -*

  u64 i = 0;
  if (minSize) {
    cd.addGate(a1.mWires[0], a2.mWires[0], GateType::Xor, diff.mWires[0]);
    ++i;

    if (minSize > 1) {
      cd.addGate(a1.mWires[0], a2.mWires[0], GateType::na_And, borrow);

      u64 a1Idx = std::min<u64>(1, a1Size - 1);
      u64 a2Idx = std::min<u64>(1, a2Size - 1);

      // second bit is the xor of borrow and input;
      cd.addGate(borrow, a1.mWires[a1Idx], GateType::Xor, aXorBorrow);
      cd.addGate(aXorBorrow, a2.mWires[a2Idx], GateType::Xor, d[1]);
      ++i;

      for (; i < minSize; ++i) {
        // compute the borrow of the previous bit which itself has a borrow in.
        cd.addGate(a1.mWires[a1Idx], a2.mWires[a2Idx], GateType::Xor, temp);
        cd.addGate(aXorBorrow, temp, GateType::Or, temp);
        cd.addGate(temp, a1.mWires[a1Idx], GateType::Xor, borrow);

        a1Idx = std::min<u64>(i, a1Size - 1);
        a2Idx = std::min<u64>(i, a2Size - 1);

        // compute the difference as the xor of the input and prev borrow.
        cd.addGate(borrow, a1.mWires[a1Idx], GateType::Xor, aXorBorrow);
        cd.addGate(aXorBorrow, a2.mWires[a2Idx], GateType::Xor, d[i]);
      }
    }
  }
}

void BetaLibrary::int_int_sub_msb_build_do(
  BetaCircuit& cd,
  const BetaBundle& a1,
  const BetaBundle& a2,
  const BetaBundle& msb,
  const BetaBundle& temps) {
  if (a1.size() != a2.size())
    throw RTE_LOC;
  if (msb.size() != 1)
    throw RTE_LOC;

  auto m = temps.mWires.begin() + a1.size() - 1;
  BetaBundle diff, rest;
  diff.mWires.insert(diff.mWires.end(), temps.mWires.begin(), m);
  rest.mWires.insert(rest.mWires.end(), m, temps.mWires.end());

  diff.mWires.push_back(msb.mWires[0]);

  int_int_subtract_build(cd, a1, a2, diff, rest);
}

void BetaLibrary::uint_uint_subtract_build(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & diff,
  const BetaBundle & temps) {
  if (diff.mWires.size() >
    std::max<u64>(a1.mWires.size(), a2.mWires.size()) + 1)
    throw std::runtime_error(LOCATION);

  BetaBundle extra(2);
  cd.addConstBundle(extra, BitVector(2));

  BetaBundle aa1 = a1;
  BetaBundle aa2 = a2;

  aa1.mWires.push_back(extra.mWires[0]);
  aa2.mWires.push_back(extra.mWires[1]);

  int_int_subtract_build(cd, aa1, aa2, diff, temps);
}

void BetaLibrary::int_int_mult_build(
  BetaCircuit & cd,
  const BetaBundle & a,
  const BetaBundle & b,
  const BetaBundle & c,
  Optimized op,
  bool useTwoComplement) {
  if (c.mWires.size() > a.mWires.size() + b.mWires.size())
    throw std::runtime_error(LOCATION);

  if (a.size() >= b.size()) {
    u64 numRows = useTwoComplement ? c.mWires.size()
      : std::min(b.size(), c.size());

    //cd.addPrint("\na = ");
    //cd.addPrint(a);
    //cd.addPrint("\nb = ");
    //cd.addPrint(b);
    //cd.addPrint("\n~~~~~~~~~~~~~~~~~~~~\n");

    // rows will hold
    // {  b[0] * a ,
    //    b[1] * a ,
    //    ...      ,
    //    b[n] * a }
    // where row i contains min(c.mWires.size() - i, a.mWires.size())
    std::vector<BetaBundle> rows(numRows);

    // first, we compute the AND between the two inputs.
    for (u64 i = 0; i < rows.size(); ++i) {
        // this will hold the b[i] * a
        rows[i].mWires.resize(std::min<u64>(c.mWires.size() - i, a.mWires.size()));

        // initialize some unused wires, these will
        // hold intermediate sums.
        cd.addTempWireBundle(rows[i]);

                if (i == 0)
                {
                    // later, we will sum together all the
                    // rows, and this row at idx 0 will be
                    // the running total, so we want it to be
                    // the wires that represent the product c.
                    rows[0].mWires[0] = c.mWires[0];
                }

                if (rows.size() == 1)
                {
                    for (u64 j = 1; j < rows[0].mWires.size(); ++j)
                    {
                        rows[0].mWires[j] = c.mWires[j];
                    }
                }

                if (a.mWires.size() == 1)
                {
                    rows[i].mWires[0] = c.mWires[i];
                }

                const BetaWire& bi = b.mWires[std::min<u64>(i, b.mWires.size() - 1)];

                //u64 prev = cd.mNonlinearGateCount;
                // compute the AND between b[i] * a[j].
                for (u64 j = 0; j < rows[i].mWires.size(); ++j)
                {
                    cd.addGate(
                        bi,
                        a.mWires[j],
                        GateType::And,
                        rows[i].mWires[j]);
                }

                //std::cout << "and[" << i << "] " << cd.mNonXorGateCount <<"  (+"<< (cd.mNonXorGateCount - prev) <<")" << std::endl;
            }

#define SERIAL
#ifdef SERIAL
            if (rows.size() > 1)
            {

                BetaBundle additonTemp(op == Optimized::Size ? 3 : a.mWires.size() * 2), temp2(rows[1].mWires.size());
                cd.addTempWireBundle(additonTemp);
                cd.addTempWireBundle(temp2);

                //cd.addPrint("+");
                //cd.addPrint(rows[0]);
                //cd.addPrint("   = row[0]\n");

                rows[0].mWires.erase(rows[0].mWires.begin());

                // starting with rows[0] + rows[1], sum the rows together
                // note that, after each sum, we will have computed one more
                // bit of the final product.
                for (u64 i = 1; i < rows.size(); i++)
                {
                    BetaBundle sum(std::min<u64>(rows[i].mWires.size() + 1, c.mWires.size() - i));


                    //cd.addPrint("+");
                    //cd.addPrint(rows[i]);
                    //cd.addPrint(std::string(i, ' '));
                    //cd.addPrint("   = row["+std::to_string(i)+"] = a * b[" +std::to_string(i) + "]"
     //                   +"\n----------------------------------------------------------------- row " + std::to_string(i) + " / " + std::to_string(b.mWires.size())
     //                   +", b[" + std::to_string(i) + "] = ");
                    //cd.addPrint(b.mWires[std::min(i, b.mWires.size() - 1)]);
                    //cd.addPrint("\n ");

                    cd.addTempWireBundle(sum);

                    sum.mWires[0] = c.mWires[i];

                    if (i == rows.size() - 1)
                    {
                        for (u64 j = 1; j < sum.mWires.size(); ++j)
                        {
                            sum.mWires[j] = c.mWires[i + j];
                        }
                    }

                    //u64 prev = cd.mNonlinearGateCount;

                    if (useTwoComplement)
                        int_int_add_build_so(cd, rows[i - 1], rows[i], sum, additonTemp);
                    else
                        uint_uint_add_build(cd, rows[i - 1], rows[i], sum, additonTemp);

                    //std::cout << "add[" << i << "] " << cd.mNonXorGateCount << "  (+" << (cd.mNonXorGateCount - prev) << ")" << std::endl;

                    //cd.addPrint(sum);
                    //cd.addPrint(std::string(i, ' ') + "   = sum \n ");
                    //cd.addPrint(c);
                    //cd.addPrint("   =  c\n");

                    rows[i].mWires.clear();
                    rows[i].mWires.insert(rows[i].mWires.begin(), sum.mWires.begin() + 1, sum.mWires.end());
                }
            }


            //cd.addPrint("=");
            //cd.addPrint(c);
            //cd.addPrint("\n\n");
#else
            this code has not been tested and surely contains errors

                // while the serial code above should work, it is more sequential.
                // as such, then using the 'leveled' presentation, fewer operations
                // can be pipelined.

                u64 k = 1, p = 1;
            while (rows.size() > 1)
            {
                std::vector<BetaBundle> newTerms;


                for (u64 i = 0; i < rows.size(); i += 2)
                {
                    BetaBundle additonTemp(3);
                    cd.addTempWireBundle(additonTemp);

                    newTerms.emplace_back(rows[i + 1].mWires.size());
                    auto& prod = newTerms.back();
                    cd.addTempWireBundle(prod);

                    if (i == 0)
                    {
                        for (u64 j = 0; j < k; ++j)
                        {
                            prod.mWires[j] = c.mWires[p++];
                        }

                        k *= 2;
                    }

                    auto sizeDiff = rows[i].mWires.size() - rows[i + 1].mWires.size();

                    std::vector<BetaWire> bottomBits(
                        rows[i].mWires.begin(),
                        rows[i].mWires.begin() + sizeDiff);

                    rows[i].mWires.erase(
                        rows[i].mWires.begin(),
                        rows[i].mWires.begin() + sizeDiff);

                    int_int_add_build(cd, rows[i], rows[i + 1], prod, additonTemp);

                    prod.mWires.insert(prod.mWires.begin(), bottomBits.begin(), bottomBits.end());
                }

                rows = std::move(newTerms);
            }

#endif
        }
        else
        {
            int_int_mult_build(cd, b, a, c, op, useTwoComplement);
        }
    }
    void BetaLibrary::int_int_div_rem_build(
        BetaCircuit & cd,
        const BetaBundle & signedA1,
        const BetaBundle & signedA2,
        const BetaBundle & quotient,
        const BetaBundle & rem
        //,const BetaBundle & divByZero,
        //bool checkDivByZero
    )
    {
        if (quotient.mWires.size() != signedA1.mWires.size())
            throw std::runtime_error(LOCATION);

        // we are computing a1 / a2 = quot  with optional remainder rem
        BetaBundle
            a1Sign(1),
            a2Sign(1),
            sign(1),
            temp(3),
            a1(signedA1.mWires.size()),
            a2(signedA2.mWires.size());

        a1Sign.mWires[0] = signedA1.mWires.back();
        a2Sign.mWires[0] = signedA2.mWires.back();

        cd.addTempWireBundle(sign);
        cd.addTempWireBundle(temp);
        cd.addTempWireBundle(a1);
        cd.addTempWireBundle(a2);

        cd.addGate(a1Sign.mWires.back(), a2Sign.mWires.back(), GateType::Xor, sign.mWires[0]);

        //std::cout << "start      " << cd.mNonXorGateCount << std::endl;

        // a1 = abs(signedA1)
        int_removeSign_build(cd, signedA1, a1, temp);
        //std::cout << "removeSign " << cd.mNonXorGateCount << std::endl;
        int_removeSign_build(cd, signedA2, a2, temp);
        //std::cout << "removeSign " << cd.mNonXorGateCount << std::endl;


        //cd.addPrint(quotient);
        //cd.addPrint("\n");


        BetaBundle remainder(rem.mWires.size());
        cd.addTempWireBundle(remainder);

        uint_uint_div_rem_build(cd, a1, a2, quotient, remainder);

        //cd.addPrint("add quo sign ");
        //cd.addPrint(sign);
        //cd.addPrint("   ");
        //cd.addPrint(quotient);


        int_addSign_build(cd, quotient, sign, quotient, temp);
        //std::cout << "addSign    " << cd.mNonXorGateCount << std::endl;
        //cd.addPrint(" ->  ");
        //cd.addPrint(quotient);

        //cd.addPrint("\n\n");

        if (rem.mWires.size())
        {
            int_addSign_build(cd, remainder, a1Sign, rem, temp);
            //std::cout << "addSign    " << cd.mNonXorGateCount << std::endl;
        }
        //cd.print(" = ", remainder);

    }

    void BetaLibrary::uint_uint_div_rem_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & quotient,
        const BetaBundle & rem)
    {
        BetaBundle
            doSubtract(1),
            temp(3),
            ssub(a1.mWires.size());

        cd.addTempWireBundle(ssub);
        cd.addTempWireBundle(temp);


        u64 shifts = quotient.mWires.size() - 1;


        BetaBundle xtra(shifts + 1);
        cd.addTempWireBundle(xtra);


        BetaBundle remainder, remTemp;
        //cd.addPrint("====================================\na1 = ");
        //cd.addPrint(a1);
        //cd.addPrint("\na2 = ");
        //cd.addPrint(a2);
        //cd.addPrint("\n");

        for (i64 i = shifts; i >= 0; --i)
        {
            remainder.mWires.insert(remainder.mWires.begin(), a1.mWires[i]);
            remTemp.mWires.push_back(xtra.mWires.back());
            xtra.mWires.pop_back();

            //partialrem.push_back(remainder);
            //auto doSubtract = remainder >= a2;
            doSubtract.mWires[0] = quotient.mWires[i];
            //u64 prev = cd.mNonlinearGateCount;

            uint_uint_gteq_build(cd, remainder, a2, doSubtract);


            //std::cout << "iter[" << i << "]  geq  " << cd.mNonXorGateCount << "  (+" << (cd.mNonXorGateCount - prev) << ")  " << remTemp.mWires.size() << std::endl;

            //cd.addPrint("rem[" + std::to_string(i) + "] = ");
            //cd.addPrint(remainder);
            //cd.addPrint("\n");
            //cd.addPrint(remainder);
            //cd.addPrint(" >= ");
            //cd.addPrint(a2);
            //cd.addPrint(" =: ");
            //cd.addPrint(doSubtract);
            //cd.addPrint("\n");



            //prev = cd.mNonXorGateCount;

            BetaBundle sub;
            sub.mWires.insert(sub.mWires.begin(), ssub.mWires.begin(), ssub.mWires.begin() + std::min(a2.mWires.size(), remainder.mWires.size()));

            //for (auto& wire : a2.mWires)
            for (u64 j = 0; j < sub.mWires.size(); ++j)
                cd.addGate(a2.mWires[j], doSubtract.mWires[0], GateType::And, sub.mWires[j]);



            //cd.addPrint(" ( rem[" + std::to_string(i) + "] = ");
            //cd.addPrint(remainder);
            //cd.addPrint(") - (sub[" + std::to_string(i) + "] = ");
            //cd.addPrint(sub);


            //if (cd.mDivPrint)
            //{

            //    cd.addPrint(remainder);
            //    cd.addPrint(" - ");
            //    cd.addPrint(a2);
            //    cd.addPrint("\n");
            //}
            //remainder = remainder - sub;
            //std::cout << "iter[" << i << "]'   " << cd.mNonXorGateCount << "  " << remTemp.mWires.size() << std::endl;

            //prev = cd.mNonXorGateCount;

            uint_uint_subtract_build(cd, remainder, sub, remTemp, temp);


            //std::cout << "iter[" << i << "]  sub  " << cd.mNonXorGateCount << "  (+" << (cd.mNonXorGateCount-prev)<<")  "<< remTemp .mWires.size()<< std::endl;


            std::swap(remTemp.mWires, remainder.mWires);

            //if (cd.mDivPrint)
            //{

                //cd.addPrint(") =: ");
                //cd.addPrint(remainder);
                //cd.addPrint("     quo " );
                //cd.addPrint(quotient);
                //cd.addPrint("\n\n");
            //}


        }
    }


    void BetaLibrary::int_isZero_build(
        BetaCircuit & cd,
        BetaBundle & a1,
        BetaBundle & out)
    {
        if (a1.mWires.size() == 1)
        {
            cd.addCopy(a1, out);
            cd.addInvert(a1.mWires[0]);
        }
        else
        {
            cd.addGate(a1.mWires[0], a1.mWires[1], GateType::Nor, out.mWires[0]);

            for (u64 i = 2; i < a1.mWires.size(); ++i)
                cd.addGate(out.mWires[0], a1.mWires[i], GateType::nb_And, out.mWires[0]);
        }
    }

    void BetaLibrary::int_int_eq_build(
        BetaCircuit & cd,
        BetaBundle & a1,
        BetaBundle & a2,
        BetaBundle & out)
    {
        auto bits = a1.mWires.size();
        BetaBundle temp(1);
        cd.addTempWireBundle(temp);
        cd.addGate(a1.mWires[0], a2.mWires[0],
            GateType::Nxor, out.mWires[0]);

        for (u64 i = 1; i < bits; ++i)
        {
            cd.addGate(a1.mWires[i], a2.mWires[i],
                GateType::Nxor, temp.mWires[0]);

            cd.addGate(temp.mWires[0], out.mWires[0],
                GateType::And, out.mWires[0]);
        }
    }

    void BetaLibrary::int_int_lt_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out)
    {

        TODO("optimize this, we dont need the full subtraction result, only the sign");

        auto bits = std::max(a1.mWires.size(), a2.mWires.size());
        BetaBundle diff(bits + 1), temp(3);
        cd.addTempWireBundle(temp);

        for (u64 i = 0; i < diff.mWires.size(); ++i)
        {
            diff.mWires[i] = out.mWires[0];
        }

        int_int_subtract_build(cd, a1, a2, diff, temp);
    }


    void BetaLibrary::int_int_gteq_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out)
    {
        int_int_lt_build(cd, a1, a2, out);

        // invert the output
        GateType gt = GateType((~(u8)cd.mGates.back().mType) & 15);
        cd.mGates.back().setType(gt);
    }

    void BetaLibrary::uint_uint_lt_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out)
    {

        TODO("optimize this, we dont need the full subtraction result, only the sign");

        auto bits = std::max(a1.mWires.size(), a2.mWires.size());
        BetaBundle diff(bits + 1), temp(3);
        cd.addTempWireBundle(temp);

        for (u64 i = 0; i < diff.mWires.size(); ++i)
        {
            diff.mWires[i] = out.mWires[0];
        }

        uint_uint_subtract_build(cd, a1, a2, diff, temp);

    }

    void BetaLibrary::uint_uint_gteq_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out)
    {
        uint_uint_lt_build(cd, a1, a2, out);

        // invert the output
        cd.addInvert(out.mWires[0]);
    }


    void BetaLibrary::int_removeSign_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & ret,
        const BetaBundle & temp)
    {

        BetaBundle sign(1);
        sign.mWires[0] = a1.mWires.back();

        //auto ret = -a1;
        int_negate_build(cd, a1, ret, temp);

        int_int_multiplex_build(cd, ret, a1, sign, ret, temp);
    }

    void BetaLibrary::int_addSign_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & sign,
        const BetaBundle & ret,
        const BetaBundle & temp)
    {
        //auto ret = -a1;
        //cd.addPrint("\nadd sign ");
        //cd.addPrint(sign);
        //cd.addPrint("  (");
        //cd.addPrint(a1);

        BetaBundle neg(a1.mWires.size());
        cd.addTempWireBundle(neg);

        int_negate_build(cd, a1, neg, temp);

        //cd.addPrint(", ");
        //cd.addPrint(neg);
        //cd.addPrint(")  -> ");

        int_int_multiplex_build(cd, neg, a1, sign, ret, temp);

        //cd.addPrint(ret);
        //cd.addPrint("\n");
    }

    void BetaLibrary::int_bitInvert_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & out)
    {
        cd.addCopy(a1, out);

        for (u64 i = 0; i < out.mWires.size(); ++i)
        {
            cd.addInvert(out.mWires[i]);
        }
    }

    void BetaLibrary::int_negate_build(
        BetaCircuit & cd,
        const BetaBundle & a1,
        const BetaBundle & out,
        const BetaBundle & temp)
    {
        //if(areDistint(a1, out) == false)
        //    throw std::runtime_error(LOCATION);
        // for two's complement, negation is done as out = ~a1 + 1
        BetaBundle invert(a1.mWires.size());
        cd.addTempWireBundle(invert);

        int_bitInvert_build(cd, a1, invert);

        //cd.addPrint("   a1 ");
        //cd.addPrint(a1);
        //cd.addPrint("\n ~a1 ");
        //cd.addPrint(invert);

        BetaBundle one(2);
        BitVector oo(2);
        oo[0] = 1;
        cd.addConstBundle(one, oo);


        int_int_add_build_so(cd, invert, one, out, temp);
        //cd.addPrint("   ~a1+1 ");
        //cd.addPrint(out);
        //cd.addPrint(" ");
    }

void BetaLibrary::int_int_bitwiseAnd_build(
  BetaCircuit & cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & out) {
  if (a1.mWires.size() != a2.mWires.size())
    throw std::runtime_error(LOCATION);
  if (out.mWires.size() > a1.mWires.size())
    throw std::runtime_error(LOCATION);

  for (u64 j = 0; j < out.mWires.size(); ++j) {
    cd.addGate(
      a1.mWires[j],
      a2.mWires[j],
      GateType::And,
      out.mWires[j]);
  }
}

    void BetaLibrary::int_int_bitwiseOr_build(BetaCircuit & cd, const BetaBundle & a1, const BetaBundle & a2, const BetaBundle & out)
    {
        if (a1.mWires.size() != a2.mWires.size())throw std::runtime_error(LOCATION);
        if (out.mWires.size() > a1.mWires.size())throw std::runtime_error(LOCATION);

        for (u64 j = 0; j < out.mWires.size(); ++j)
        {
            cd.addGate(
                a1.mWires[j],
                a2.mWires[j],
                GateType::Or,
                out.mWires[j]);
        }
    }

    void BetaLibrary::int_int_bitwiseXor_build(BetaCircuit & cd, const BetaBundle & a1, const BetaBundle & a2, const BetaBundle & out)
    {
        if (a1.mWires.size() != a2.mWires.size())throw std::runtime_error(LOCATION);
        if (out.mWires.size() > a1.mWires.size())throw std::runtime_error(LOCATION);

        for (u64 j = 0; j < out.mWires.size(); ++j)
        {
            cd.addGate(
                a1.mWires[j],
                a2.mWires[j],
                GateType::Xor,
                out.mWires[j]);
        }
    }


    void BetaLibrary::int_int_multiplex_build(
        BetaCircuit & cd,
        const BetaBundle & ifTrue,
        const BetaBundle & ifFalse,
        const BetaBundle & choice,
        const BetaBundle & out,
        const BetaBundle & temp)
    {
        // multiplex them together as (ifFalse ^ ifTrue) & s ^ ifFalse
        for (u64 i = 0; i < out.mWires.size(); ++i)
        {
            cd.addGate(ifFalse.mWires[i], ifTrue.mWires[i], GateType::Xor, temp.mWires[0]);
            //cd.addPrint("a^b  [" + std::to_string(i) + "] = ");
            //cd.addPrint(temp.mWires[0]);
            //cd.addPrint("\n");

            cd.addGate(temp.mWires[0], choice.mWires[0], GateType::And, temp.mWires[0]);

            //cd.addPrint("a^b&s[" + std::to_string(i) + "] = ");
            //cd.addPrint(temp.mWires[0]);
            //cd.addPrint("\n");

            cd.addGate(ifFalse.mWires[i], temp.mWires[0], GateType::Xor, out.mWires[i]);
        }
    }


    void reverse(span<BetaWire> bv)
    {
        u64 b = 0, e = bv.size() - 1;
        while (b < e)
        {
            auto  t = bv[e];
            bv[e] = bv[b];
            bv[b] = t;

            ++b;
            --e;
        }

    }


    void BetaLibrary::aes_sbox_build(BetaCircuit & cir, const BetaBundle & in, const BetaBundle & out)
    {
        for (u64 i = 0; i < in.size(); ++i)
            if (cir.mWireFlags[in.mWires[i]] == BetaWireFlag::Uninitialized)
                throw RTE_LOC;

        for (u64 i = 0; i < 16; ++i)
        {

            BetaBundle x; 
            x.mWires.insert(
                x.mWires.end(),
                in.mWires.rbegin() + i * 8,
                in.mWires.rbegin() + i * 8 + 8);

            BetaBundle s;
            s.mWires.insert(
                s.mWires.end(),
                out.mWires.rbegin() + i * 8,
                out.mWires.rbegin() + i * 8 + 8);
            //reverse(x.mWires);
            //reverse(s.mWires);

            BetaBundle y(22), t(68), z(18);

            cir.addTempWireBundle(y);
            cir.addTempWireBundle(t);
            cir.addTempWireBundle(z);

            // Jan 18 +  09
            // Straight-line program for AES sbox 
            // Joan Boyar and Rene Peralta

              // input is X0 + ..,X7  
              //output is S0 + ...,S7
              // arithmetic is over GF2

              // begin top linear transformation 
            cir.addGate(x[3], x[5], GateType::Xor, y[14]);//y14 = x3 + x5;
            cir.addGate(x[0], x[6], GateType::Xor, y[13]);//y13 = x0 + x6;
            cir.addGate(x[0], x[3], GateType::Xor, y[9]); //y9  = x0 + x3;
            cir.addGate(x[0], x[5], GateType::Xor, y[8]); //y8  = x0 + x5;
            cir.addGate(x[01], x[02], GateType::Xor, t[00]); //t00 = x01 + x02;
            cir.addGate(t[00], x[07], GateType::Xor, y[01]); //y01 = t00 + x07;
            cir.addGate(y[01], x[03], GateType::Xor, y[04]); //y04 = y01 + x03;
            cir.addGate(y[13], y[14], GateType::Xor, y[12]); //y12 = y13 + y14;
            cir.addGate(y[01], x[00], GateType::Xor, y[02]); //y02 = y01 + x00;
            cir.addGate(y[01], x[06], GateType::Xor, y[05]); //y05 = y01 + x06;
            cir.addGate(y[05], y[8], GateType::Xor, y[03]); //y03 = y05 + y 8;
            cir.addGate(x[04], y[12], GateType::Xor, t[01]); //t01 = x04 + y12;
            cir.addGate(t[01], x[05], GateType::Xor, y[15]); //y15 = t01 + x05;
            cir.addGate(t[01], x[01], GateType::Xor, y[20]); //y20 = t01 + x01;
            cir.addGate(y[15], x[07], GateType::Xor, y[06]); //y06 = y15 + x07;
            cir.addGate(y[15], t[00], GateType::Xor, y[10]); //y10 = y15 + t00;
            cir.addGate(y[20], y[9], GateType::Xor, y[11]); //y11 = y20 + y 9;
            cir.addGate(x[07], y[11], GateType::Xor, y[07]); //y07 = x07 + y11;
            cir.addGate(y[10], y[11], GateType::Xor, y[17]); //y17 = y10 + y11;
            cir.addGate(y[10], y[8], GateType::Xor, y[19]); //y19 = y10 + y 8;
            cir.addGate(t[00], y[11], GateType::Xor, y[16]); //y16 = t00 + y11;
            cir.addGate(y[13], y[16], GateType::Xor, y[21]); //y21 = y13 + y16;
            cir.addGate(x[00], y[16], GateType::Xor, y[18]); //y18 = x00 + y16;
            // end top linear transformation 
            cir.addGate(y[12], y[15], GateType::And, t[02]); //t02 = y12 X y15;
            cir.addGate(y[03], y[06], GateType::And, t[03]); //t03 = y03 X y06;
            cir.addGate(t[03], t[02], GateType::Xor, t[04]); //t04 = t03 + t02;
            cir.addGate(y[04], x[07], GateType::And, t[05]); //t05 = y04 X x07;
            cir.addGate(t[05], t[02], GateType::Xor, t[06]); //t06 = t05 + t02;
            cir.addGate(y[13], y[16], GateType::And, t[07]); //t07 = y13 X y16;
            cir.addGate(y[05], y[01], GateType::And, t[8]); //t 8 = y05 X y01;
            cir.addGate(t[8], t[07], GateType::Xor, t[9]); //t 9 = t 8 + t07;
            cir.addGate(y[02], y[07], GateType::And, t[10]); //t10 = y02 X y07;
            cir.addGate(t[10], t[07], GateType::Xor, t[11]); //t11 = t10 + t07;
            cir.addGate(y[9], y[11], GateType::And, t[12]); //t12 = y 9 X y11;
            cir.addGate(y[14], y[17], GateType::And, t[13]); //t13 = y14 X y17;
            cir.addGate(t[13], t[12], GateType::Xor, t[14]); //t14 = t13 + t12;
            cir.addGate(y[8], y[10], GateType::And, t[15]); //t15 = y 8 X y10;
            cir.addGate(t[15], t[12], GateType::Xor, t[16]); //t16 = t15 + t12;
            cir.addGate(t[04], t[14], GateType::Xor, t[17]); //t17 = t04 + t14;
            cir.addGate(t[06], t[16], GateType::Xor, t[18]); //t18 = t06 + t16;
            cir.addGate(t[9], t[14], GateType::Xor, t[19]); //t19 = t 9 + t14;
            cir.addGate(t[11], t[16], GateType::Xor, t[20]); //t20 = t11 + t16;
            cir.addGate(t[17], y[20], GateType::Xor, t[21]); //t21 = t17 + y20;
            cir.addGate(t[18], y[19], GateType::Xor, t[22]); //t22 = t18 + y19;
            cir.addGate(t[19], y[21], GateType::Xor, t[23]); //t23 = t19 + y21;
            cir.addGate(t[20], y[18], GateType::Xor, t[24]); //t24 = t20 + y18;
            // this next piece of the circuit is 
            // inversion in GF16, inputs are t21..24
            // and outputs are T37,T33,T40,T29.
            // Refer to paper for representation details
            // (tower field construction, normal basis (W,W^2) for extension   
            // from GF2 to GF4 and (Z^2,Z^8) for extension from GF4 to GF16).
            cir.addGate(t[21], t[22], GateType::Xor, t[25]);// t25 = t21 + t22;
            cir.addGate(t[21], t[23], GateType::And, t[26]);// t26 = t21 X t23;
            cir.addGate(t[24], t[26], GateType::Xor, t[27]);// t27 = t24 + t26;
            cir.addGate(t[25], t[27], GateType::And, t[28]);// t28 = t25 X t27;
            cir.addGate(t[28], t[22], GateType::Xor, t[29]);// t29 = t28 + t22;
            cir.addGate(t[23], t[24], GateType::Xor, t[30]);// t30 = t23 + t24;
            cir.addGate(t[22], t[26], GateType::Xor, t[31]);// t31 = t22 + t26;
            cir.addGate(t[31], t[30], GateType::And, t[32]);// t32 = t31 X t30;
            cir.addGate(t[32], t[24], GateType::Xor, t[33]);// t33 = t32 + t24;
            cir.addGate(t[23], t[33], GateType::Xor, t[34]);// t34 = t23 + t33;
            cir.addGate(t[27], t[33], GateType::Xor, t[35]);// t35 = t27 + t33;
            cir.addGate(t[24], t[35], GateType::And, t[36]);// t36 = t24 X t35;
            cir.addGate(t[36], t[34], GateType::Xor, t[37]);// t37 = t36 + t34;
            cir.addGate(t[27], t[36], GateType::Xor, t[38]);// t38 = t27 + t36;
            cir.addGate(t[29], t[38], GateType::And, t[39]);// t39 = t29 X t38;
            cir.addGate(t[25], t[39], GateType::Xor, t[40]);// t40 = t25 + t39;
            // end GF16 inversion
            cir.addGate(t[40], t[37], GateType::Xor, t[41]);// t41 = t40 + t37;
            cir.addGate(t[29], t[33], GateType::Xor, t[42]);// t42 = t29 + t33;
            cir.addGate(t[29], t[40], GateType::Xor, t[43]);// t43 = t29 + t40;
            cir.addGate(t[33], t[37], GateType::Xor, t[44]);// t44 = t33 + t37;
            cir.addGate(t[42], t[41], GateType::Xor, t[45]);// t45 = t42 + t41;
            cir.addGate(t[44], y[15], GateType::And, z[00]);// z00 = t44 X y15;
            cir.addGate(t[37], y[06], GateType::And, z[01]);// z01 = t37 X y06;
            cir.addGate(t[33], x[07], GateType::And, z[02]);// z02 = t33 X x07;
            cir.addGate(t[43], y[16], GateType::And, z[03]);// z03 = t43 X y16;
            cir.addGate(t[40], y[01], GateType::And, z[04]);// z04 = t40 X y01;
            cir.addGate(t[29], y[07], GateType::And, z[05]);// z05 = t29 X y07;
            cir.addGate(t[42], y[11], GateType::And, z[06]);// z06 = t42 X y11;
            cir.addGate(t[45], y[17], GateType::And, z[07]);// z07 = t45 X y17;
            cir.addGate(t[41], y[10], GateType::And, z[8]);// z 8 = t41 X y10;
            cir.addGate(t[44], y[12], GateType::And, z[9]);// z 9 = t44 X y12;
            cir.addGate(t[37], y[03], GateType::And, z[10]);// z10 = t37 X y03;
            cir.addGate(t[33], y[04], GateType::And, z[11]);// z11 = t33 X y04;
            cir.addGate(t[43], y[13], GateType::And, z[12]);// z12 = t43 X y13;
            cir.addGate(t[40], y[05], GateType::And, z[13]);// z13 = t40 X y05;
            cir.addGate(t[29], y[02], GateType::And, z[14]);// z14 = t29 X y02;
            cir.addGate(t[42], y[9], GateType::And, z[15]);// z15 = t42 X y 9;
            cir.addGate(t[45], y[14], GateType::And, z[16]);// z16 = t45 X y14;
            cir.addGate(t[41], y[8], GateType::And, z[17]);// z17 = t41 X y 8;
            // begin end linear transformation 
            cir.addGate(z[15], z[16], GateType::Xor, t[46]);// t46 = z15 +    z16;
            cir.addGate(z[10], z[11], GateType::Xor, t[47]);// t47 = z10 +    z11;
            cir.addGate(z[05], z[13], GateType::Xor, t[48]);// t48 = z05 +    z13;
            cir.addGate(z[9], z[10], GateType::Xor, t[49]);// t49 = z 9 +    z10;
            cir.addGate(z[02], z[12], GateType::Xor, t[50]);// t50 = z02 +    z12;
            cir.addGate(z[02], z[05], GateType::Xor, t[51]);// t51 = z02 +    z05;
            cir.addGate(z[07], z[8], GateType::Xor, t[52]);// t52 = z07 +    z 8;
            cir.addGate(z[00], z[03], GateType::Xor, t[53]);// t53 = z00 +    z03;
            cir.addGate(z[06], z[07], GateType::Xor, t[54]);// t54 = z06 +    z07;
            cir.addGate(z[16], z[17], GateType::Xor, t[55]);// t55 = z16 +    z17;
            cir.addGate(z[12], t[48], GateType::Xor, t[56]);// t56 = z12 +    t48;
            cir.addGate(t[50], t[53], GateType::Xor, t[57]);// t57 = t50 +    t53;
            cir.addGate(z[04], t[46], GateType::Xor, t[58]);// t58 = z04 +    t46;
            cir.addGate(z[03], t[54], GateType::Xor, t[59]);// t59 = z03 +    t54;
            cir.addGate(t[46], t[57], GateType::Xor, t[60]);// t60 = t46 +    t57;
            cir.addGate(z[14], t[57], GateType::Xor, t[61]);// t61 = z14 +    t57;
            cir.addGate(t[52], t[58], GateType::Xor, t[62]);// t62 = t52 +    t58;
            cir.addGate(t[49], t[58], GateType::Xor, t[63]);// t63 = t49 +    t58;
            cir.addGate(z[04], t[59], GateType::Xor, t[64]);// t64 = z04 +    t59;
            cir.addGate(t[61], t[62], GateType::Xor, t[65]);// t65 = t61 +    t62;
            cir.addGate(z[01], t[63], GateType::Xor, t[66]);// t66 = z01 +    t63;

            cir.addGate(t[59], t[63], GateType::Xor, s[00]);// s00 = t59 +    t63;
            cir.addGate(t[56], t[62], GateType::Nxor, s[06]);// s06 = t56 XNOR t62;
            cir.addGate(t[48], t[60], GateType::Nxor, s[07]);// s07 = t48 XNOR t60;
            cir.addGate(t[64], t[65], GateType::Xor, t[67]);// t67 = t64 +    t65;
            cir.addGate(t[53], t[66], GateType::Xor, s[03]);// s03 = t53 +    t66;
            cir.addGate(t[51], t[66], GateType::Xor, s[04]);// s04 = t51 +    t66;
            cir.addGate(t[47], t[65], GateType::Xor, s[05]);// s05 = t47 +    t65;
            cir.addGate(t[64], s[03], GateType::Nxor, s[01]);// s01 = t64 XNOR s03;
            cir.addGate(t[55], t[67], GateType::Nxor, s[02]);// s02 = t55 XNOR t67;
        }

        for (u64 i = 0; i < out.size(); ++i)
            if (cir.mWireFlags[out.mWires[i]] == BetaWireFlag::Uninitialized)
                throw RTE_LOC;



    }

    struct ByteView
    {
        ByteView() = default;

        ByteView(const BetaBundle& v)
        {
            init(v);
        }

        void init(const BetaBundle& v)
        {
            init(v.mWires.begin(), v.mWires.end());
        }
        void init(
            std::vector<BetaWire>::const_iterator b,
            std::vector<BetaWire>::const_iterator e)
        {
            if (e - b != 128)
                throw RTE_LOC;

            for (u64 i = 0; i < 16; ++i)
            {
                a[i].mWires.insert(
                    a[i].mWires.end(),
                    b + i * 8,
                    b + i * 8 + 8);
            }
        }

        std::array<BetaBundle, 16> a;
    };


    void inplace_shiftRows(BetaBundle & in)
    {


        ByteView buf(in);

        /*shift 1st row*/
        //cir.addCopy(buf.a[0], out.a[0]);
        //cir.addCopy(buf.a[4], out.a[4]);
        //cir.addCopy(buf.a[8], out.a[8]);
        //cir.addCopy(buf.a[12], out.a[12]);

        auto get = [](const BetaBundle& b, u64 i)
        {
            BetaBundle r;
            r.mWires.insert(
                r.mWires.end(),
                b.mWires.begin() + i * 8,
                b.mWires.begin() + i * 8 + 8);
            return r;
        };
        auto assign = [](BetaBundle& o, const BetaBundle& b,u64 i) {
            auto oiter = o.mWires.begin() + i * 8;
            auto biter = b.mWires.begin();
            for (u64 i = 0; i < 8; ++i)
                *oiter++ = *biter++;
        };

        /*shift 2nd row*/
        auto in13 = get(in, 13);     // in13 = in[13]
        assign(in, get(in,  1), 13); // in[13] = in[1];
        assign(in, get(in,  5),  1); // in[1] = in[5];
        assign(in, get(in,  9),  5); // in[5] = in[9];
        assign(in, in13,  9);        // in[9] = in13;

        /*shift 3rd row*/
        auto in10 = get(in, 10);    // in10 = in[10]
        auto in14 = get(in, 14);    // in14 = in[14]
        assign(in, get(in, 2), 10); // in[10] = in[2];
        assign(in, get(in, 6), 14); // in[14] = in[6];
        assign(in, in10, 2);        // in[2] = in10;
        assign(in, in14, 6);        // in[6] = in14;


        /*shift 4th row*/
        auto in3 = get(in, 3);      // in3    = in[3]
        assign(in, get(in, 15), 3); // in[3]  = in[15];
        assign(in, get(in, 11), 15);// in[15] = in[11];
        assign(in, get(in, 7), 11); // in[11] = in[7];
        assign(in, in3, 7);         // in[7]  = in3;

        //cir.addCopy(buf.a[15], out.a[3]);
        //cir.addCopy(buf.a[11], out.a[15]);
        //cir.addCopy(buf.a[7], out.a[11]);
        //cir.addCopy(buf.a[3], out.a[7]);



    }

    void BetaLibrary::aes_shiftRows_build(BetaCircuit & cir, const BetaBundle & in, const BetaBundle & o)
    {
        for (u64 i = 0; i < in.size(); ++i)
            if (cir.mWireFlags[in.mWires[i]] == BetaWireFlag::Uninitialized)
                throw RTE_LOC;


        ByteView buf(in);
        ByteView out(o);

        /*shift 1st row*/
        cir.addCopy(buf.a[0], out.a[0]);
        cir.addCopy(buf.a[4], out.a[4]);
        cir.addCopy(buf.a[8], out.a[8]);
        cir.addCopy(buf.a[12], out.a[12]);


        /*shift 2nd row*/
        cir.addCopy(buf.a[1], out.a[13]);
        cir.addCopy(buf.a[5], out.a[1]);
        cir.addCopy(buf.a[9], out.a[5]);
        cir.addCopy(buf.a[13], out.a[9]);

        /*shift 3rd row*/
        cir.addCopy(buf.a[2], out.a[10]);
        cir.addCopy(buf.a[6], out.a[14]);
        cir.addCopy(buf.a[10], out.a[2]);
        cir.addCopy(buf.a[14], out.a[6]);

        /*shift 4th row*/
        cir.addCopy(buf.a[15], out.a[3]);
        cir.addCopy(buf.a[11], out.a[15]);
        cir.addCopy(buf.a[7], out.a[11]);
        cir.addCopy(buf.a[3], out.a[7]);

        for (u64 i = 0; i < o.size(); ++i)
            if (cir.mWireFlags[o.mWires[i]] == BetaWireFlag::Uninitialized)
                throw RTE_LOC;


    }

    //void xor(BetaCircuit& cir, ByteView& in0)

    void BetaLibrary::aes_mixColumns_build(BetaCircuit & cir, const BetaBundle & in, const BetaBundle & o)
    {

        for (u64 i = 0; i < in.size(); ++i)
            if (cir.mWireFlags[in.mWires[i]] == BetaWireFlag::Uninitialized)
                throw RTE_LOC;


        BetaBundle a(8);
        std::array<BetaBundle, 4> b{ BetaBundle(8), BetaBundle(8), BetaBundle(8), BetaBundle(8) };
        //BetaBundle h(8);
        BetaWire h;

        ByteView buf(in);
        ByteView out(o);
        //print(cir, buf);

        cir.addTempWireBundle(a);
        cir.addTempWireBundle(b[0]);
        cir.addTempWireBundle(b[1]);
        cir.addTempWireBundle(b[2]);
        cir.addTempWireBundle(b[3]);
        //cir.addTempWireBundle(h);

        for (uint8_t i = 0; i < 4; i++) {

            for (uint8_t c = 0; c < 4; c++) {

                for (u64 j = 1; j < 8; ++j)
                    cir.addCopy(buf.a[4 * i + c][j - 1], b[c][j]);

                h = buf.a[4 * i + c][7];
                cir.addCopy(h, b[c][0]);
                cir.addGate(b[c][1], h, GateType::Xor, b[c][1]);
                cir.addGate(b[c][3], h, GateType::Xor, b[c][3]);
                cir.addGate(b[c][4], h, GateType::Xor, b[c][4]);

            }

            //a = buf.a[4 * i] ^ buf.a[4 * i + 1] ^ buf.a[4 * i + 2] ^ buf.a[4 * i + 3];
            int_int_bitwiseXor_build(cir, buf.a[4 * i], buf.a[4 * i + 1], a);
            int_int_bitwiseXor_build(cir, a, buf.a[4 * i + 2], a);
            int_int_bitwiseXor_build(cir, a, buf.a[4 * i + 3], a);

            //buf.a[4 * i] = b[0] ^ b[1] ^ a ^ buf.a[4 * i]; /* 2 * a0 + a3 + a2 + 3 * a1 */
            int_int_bitwiseXor_build(cir, buf.a[4 * i], b[0], out.a[4 * i]);
            int_int_bitwiseXor_build(cir, out.a[4 * i], b[1], out.a[4 * i]);
            int_int_bitwiseXor_build(cir, out.a[4 * i], a, out.a[4 * i]);


            //buf.a[4 * i + 1] = b[1] ^ b[2] ^ a ^ buf.a[4 * i + 1]; /* 2 * a1 + a0 + a3 + 3 * a2 */
            int_int_bitwiseXor_build(cir, buf.a[4 * i + 1], b[1], out.a[4 * i + 1]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 1], b[2], out.a[4 * i + 1]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 1], a, out.a[4 * i + 1]);

            //buf.a[4 * i + 2] = b[2] ^ b[3] ^ a ^ buf.a[4 * i + 2]; /* 2 * a2 + a1 + a0 + 3 * a3 */
            int_int_bitwiseXor_build(cir, buf.a[4 * i + 2], b[2], out.a[4 * i + 2]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 2], b[3], out.a[4 * i + 2]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 2], a, out.a[4 * i + 2]);

            //buf.a[4 * i + 3] = b[3] ^ b[0] ^ a ^ buf.a[4 * i + 3]; /* 2 * a3 + a2 + a1 + 3 * a0 */
            int_int_bitwiseXor_build(cir, buf.a[4 * i + 3], b[3], out.a[4 * i + 3]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 3], b[0], out.a[4 * i + 3]);
            int_int_bitwiseXor_build(cir, out.a[4 * i + 3], a, out.a[4 * i + 3]);
        }
    }



    void print(BetaCircuit& cir, BetaBundle& b)
    {
        for (u64 i = 0; i < 16; ++i)
        {
            if (i)
                cir.addPrint(".");

            BetaBundle a;
            a.mWires.insert(a.mWires.end(),
                b.mWires.begin() + i * 8,
                b.mWires.begin() + i * 8 + 8);

            cir.addPrint(a);
        }
        cir.addPrint("\n");
    }

    void BetaLibrary::aes_exapnded_build(
        BetaCircuit & cir,
        const BetaBundle & message,
        const BetaBundle & expandedKey,
        const BetaBundle & ciphertext)
    {

        auto Nr = expandedKey.size() / 128 - 1;
        std::vector<BetaBundle> keys(Nr + 1);

        for (u64 i = 0; i < Nr + 1; ++i)
        {
            keys[i].mWires.insert(
                keys[i].mWires.end(),
                expandedKey.mWires.begin() + 128 * i,
                expandedKey.mWires.begin() + 128 * i + 128);
        }

        BetaBundle state(128);
        cir.addTempWireBundle(state);

        int_int_bitwiseXor_build(cir, message, keys[0], state);



        for (u64 i = 1; i < Nr; i++)
        {
            aes_sbox_build(cir, state, state);
            inplace_shiftRows(state);
            aes_mixColumns_build(cir, state, state);
            int_int_bitwiseXor_build(cir, state, keys[i], state);
        }

        aes_sbox_build(cir, state, state);
        inplace_shiftRows(state);
        int_int_bitwiseXor_build(cir, state, keys[Nr], ciphertext);
    }

    bool BetaLibrary::areDistint(const BetaBundle & a1, const BetaBundle & a2)
    {
        for (u64 i = 0; i < a1.mWires.size(); ++i)
        {
            if (std::find(a2.mWires.begin(), a2.mWires.end(), a1.mWires[i]) != a2.mWires.end())
                return false;
        }
        return true;
    }
}
#endif
