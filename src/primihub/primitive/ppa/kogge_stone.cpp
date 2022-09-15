/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "src/primihub/primitive/ppa/kogge_stone.h"
#include <cmath>
#include <glog/logging.h>
#include <string>

namespace primihub {
void KoggeStoneLibrary::int_int_add_build_do(BetaCircuit *cd,
                                             const BetaBundle &a1,
                                             const BetaBundle &a2,
                                             const BetaBundle &sum,
                                             const BetaBundle &temps) {
  u64 size_a1 = a1.mWires.size();
  u64 size_a2 = a2.mWires.size();
  u64 size_sum = sum.mWires.size();

  if (size_a1 != size_a2 || size_a1 != size_sum)
    throw std::runtime_error("inputs must be same size " LOCATION);

  if (temps.mWires.size() < size_a1 * 2)
    throw std::runtime_error("need more temp bits " LOCATION);

  if (!areDistint(a2, sum) || !areDistint(a1, sum))
    throw std::runtime_error("must be distinct" LOCATION);

  BetaBundle PP, GG;
  auto &P = PP.mWires;
  auto &G = GG.mWires;

  P.insert(P.end(), temps.mWires.begin(), temps.mWires.begin() + size_a1);
  G.insert(G.end(), temps.mWires.begin() + size_a1,
           temps.mWires.begin() + size_a1 * 2 - 1);

  BetaWire temp_wire = temps.mWires.back();
  P[0] = sum.mWires[0];

  for (u64 i = 0; i < size_a1; ++i) {
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
    if (i < size_a1 - 1)
      cd->addGate(a1.mWires[i], a2.mWires[i], GateType::And, G[i]);
  }

  // Kogge-Stone algorithm.
  u64 d = log2ceil(size_a1);
  for (u64 level = 0; level < d; level++) {
    u64 start_pos = 1ull << level;
    u64 step = 1ull << level;
    for (u64 cur_wire = size_a1 - 1; cur_wire >= start_pos; --cur_wire) {
      u64 low_wire = cur_wire - step;
      BetaWire p0 = P[low_wire];
      BetaWire g0 = G[low_wire];
      BetaWire p1 = P[cur_wire];

      if (cur_wire < size_a1 - 1) {
        // G1 = G1 ^ P1 & G0.
        BetaWire g1 = G[cur_wire];
        cd->addGate(p1, g0, GateType::And, temp_wire);
        cd->addGate(temp_wire, g1, GateType::Xor, g1);
      }

      // If this is the first merge of this level, then we know the
      // propegate in is pointless since there is no global carry in.
      if (cur_wire != start_pos)
        // P1 = P1 & P0.
        cd->addGate(p0, p1, GateType::And, p1);
    }
  }

  for (u64 i = 1; i < size_a1; i++) {
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
    cd->addGate(P[i], G[i - 1], GateType::Xor, sum.mWires[i]);
  }

  return;
}

void KoggeStoneLibrary::int_int_add_msb_build_do(BetaCircuit *cd,
                                                 const BetaBundle &a1,
                                                 const BetaBundle &a2,
                                                 const BetaBundle &msb,
                                                 const BetaBundle &temps) {
  u64 size_a1 = a1.mWires.size();
  u64 size_a2 = a2.mWires.size();
  u64 size_msb = msb.mWires.size();

  if (size_a1 != size_a2)
    throw std::runtime_error("input must be the same size " LOCATION);

  if (size_msb != 1)
    throw std::runtime_error("size of msb must be 1 " LOCATION);

  if (temps.mWires.size() < size_a1 * 2)
    throw std::runtime_error("more temp bits needed " LOCATION);

  BetaBundle PP;
  BetaBundle GG;
  std::vector<BetaWire> &P = PP.mWires;
  std::vector<BetaWire> &G = GG.mWires;
  BetaWire temp_wire = temps.mWires.back();

  P.insert(P.end(), temps.mWires.begin(), temps.mWires.begin() + size_a1);
  G.insert(G.end(), temps.mWires.begin() + size_a1,
           temps.mWires.begin() + size_a1 * 2 - 1);

  if (size_a1 == 1)
    P[0] = msb.mWires[0];

  // MSB = P[i] ^ G[i-1], so only build circuit for G[0...i-1] and P[1...i-1]
  // in kogge-stone algorithm.
  for (u64 i = 0; i < size_a1 - 1; i++) {
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::And, G[i]);
  }

  u64 d = log2ceil(size_a1);
  for (u64 level = 0; level < d - 1; level++) {
    u64 start_pos = 1 << level;
    u64 step = 1 << level;

    for (u64 i = size_a1 - 2; i >= start_pos; i--) {
      u64 cur_wire = i;
      u64 low_wire = i - step;
      if (cur_wire != low_wire) {
        BetaWire p0 = P[low_wire];
        BetaWire g0 = G[low_wire];
        BetaWire p1 = P[cur_wire];
        BetaWire g1 = G[cur_wire];

        // g1 = g1 ^ p1 & g0
        cd->addGate(p1, g0, GateType::And, temp_wire);
        cd->addGate(temp_wire, g1, GateType::Xor, g1);

        // p1 = p1 & p0
        cd->addGate(p0, p1, GateType::And, p1);
      }
    }
  }

