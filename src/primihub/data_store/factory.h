/*
 Copyright 2022 PrimiHub

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

#ifndef SRC_PRIMIHUB_DATA_STORE_FACTORY_H_
#define SRC_PRIMIHUB_DATA_STORE_FACTORY_H_

#include <memory>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/sqlite/sqlite_driver.h"
#include "src/primihub/data_store/image/image_driver.h"
#include "src/primihub/util/util.h"
#ifdef ENABLE_MYSQL_DRIVER
#include "src/primihub/data_store/mysql/mysql_driver.h"
#endif
#include "src/primihub/data_store/parquet/parquet_driver.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub {
class DataDirverFactory {
 public:
  using DataSetAccessInfoPtr = std::unique_ptr<DataSetAccessInfo>;
  using DataDriverPtr = std::shared_ptr<DataDriver>;
  static DataDriverPtr getDriver(const std::string &dirverName,
      const std::string& nodeletAddr,
      DataSetAccessInfoPtr access_info = nullptr) {
    DataDriverPtr driver_ptr{nullptr};
    std::string driver_name = strToUpper(dirverName);
    if (driver_name == kDriveType[DriverType::CSV]) {
      driver_ptr = std::make_shared<CSVDriver>(nodeletAddr,
                                               std::move(access_info));
    } else if (driver_name == kDriveType[DriverType::SQLITE]) {
      driver_ptr = std::make_shared<SQLiteDriver>(nodeletAddr,
                                                  std::move(access_info));
    } else if (driver_name == kDriveType[DriverType::MYSQL]) {
#ifdef ENABLE_MYSQL_DRIVER
      driver_ptr = std::make_shared<MySQLDriver>(nodeletAddr,
                                                 std::move(access_info));
#else
      std::string err_msg = "MySQL is not enabled";
      RaiseException(err_msg);
#endif
    } else if (driver_name == kDriveType[DriverType::IMAGE]) {
      driver_ptr = std::make_shared<ImageDriver>(nodeletAddr,
                                                 std::move(access_info));
    } else if (driver_name == kDriveType[DriverType::PARQUET]) {
      driver_ptr = std::make_shared<ParquetDriver>(nodeletAddr,
                                                   std::move(access_info));
    } else {
      std::string err_msg =
          "[DataDriverFactory] Invalid driver name [" + dirverName + "]";
      RaiseException(err_msg);
    }
    return driver_ptr;
  }

  // internal
  static DataSetAccessInfoPtr createAccessInfoInternal(
      const std::string& driver_type) {
    std::string drive_type_ = strToUpper(driver_type);
    DataSetAccessInfoPtr access_info_ptr{nullptr};
    if (drive_type_ == kDriveType[DriverType::CSV]) {
      access_info_ptr = std::make_unique<CSVAccessInfo>();
    } else if (drive_type_ == kDriveType[DriverType::SQLITE]) {
      access_info_ptr = std::make_unique<SQLiteAccessInfo>();
    } else if (drive_type_ == kDriveType[DriverType::MYSQL]) {
#ifdef ENABLE_MYSQL_DRIVER
      access_info_ptr = std::make_unique<MySQLAccessInfo>();
#else
      std::string err_msg = "MySQL is not enabled";
      RaiseException(err_msg);
#endif
    } else if (drive_type_ == kDriveType[DriverType::IMAGE]) {
      access_info_ptr = std::make_unique<ImageAccessInfo>();
    } else if (drive_type_ == kDriveType[DriverType::PARQUET]) {
      access_info_ptr = std::make_unique<ParquetAccessInfo>();
    } else {
      std::string err_msg = "unsupported driver type: " + drive_type_;
      RaiseException(err_msg);
    }
    return access_info_ptr;
  }

  static DataSetAccessInfoPtr createAccessInfo(
      const std::string& driver_type,
      const std::string& meta_info) {
    auto access_info_ptr = createAccessInfoInternal(driver_type);
    if (access_info_ptr == nullptr) {
      return nullptr;
    }
    auto ret = access_info_ptr->fromJsonString(meta_info);
    if (ret == retcode::FAIL) {
      std::string err_msg = "create dataset access info failed";
      RaiseException(err_msg);
    }
    return access_info_ptr;
  }

  static DataSetAccessInfoPtr createAccessInfo(
      const std::string& driver_type, const YAML::Node& meta_info) {
    auto access_info_ptr = createAccessInfoInternal(driver_type);
    if (access_info_ptr == nullptr) {
      return nullptr;
    }
    // init
    auto ret = access_info_ptr->fromYamlConfig(meta_info);
    if (ret == retcode::FAIL) {
      std::string err_msg = "create dataset access info failed";
      RaiseException(err_msg);
    }
    return access_info_ptr;
  }

  static DataSetAccessInfoPtr createAccessInfo(
      const std::string& driver_type, const DatasetMetaInfo& meta_info) {
    auto access_info_ptr = createAccessInfoInternal(driver_type);
    if (access_info_ptr == nullptr) {
      return nullptr;
    }
    // init
    auto ret = access_info_ptr->FromMetaInfo(meta_info);
    if (ret == retcode::FAIL) {
      std::string err_msg = "create dataset access info failed";
      RaiseException(err_msg);
    }
    return access_info_ptr;
  }
};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_FACTORY_H_
