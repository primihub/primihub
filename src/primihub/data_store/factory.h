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

#ifndef SRC_PRIMIHUB_DATA_STORE_FACTORY_H_
#define SRC_PRIMIHUB_DATA_STORE_FACTORY_H_

#include <memory>
#include <boost/algorithm/string.hpp>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class DataDirverFactory {
  public:
    static std::shared_ptr<DataDriver>
    getDriver(const std::string &dirverName, const std::string &nodeletAddr) {
        if ( boost::to_upper_copy(dirverName) == "CSV" ) {
            auto csvDriver = std::make_shared<CSVDriver>(nodeletAddr);
            return std::dynamic_pointer_cast<DataDriver>(csvDriver);

        } else if (dirverName == "HDFS") {
            // return new HDFSDriver(dirverName);
        } else {
            throw std::invalid_argument(
                "[DataDirverFactory]Invalid dirver name");
        }
    }
};
} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_FACTORY_H_