  {
    u64 step = 1 << (d - 1);
    u64 cur_wire = size_a1 - 2;
    u64 low_wire = cur_wire - step;

    BetaWire p0 = P[low_wire];
    BetaWire g0 = G[low_wire];
    BetaWire p1 = P[cur_wire];
    BetaWire g1 = G[cur_wire];

    // g1 = g1 ^ p1 & g0.
    cd->addGate(p1, g0, GateType::And, temp_wire);
    cd->addGate(temp_wire, g1, GateType::Xor, g1);

    // p1 = p1 & p0.
    cd->addGate(p0, p1, GateType::And, p1);
  }

  // MSB = P[i] ^ G[i-1].
  cd->addGate(a1.mWires.back(), a2.mWires.back(), GateType::Xor, P.back());
  cd->addGate(P.back(), G.back(), GateType::Xor, msb.mWires.back());

  return;
}

u64 KoggeStoneLibrary::find_node_level(u64 total_level, BetaWire wire) {
  for (u64 i = 0; i < total_level; i++) {
    u64 start_pos = 1 << i;
    if (start_pos > wire) {
      if (!i)
        return i;
      else
        return i - 1;
    }
  }

  return (u64)-1;
}

// Assume that bit length of input is n, MSB = P[n-1] ^ C[n-2], P[n] is
// a1[n-1] ^ a2[n-1], so the problem is how to get C[n-2]. The circuit
// built by kogge-stone algorithm output C[0...n-1], but only want C[n-2].
// Have a look at all dependent node of C[n-2] in the circuit no matter
// directly or indirectly, the C[n-2] and it's dependent looks like a binary
// tree and each node in circuit is a node in binary tree. The way of finding
// all nodes in binary tree then build circuit for these node will get a tiny
// kogge-stone circuit to get C[n-2].
void KoggeStoneLibrary::int_int_add_msb_build_optimized(
    BetaCircuit *cd, const BetaBundle &a1, const BetaBundle &a2,
    const BetaBundle &msb, const BetaBundle &temps) {
  u64 size_a1 = a1.mWires.size();
  u64 size_a2 = a2.mWires.size();
  u64 size_msb = msb.mWires.size();

  if (size_a1 != size_a2)
    throw std::runtime_error("input must be the same size " LOCATION);

  if (size_msb != 1)
    throw std::runtime_error("size of msb must be 1 " LOCATION);

  if (temps.mWires.size() < size_a1 * 2)
    throw std::runtime_error("more temp bits needed " LOCATION);

  u64 d = log2ceil(size_a1);
  node_by_depth_.resize(d);

  VLOG(3) << "Depth of circuit is " << d;
  VLOG(3) << "Size of input is " << size_a1;

  // Resolve the dependent of C[i-1] bit in circuit built by Kogge-Stone
  // algorithm.
  u64 step = 1 << (d - 1);
  BetaWire c_bit = size_a1 - 2;

  u64 start_pos = 1 << (d - 1);
  if (start_pos > c_bit && d != 1) {
    d -= 1;
    step = 1 << (d - 1);
  }

  {
    NodeDesc tmp(c_bit, c_bit, c_bit - step, d - 1);
    node_by_depth_[d - 1].insert(tmp);
    VLOG(3) << "Insert " << tmp.ToString() << " into level " << d - 1
            << ", step " << step;
  }

  for (u64 level = d - 1; level >= 1; level--) {
    for (auto desc : node_by_depth_[level]) {
      u64 input1 = desc.deps_[0];
      u64 input2 = desc.deps_[1];

      u64 prev_level = level - 1;
      u64 step = 1 << prev_level;
      u64 start_pos = 1 << prev_level;

      std::set<NodeDesc> &level_node = node_by_depth_[prev_level];
      NodeDesc tmp(input1, input1, input1 - step, prev_level);
      level_node.insert(tmp);

      // VLOG(3) << "Insert " << tmp.ToString() << " into level " << prev_level
      //         << ", step " << step;

      if (desc.node_ != 0) {
        if (start_pos > input2) {
          u64 node_level = find_node_level(d, input2);
          u64 step = 1 << node_level;

          std::set<NodeDesc> &level_node = node_by_depth_[node_level];
          NodeDesc tmp(input2, input2, input2 - step, node_level);
          level_node.insert(tmp);

          VLOG(3) << "Insert " << tmp.ToString() << " into level " << node_level
                  << ", step " << step;
        } else {
          NodeDesc tmp(input2, input2, input2 - step, prev_level);
          level_node.insert(tmp);

          // VLOG(3) << "Insert " << tmp.ToString() << " into level " <<
          // prev_level
          //         << ", step " << step;
        }
      }
    }
  }

  if (VLOG_IS_ON(5)) {
    for (i64 level = static_cast<i64>(d - 1); level >= 0; level--) {
      VLOG(5) << "----Level " << level << "-----";
      for (auto desc : node_by_depth_[level])
        VLOG(5) << "NodeDesc:" << desc.ToString() << ".";
    }
  }

  // Build circuit according to dependent resolved just now.
  BetaBundle PP;
  BetaBundle GG;
  std::vector<BetaWire> &P = PP.mWires;
  std::vector<BetaWire> &G = GG.mWires;
  BetaWire temp_wire = temps.mWires.back();

  P.insert(P.end(), temps.mWires.begin(), temps.mWires.begin() + size_a1);
  G.insert(G.end(), temps.mWires.begin() + size_a1,
           temps.mWires.begin() + size_a1 * 2 - 1);

  if (size_a1 == 1)
    P[0] = msb.mWires[0];

  // MSB = P[i] ^ G[i-1], so only build circuit for G[0...i-1] and P[1...i-1]
  // in kogge-stone algorithm.
  for (u64 i = 0; i < size_a1 - 1; i++) {
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::Xor, P[i]);
    cd->addGate(a1.mWires[i], a2.mWires[i], GateType::And, G[i]);
  }

  std::set<NodeDesc> &level_node = node_by_depth_[0];
  for (const NodeDesc &desc : level_node) {
    if (desc.node_ == 0)
      break;
    u64 low_wire = desc.deps_[1];
    u64 cur_wire = desc.deps_[0];

    BetaWire p0 = P[low_wire];
    BetaWire g0 = G[low_wire];
    BetaWire p1 = P[cur_wire];
    BetaWire g1 = G[cur_wire];

    // g1 = g1 ^ p1 & g0
    cd->addGate(p1, g0, GateType::And, temp_wire);
    cd->addGate(temp_wire, g1, GateType::Xor, g1);

    // p1 = p1 & p0
    cd->addGate(p0, p1, GateType::And, p1);
  }

  for (size_t level = 1; level < node_by_depth_.size(); level++) {
    std::set<NodeDesc> &level_node = node_by_depth_[level];
    for (const NodeDesc &desc : level_node) {
      u64 low_wire = desc.deps_[1];
      u64 cur_wire = desc.deps_[0];

      BetaWire p0 = P[low_wire];
      BetaWire g0 = G[low_wire];
      BetaWire p1 = P[cur_wire];
      BetaWire g1 = G[cur_wire];

      // g1 = g1 ^ p1 & g0
      cd->addGate(p1, g0, GateType::And, temp_wire);
      cd->addGate(temp_wire, g1, GateType::Xor, g1);

      // p1 = p1 & p0
      cd->addGate(p0, p1, GateType::And, p1);
    }
  }

  // MSB = P[i] ^ G[i-1].
  cd->addGate(a1.mWires.back(), a2.mWires.back(), GateType::Xor, P.back());
  cd->addGate(P.back(), G.back(), GateType::Xor, msb.mWires.back());

  node_by_depth_.clear();
  return;
}

