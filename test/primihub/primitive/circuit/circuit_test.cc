// Copyright [2021] <primihub.com>

#include <fstream>
#include <random>

#include "gtest/gtest.h"

#include <glog/logging.h>
#include "src/primihub/primitive/circuit/beta_library.h"
#include "src/primihub/primitive/circuit/circuit_library.h"
#include "src/primihub/primitive/circuit/garble.h"
#include "src/primihub/primitive/ppa/kogge_stone.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/log.h"

using namespace primihub;

TEST(CircuitTest, BetaCircuit_int_Sh3Piecewise_Test) {
    CircuitLibrary lib;
    PRNG prng(ZeroBlock);
    u64 tries = 200;

    u64 numThresholds = 3;
    u64 decimal = 16;
    u64 size = 64;

    auto* cir = lib.int_Sh3Piecewise_helper(size, numThresholds);

    std::vector<double> dd;
    std::normal_distribution<> d(0.0, 3);

    for (u64 i = 0; i < tries; ++i) {
        std::vector<BitVector> in(numThresholds + 1), out(numThresholds + 1);

        dd.clear();
        for (u64 t = 0; t < numThresholds; ++t) dd.emplace_back(d(prng));

        std::sort(dd.rbegin(), dd.rend());

        i64 b = prng.get<i64>();
        u64 exp = numThresholds;

        for (u64 t = 0; t < numThresholds; ++t) {
            if (exp == numThresholds && dd[t] <= 0) exp = t;

            i64 val = static_cast<i64>(dd[t] * (1 << decimal));

            val -= b;

            in[t].append((u8*)&val, size);
        }

        in[numThresholds].append((u8*)&b, size);

        for (auto& o : out) o.resize(1);

        cir->evaluate(in, out);

        for (u64 t = 0; t < numThresholds + 1; ++t) {
            if (out[t][0] != (t == exp)) {
                throw std::runtime_error(LOCATION);
            }
        }
    }
}

TEST(CircuitTest, garble_Test) {
    CircuitLibrary lib;
    u64 bitCount = 64;
    auto cir = lib.int_int_mult(bitCount, bitCount, bitCount);

    Garble garb;

    uint64_t s = 0;
    PRNG prng(toBlock(s));
    std::vector<block> zeroWireLabels(cir->mWireCount);

    // set the free xor key. Bottom bit must be 1.
    block freeXorOffset = prng.get<block>() | OneBlock;

    block gTweak = ZeroBlock;

    // randomly pick the zero labels for the inputs
    for (auto in : cir->mInputs) {
        for (auto i : in.mWires) {
            zeroWireLabels[i] = prng.get();
        }
    }

    // pick the garbler's input values, in this case at random.
    BitVector garbPlainInput(bitCount);
    garbPlainInput.randomize(prng);

    // construct the garbler's garbled input
    std::vector<block> garblersInput(cir->mInputs[0].size());
    for (u64 i = 0; i < cir->mInputs[0].mWires.size(); ++i) {
        garblersInput[i] = zeroWireLabels[cir->mInputs[0].mWires[i]];
        if (garbPlainInput[i])
            garblersInput[i] = garblersInput[i] ^ freeXorOffset;
    }

    std::vector<std::array<block, 2>> evalsLabels(cir->mInputs[1].size());
    for (u64 i = 0; i < cir->mInputs[1].mWires.size(); ++i) {
        evalsLabels[i][0] = zeroWireLabels[cir->mInputs[1].mWires[i]];
        evalsLabels[i][1] =
            zeroWireLabels[cir->mInputs[1].mWires[i]] ^ freeXorOffset;
    }

    // the garbled circuit.
    std::vector<GarbledGate<2>> garbledGates(cir->mNonlinearGateCount);

#ifdef GARBLE_DEBUG
    std::vector<block> G_DEBUG_LABELS(cir->mGates.size());
    garb.garble(*cir, zeroWireLabels, garbledGates, gTweak, freeXorOffset,
                G_DEBUG_LABELS);
#else
    garb.garble(*cir, zeroWireLabels, garbledGates, gTweak, freeXorOffset);
#endif

    std::cout << zeroWireLabels[0] << std::endl;

    // optionally construct the decoding information.
    BitVector decoding(bitCount);
    for (u64 i = 0; i < cir->mOutputs[0].size(); ++i) {
        decoding[i] = PermuteBit(zeroWireLabels[cir->mOutputs[0].mWires[i]]);
    }

    Garble eval;

    std::vector<block> activeWireLabels(cir->mWireCount);

    // copy in the garbler's input labels.
    for (u64 i = 0; i < cir->mInputs[0].mWires.size(); ++i) {
        activeWireLabels[cir->mInputs[0].mWires[i]] = garblersInput[i];
    }

    // pick some random inputs for the evaluator, in this case at random.
    BitVector evalsPlainInput(bitCount);
    evalsPlainInput.randomize(prng);

    // perform OT to get the evalutor's input. This is fakes.
    std::vector<block> evalsInput(cir->mInputs[1].size());
    for (u64 i = 0; i < cir->mInputs[1].mWires.size(); ++i) {
        activeWireLabels[cir->mInputs[1].mWires[i]] =
            evalsLabels[i][evalsPlainInput[i]];
    }

    // evaluator also needs to set their own tweak.
    block eTweak = ZeroBlock;

#ifdef GARBLE_DEBUG
    std::vector<block> E_DEBUG_LABELS(cir->mGates.size());
    garb.evaluate(*cir, activeWireLabels, garbledGates, eTweak, E_DEBUG_LABELS);
#else
    garb.evaluate(*cir, activeWireLabels, garbledGates, eTweak);
#endif

    // decode the output.
    BitVector output(bitCount);
    for (u64 i = 0; i < cir->mOutputs[0].size(); ++i) {
        output[i] = decoding[i] ^
                    PermuteBit(activeWireLabels[cir->mOutputs[0].mWires[i]]);
    }

#ifdef GARBLE_DEBUG
    for (u64 i = 0; i < E_DEBUG_LABELS.size(); ++i) {
        if (E_DEBUG_LABELS[i] != G_DEBUG_LABELS[i] &&
            E_DEBUG_LABELS[i] != (G_DEBUG_LABELS[i] ^ freeXorOffset)) {
            throw std::runtime_error(LOCATION);
        }
    }
#endif

    std::vector<BitVector> plainInputs{garbPlainInput, evalsPlainInput};
    std::vector<BitVector> plainOutputs(1);
    plainOutputs[0].resize(bitCount);

    cir->evaluate(plainInputs, plainOutputs);

    if (plainOutputs[0] != output) {
        std::cout << "exp " << plainOutputs[0] << std::endl;
        std::cout << "act " << output << std::endl;
        throw std::runtime_error(LOCATION);
    }
    // TODO(liyankai) : update
    EXPECT_EQ(1, 1);
}

