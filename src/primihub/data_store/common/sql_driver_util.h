/*
 Copyright 2023 Primihub

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
#ifndef SRC_PRIMIHUB_DATA_STORE_COMMON_SQL_UTIL_H_
#define SRC_PRIMIHUB_DATA_STORE_COMMON_SQL_UTIL_H_
#include <unordered_map>
#include <arrow/api.h>
#include "src/primihub/common/common.h"

namespace primihub {
retcode SqlType2ArrowType(const std::string& sql_type, int* arrow_type);
class SqlCommonOperation {
 public:
  std::shared_ptr<arrow::Array>
  makeArrowArray(int field_type, const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  Int32ArrowArrayBuilder(const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  Int64ArrowArrayBuilder(const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  FloatArrowArrayBuilder(const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  DoubleArrowArrayBuilder(const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  StringArrowArrayBuilder(const std::vector<std::string>& arr);

  std::shared_ptr<arrow::Array>
  BinaryArrowArrayBuilder(const std::vector<std::string>& arr);
};

}  // namespace primihub
#endif  // SRC_PRIMIHUB_DATA_STORE_SQL_DRIVER_H_