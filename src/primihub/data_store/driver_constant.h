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
#ifndef SRC_PRIMIHUB_DATA_STORE_DRIVER_CONSTANT_H_
#define SRC_PRIMIHUB_DATA_STORE_DRIVER_CONSTANT_H_
#include <map>
#include <string>

namespace primihub {
enum class DriverType {
  CSV = 0,
  SQLITE,
  HDFS,
  MYSQL,
  IMAGE,
  PARQUET,
};

static std::map<DriverType, std::string> kDriveType = {
  {DriverType::CSV, "CSV"},
  {DriverType::SQLITE, "SQLITE"},
  {DriverType::HDFS, "HDFS"},
  {DriverType::MYSQL, "MYSQL"},
  {DriverType::IMAGE, "IMAGE"},
  {DriverType::PARQUET, "PARQUET"},
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_DATA_STORE_DRIVER_CONSTANT_H_