i64 signExtend(i64 v, u64 b, bool print = false) {
    if (b != 64) {
        i64 loc = (i64(1) << (b - 1));
        i64 sign = v & loc;

        if (sign) {
            i64 mask = i64(-1) << (b);
            auto ret = v | mask;
            if (print) {
                std::cout << "sign: " << BitVector((u8*)&sign, 64) << std::endl;
                std::cout << "mask: " << BitVector((u8*)&mask, 64) << std::endl;
                std::cout << "v   : " << BitVector((u8*)&v, 64) << std::endl;
                std::cout << "ret : " << BitVector((u8*)&ret, 64) << std::endl;
            }
            return ret;
        } else {
            i64 mask = (i64(1) << b) - 1;
            auto ret = v & mask;
            if (print) {
                std::cout << "sign: " << BitVector((u8*)&loc, 64) << std::endl;
                std::cout << "mask: " << BitVector((u8*)&mask, 64) << std::endl;
                std::cout << "v   : " << BitVector((u8*)&v, 64) << std::endl;
                std::cout << "ret : " << BitVector((u8*)&ret, 64) << std::endl;
            }
            return ret;
        }
    }

    return v;
}

u64 mask(u64 v, u64 b) { return v & ((1ull << b) - 1); }

TEST(BetaCircuit_int_Adder_Test, int_adder) {
    setThreadName("CP_Test_Thread");
    BetaLibrary lib;

    PRNG prng(OneBlock);
    u64 tries = 2;

    u64 size = 57;

    for (u64 i = 0; i < tries; ++i) {
        size = (prng.get<u64>() % 63) + 2;
        i64 a = signExtend(prng.get<i64>(), size);
        i64 b = signExtend(prng.get<i64>(), size);
        i64 c = signExtend((a + b), size);

        auto* msb = lib.int_int_add_msb(size);
        auto* cir1 =
            lib.int_int_add(size, size, size, BetaLibrary::Optimized::Depth);
        auto* cir2 =
            lib.int_int_add(size, size, size, BetaLibrary::Optimized::Size);
        auto* cir3 =
            lib.int_int_add(size, size, size, BetaLibrary::Optimized::Size);

        // msb->levelByAndDepth();
        cir1->levelByAndDepth();
        cir3->levelByAndDepth(BetaCircuit::LevelizeType::NoReorder);

        std::vector<BitVector> inputs(2), output1(1), output2(1), output3(1),
            output4(1);
        inputs[0].append((u8*)&a, size);
        inputs[1].append((u8*)&b, size);
        output1[0].resize(size);
        output2[0].resize(1);
        output3[0].resize(size);
        output4[0].resize(size);

        cir1->evaluate(inputs, output1);
        cir2->evaluate(inputs, output3);
        cir3->evaluate(inputs, output4);
        msb->evaluate(inputs, output2);
        // std::cout << "msb " << size << "  -> " << msb->mNonlinearGateCount /
        // double(size) << std::endl;

        i64 cc = 0;
        memcpy(&cc, output1[0].data(), output1[0].sizeBytes());

        cc = signExtend(cc, size);
        if (cc != c) {
            std::cout << "i " << i << std::endl;

            BitVector cExp;
            cExp.append((u8*)&c, size);
            std::cout << "a  : " << inputs[0] << std::endl;
            std::cout << "b  : " << inputs[1] << std::endl;
            std::cout << "exp: " << cExp << std::endl;
            std::cout << "act: " << output1[0] << std::endl;

            throw std::runtime_error(LOCATION);
        }

        if (output2.back().back() != output1.back().back()) {
            std::cout << "exp: " << output1.back().back() << std::endl;
            std::cout << "act: " << output2.back().back() << std::endl;
            throw std::runtime_error(LOCATION);
        }

        if (output3.back().back() != output1.back().back()) {
            std::cout << "exp: " << output1.back().back() << std::endl;
            std::cout << "act: " << output3.back().back() << std::endl;
            throw std::runtime_error(LOCATION);
        }
        if (output4.back().back() != output1.back().back()) {
            std::cout << "exp: " << output1.back().back() << std::endl;
            std::cout << "act: " << output4.back().back() << std::endl;
            throw std::runtime_error(LOCATION);
        }
    }
}

