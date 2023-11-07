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
#include "src/primihub/data_store/seatunnel/seatunnel_driver.h"
#define CSV_DRIVER_NAME "CSV"
#define SQLITE_DRIVER_NAME "SQLITE"
#define HDFS_DRIVER_NAME "HDFS"
#define MYSQL_DRIVER_NAME "MYSQL"
#define IMAGE_DRIVER_NAME "IMAGE"
#define SEATUNNEL_DRIVER_NAME "SEATUNNEL"
namespace primihub {
class DataDirverFactory {
 public:
    using DataSetAccessInfoPtr = std::unique_ptr<DataSetAccessInfo>;
    static std::shared_ptr<DataDriver>
    getDriver(const std::string &dirverName,
            const std::string& nodeletAddr,
            DataSetAccessInfoPtr access_info = nullptr) {
        if (boost::to_upper_copy(dirverName) == CSV_DRIVER_NAME) {
            if (access_info == nullptr) {
                return std::make_shared<CSVDriver>(nodeletAddr);
            } else {
                return std::make_shared<CSVDriver>(nodeletAddr, std::move(access_info));
            }
        } else if (dirverName == HDFS_DRIVER_NAME) {
            // return new HDFSDriver(dirverName);
            // TODO not implemented yet
        } else if (boost::to_upper_copy(dirverName) == SQLITE_DRIVER_NAME) {
            if (access_info == nullptr) {
                return std::make_shared<SQLiteDriver>(nodeletAddr);
            } else {
                return std::make_shared<SQLiteDriver>(nodeletAddr, std::move(access_info));
            }
        } else if (boost::to_upper_copy(dirverName) == MYSQL_DRIVER_NAME) {
#ifdef ENABLE_MYSQL_DRIVER
            if (access_info == nullptr) {
                return std::make_shared<MySQLDriver>(nodeletAddr);
            } else {
                return std::make_shared<MySQLDriver>(nodeletAddr, std::move(access_info));
            }
#else
            std::string err_msg = "MySQL is not enabled";
            LOG(ERROR) << err_msg;
            throw std::invalid_argument(err_msg);
#endif
        } else if (boost::to_upper_copy(dirverName) == IMAGE_DRIVER_NAME) {
            if (access_info == nullptr) {
                return std::make_shared<ImageDriver>(nodeletAddr);
            } else {
                return std::make_shared<ImageDriver>(nodeletAddr, std::move(access_info));
            }
        } else if (boost::to_upper_copy(dirverName) == SEATUNNEL_DRIVER_NAME) {
            if (access_info == nullptr) {
                return std::make_shared<SeatunnelDriver>(nodeletAddr);
            } else {
                return std::make_shared<SeatunnelDriver>(nodeletAddr, std::move(access_info));
            }
        } else {
            std::string err_msg = "[DataDriverFactory]Invalid driver name [" + dirverName + "]";
            throw std::invalid_argument(err_msg);
        }
        return nullptr;
    }
    // internal
    static DataSetAccessInfoPtr createAccessInfoInternal(const std::string& driver_type) {
        std::string drive_type_ = strToUpper(driver_type);
        DataSetAccessInfoPtr access_info_ptr{nullptr};
        if (drive_type_ == CSV_DRIVER_NAME) {
            access_info_ptr = std::make_unique<CSVAccessInfo>();
        } else if (drive_type_ == SQLITE_DRIVER_NAME) {
            access_info_ptr = std::make_unique<SQLiteAccessInfo>();
        } else if (drive_type_ == MYSQL_DRIVER_NAME) {
#ifdef ENABLE_MYSQL_DRIVER
            access_info_ptr = std::make_unique<MySQLAccessInfo>();
#else
            LOG(ERROR) << "MySQL is not enabled";
#endif
        } else if (drive_type_ == IMAGE_DRIVER_NAME) {
          access_info_ptr = std::make_unique<ImageAccessInfo>();
        } else if (drive_type_ == SEATUNNEL_DRIVER_NAME) {
          access_info_ptr = std::make_unique<SeatunnelAccessInfo>();
        } else {
            LOG(ERROR) << "unsupported driver type: " << drive_type_;
            return access_info_ptr;
        }
        return access_info_ptr;
    }

    static DataSetAccessInfoPtr createAccessInfo(
            const std::string& driver_type, const std::string& meta_info) {
        auto access_info_ptr = createAccessInfoInternal(driver_type);
        if (access_info_ptr == nullptr) {
            return nullptr;
        }
        auto ret = access_info_ptr->fromJsonString(meta_info);
        if (ret == retcode::FAIL) {
            LOG(ERROR) << "create dataset access info failed";
            return nullptr;
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
            LOG(ERROR) << "create dataset access info failed";
            return nullptr;
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
        LOG(ERROR) << "create dataset access info failed";
        return nullptr;
      }
      return access_info_ptr;
    }
};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_FACTORY_H_