BetaCircuit *KoggeStoneLibrary::int_int_add(u64 a_size, u64 b_size,
                                            u64 c_size) {
  std::string key = "kogge_stone_ppa_add" + std::to_string(0) + "_" +
                    std::to_string(a_size) + "x" + std::to_string(b_size) +
                    "x" + std::to_string(c_size);

  std::unordered_map<std::string, BetaCircuit *>::iterator iter =
      mCirMap.find(key);
  if (iter != mCirMap.end())
    return iter->second;

  BetaCircuit *circuit = new BetaCircuit();
  BetaBundle a(a_size);
  BetaBundle b(b_size);
  BetaBundle c(c_size);

  circuit->addInputBundle(a);
  circuit->addInputBundle(b);
  circuit->addOutputBundle(c);

  BetaBundle t(a_size * 2);
  circuit->addTempWireBundle(t);
  int_int_add_build_do(circuit, a, b, c, t);

  return circuit;
}

BetaCircuit *KoggeStoneLibrary::int_int_add_msb(u64 size) {
  std::string key = "kogge_stone_ppa_add_msb" + std::to_string(0) + "_" +
                    std::to_string(size) + "x" + std::to_string(size) + "x" +
                    std::to_string(size);

  std::unordered_map<std::string, BetaCircuit *>::iterator iter =
      mCirMap.find(key);
  if (iter != mCirMap.end())
    return iter->second;

  BetaCircuit *circuit = new BetaCircuit();
  BetaBundle a(size);
  BetaBundle b(size);
  BetaBundle msb(1);

  circuit->addInputBundle(a);
  circuit->addInputBundle(b);
  circuit->addOutputBundle(msb);

  BetaBundle t(size * 2);
  circuit->addTempWireBundle(t);
  int_int_add_msb_build_optimized(circuit, a, b, msb, t);

  return circuit;
}

} // End namespace primihub.
