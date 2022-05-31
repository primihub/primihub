
#ifndef SRC_primihub_PRIMITIVE_CIRCUIT_BETA_LIBRARY_H_
#define SRC_primihub_PRIMITIVE_CIRCUIT_BETA_LIBRARY_H_

#include <cstring>
#include <string>

#include "src/primihub/common/defines.h"
#ifdef ENABLE_CIRCUITS

#include <unordered_map>
#include "src/primihub/primitive/circuit/beta_circuit.h"
#include "src/primihub/util/crypto/bit_vector.h"

namespace primihub {
class BetaLibrary {
 public:
  BetaLibrary();
  ~BetaLibrary();

 enum class Optimized {
  Size,
  Depth
};

std::unordered_map<std::string, BetaCircuit*> mCirMap;

BetaCircuit* int_int_add(u64 aSize, u64 bSize, u64 cSize,
  Optimized op = Optimized::Size);

BetaCircuit* int_int_add_msb(u64 aSize);

BetaCircuit* uint_uint_add(u64 aSize, u64 bSize, u64 cSize);
BetaCircuit* int_intConst_add(u64 aSize, u64 bSize, i64 bVal, u64 cSize);
BetaCircuit* int_int_subtract(u64 aSize, u64 bSize, u64 cSize);
BetaCircuit* int_int_sub_msb(u64 aSize, u64 bSize, u64 cSize);
BetaCircuit* uint_uint_subtract(u64 aSize, u64 bSize, u64 cSize);

BetaCircuit* int_intConst_subtract(u64 aSize, u64 bSize, i64 bVal, u64 cSize);
BetaCircuit* int_int_mult(u64 aSize, u64 bSize, u64 cSize,
  Optimized op = Optimized::Size);
BetaCircuit* int_int_div(u64 aSize, u64 bSize, u64 cSize);

BetaCircuit* int_eq(u64 aSize);
BetaCircuit* int_neq(u64 aSize);

BetaCircuit* int_int_lt(u64 aSize, u64 bSize);
BetaCircuit* int_int_gteq(u64 aSize, u64 bSize);

BetaCircuit* uint_uint_lt(u64 aSize, u64 bSize);
BetaCircuit* uint_uint_gteq(u64 aSize, u64 bSize);
BetaCircuit* uint_uint_mult(u64 aSize, u64 bSize, u64 cSize,
  Optimized op = Optimized::Size);

BetaCircuit* int_int_multiplex(u64 aSize);

BetaCircuit* int_removeSign(u64 aSize);
BetaCircuit* int_addSign(u64 aSize);
BetaCircuit* int_negate(u64 aSize);
BetaCircuit* int_isZero(u64 aSize);

BetaCircuit* int_bitInvert(u64 aSize);
BetaCircuit* int_int_bitwiseAnd(u64 aSize, u64 bSize, u64 cSize);
BetaCircuit* int_int_bitwiseOr(u64 aSize, u64 bSize, u64 cSize);
BetaCircuit* int_int_bitwiseXor(u64 aSize, u64 bSize, u64 cSize);

BetaCircuit* aes_exapnded(u64 rounds);

static void int_int_add_build_so(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & sum,
    const BetaBundle & temps);

static void int_int_add_build_do(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & sum,
    const BetaBundle & temps);

static void int_int_add_build_do(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & sum) {
    BetaBundle  temps(a1.size() * 2);
    cd.addTempWireBundle(temps);
    int_int_add_build_do(cd, a1, a2, sum, temps);
}

static void int_int_add_msb_build_do(
  BetaCircuit& cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & sum,
  const BetaBundle & temps);

static void uint_uint_add_build(
  BetaCircuit& cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & sum,
  const BetaBundle & temps);

static void int_int_subtract_build(
  BetaCircuit& cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & diff,
  const BetaBundle & temps);

static void int_int_sub_msb_build_do(
    BetaCircuit& cd,
    const BetaBundle& a1,
    const BetaBundle& a2,
    const BetaBundle& diff,
    const BetaBundle& temps);

static void uint_uint_subtract_build(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & diff,
    const BetaBundle & temps);

static void int_int_mult_build(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & prod,
    Optimized o,
    bool useTwoComplement);

static void int_int_div_rem_build(
    BetaCircuit& cd,
    const BetaBundle& a1,
    const BetaBundle& a2,
    const BetaBundle& quot,
    const BetaBundle& rem
);

static void uint_uint_div_rem_build(
    BetaCircuit& cd,
    const BetaBundle& a1,
    const BetaBundle& a2,
    const BetaBundle& quot,
    const BetaBundle& rem
);

static void int_isZero_build(
    BetaCircuit& cd,
    BetaBundle & a1,
    BetaBundle & out);

static void int_int_eq_build(
    BetaCircuit& cd,
    BetaBundle & a1,
    BetaBundle & a2,
    BetaBundle & out);

static void int_int_lt_build(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & out);

static void int_int_gteq_build(
    BetaCircuit& cd,
    const BetaBundle & a1,
    const BetaBundle & a2,
    const BetaBundle & out);

static void uint_uint_lt_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out);

static void uint_uint_gteq_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out);

static void int_removeSign_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & out,
        const BetaBundle & temp);

static void int_addSign_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & sign,
        const BetaBundle & out,
        const BetaBundle & temp);

static void int_bitInvert_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & out);

static void int_negate_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & out,
        const BetaBundle & temp);

static void int_int_bitwiseAnd_build(
        BetaCircuit& cd,
        const BetaBundle & a1,
        const BetaBundle & a2,
        const BetaBundle & out);

static void int_int_bitwiseOr_build(
  BetaCircuit& cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & out);

static void int_int_bitwiseXor_build(
  BetaCircuit& cd,
  const BetaBundle & a1,
  const BetaBundle & a2,
  const BetaBundle & out);

// if choice = 1, the take the first parameter (ifTrue). Otherwise take the second parameter (ifFalse).
static void int_int_multiplex_build(
        BetaCircuit& cd,
        const BetaBundle & ifTrue,
        const BetaBundle & ifFalse,
        const BetaBundle & choice,
        const BetaBundle & out,
        const BetaBundle & temp);

static void aes_sbox_build(
        BetaCircuit& cd,
        const BetaBundle & in,
        const BetaBundle & out);

static void aes_shiftRows_build(
        BetaCircuit& cd,
        const BetaBundle & in,
        const BetaBundle & out);

static void aes_mixColumns_build(
        BetaCircuit& cd,
        const BetaBundle & in,
        const BetaBundle & out);

static void aes_exapnded_build(
        BetaCircuit& cd,
        const BetaBundle & message,
        const BetaBundle & expandedKey,
        const BetaBundle & ciphertext);

static bool areDistint(const BetaBundle& a1, const BetaBundle& a2);
};

}  // namepsace primihub
#endif

#endif  // SRC_primihub_PRIMITIVE_CIRCUIT_BETA_LIBRARY_H_