TEST(KoggeStone_int_Adder_test, KoggeStone_int_adder) {
    setThreadName("CP_Test_Thread");
    google::InitGoogleLogging("msb_test");

    u64 size = 0;
    KoggeStoneLibrary library;
    PRNG prng(ZeroBlock);

    u64 num_circ_tries = 1000;
    for (u64 i = 0; i < num_circ_tries; i++) {
        size = (prng.get<u64>() % 63) + 2;
        BetaCircuit* circ = library.int_int_add(size, size, size);
        BetaCircuit* msb = library.int_int_add_msb(size);
        circ->levelByAndDepth();
        msb->levelByAndDepth();
        
        u64 num_tries = 4;
        for (u64 i = 0; i < num_tries; i++) {
            i64 v_a = prng.get<i64>();
            i64 v_b = prng.get<i64>();
            i64 a = signExtend(v_a, size, false);
            i64 b = signExtend(v_b, size, false);
            i64 c = signExtend((a + b), size);

            std::vector<BitVector> inputs(2);
            std::vector<BitVector> output(1);
            std::vector<BitVector> output1(1);

            inputs[0].append((u8*)&a, size);
            inputs[1].append((u8*)&b, size);
            output[0].resize(size);
            output1[0].resize(1);

            circ->evaluate(inputs, output);
            msb->evaluate(inputs, output1);

            i64 cc = 0;
            memcpy(&cc, output[0].data(), output[0].sizeBytes());

            cc = signExtend(cc, size);
            if (cc != c) {
                BitVector cExp;
                cExp.append((u8*)&c, size);
                std::cout << "a  : " << inputs[0] << std::endl;
                std::cout << "b  : " << inputs[1] << std::endl;
                std::cout << "exp: " << cExp << std::endl;
                std::cout << "act: " << output[0] << std::endl;

                throw std::runtime_error(
                    "Kogge-Stone circuit gives wrong add result.");
            }

            if (output.back().back() != output1.back().back()) {
                BitVector cExp;
                cExp.append((u8*)&c, size);
                std::cout << "v_a: " << v_a << std::endl;
                std::cout << "v_b: " << v_b << std::endl;
                std::cout << "a  : " << inputs[0] << std::endl;
                std::cout << "b  : " << inputs[1] << std::endl;
                std::cout << "c  : " << output[0] << std::endl;
                std::cout << "exp: " << output.back().back() << std::endl;
                std::cout << "act: " << output1.back().back() << std::endl;

                throw std::runtime_error(
                    "Kogge-Stone circuit gives wrong add msb result.");
            }
        }
    }
}
