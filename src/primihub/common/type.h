/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_PRIMIHUB_COMMON_TYPE_H_
#define SRC_PRIMIHUB_COMMON_TYPE_H_
#include "Eigen/Dense"
#include "cryptoTools/Common/Defines.h"
#include "aby3/sh3/Sh3Types.h"
#include "aby3/sh3/Sh3FixedPoint.h"
namespace primihub {
template<typename T>
// all type definition from crytoTools or aby3
// export to primihub type as follows
using eMatrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

using i64Matrix = eMatrix<int64_t>;
using Decimal = aby3::Decimal;

enum class ColumnDtype {
  STRING = 0,
  INTEGER = 1,
  DOUBLE = 2,
  LONG = 3,
  ENUM = 4,
  BOOLEAN = 5,
  UNKNOWN = 6,
};

std::string columnDtypeToString(const ColumnDtype &type);
void columnDtypeFromInteger(int val, ColumnDtype &type);
}  // namespace primihub
#endif  // SRC_PRIMIHUB_COMMON_TYPE_H_
