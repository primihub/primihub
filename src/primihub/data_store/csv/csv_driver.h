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

#ifndef SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class CSVDriver;

class CSVCursor : public Cursor {
public:
  CSVCursor(std::string filePath, std::shared_ptr<CSVDriver> driver);
  ~CSVCursor();
  std::shared_ptr<primihub::Dataset> read() override;
  std::shared_ptr<primihub::Dataset> read(int64_t offset, int64_t limit);
  int write(std::shared_ptr<primihub::Dataset> dataset) override;
  void close() override;

private:
  std::string filePath;
  unsigned long long offset = 0;
  std::shared_ptr<CSVDriver> driver_;
};

class CSVDriver : public DataDriver,
                  public std::enable_shared_from_this<CSVDriver> {
public:
  explicit CSVDriver(const std::string &nodelet_addr);
  ~CSVDriver() {}

  std::shared_ptr<Cursor> &read(const std::string &filePath) override;
  std::shared_ptr<Cursor> &initCursor(const std::string &filePath) override;
  std::string getDataURL() const override;

private:
  std::string filePath_;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
