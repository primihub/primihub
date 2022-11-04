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

#ifndef __KOGGE_STONE_H_
#define __KOGGE_STONE_H_

#include <algorithm>
#include <iostream>
#include <string>

#include "src/primihub/primitive/circuit/beta_circuit.h"
#include "src/primihub/primitive/circuit/beta_library.h"

namespace primihub {
class KoggeStoneLibrary : public BetaLibrary {
public:
  BetaCircuit *int_int_add(u64 a_size, u64 b_size, u64 c_size);
  BetaCircuit *int_int_add_msb(u64 size);

  void int_int_add_build_do(BetaCircuit *cd, const BetaBundle &a1,
                            const BetaBundle &a2, const BetaBundle &sum,
                            const BetaBundle &tmps);

  void int_int_add_msb_build_do(BetaCircuit *cd, const BetaBundle &a1,
                                const BetaBundle &a2, const BetaBundle &msb,
                                const BetaBundle &temps);

  void int_int_add_msb_build_optimized(BetaCircuit *cd, const BetaBundle &a1,
                                       const BetaBundle &a2,
                                       const BetaBundle &msb,
                                       const BetaBundle &temps);

private:
  u64 find_node_level(u64 total_level, BetaWire wire);

  class NodeDesc {
  public:
    NodeDesc(u64 node, u64 dep1, u64 dep2, u64 level) {
      node_ = node;
      deps_[0] = dep1;
      deps_[1] = dep2;
      level_ = level;
    }

    bool operator<(const NodeDesc &a) const { return this->node_ > a.node_; }

    std::string ToString(void) {
      std::ostringstream str_stream;
      if (node_ == 0) {
        str_stream << "[Node:" << node_ << ",inNode:" << deps_[0]
                   << ",level:" << level_ << "]";
        return str_stream.str();
      }

      str_stream << "[Node:" << node_ << ",inNode:" << deps_[0]
                 << ",inNode:" << deps_[1] << ",level:" << level_ << "]";
      return str_stream.str();
    }

    u64 level_;
    u64 node_;
    u64 deps_[2];
  };

  std::vector<std::set<NodeDesc>> node_by_depth_;
};
} // End namespace primihub.

#endif
