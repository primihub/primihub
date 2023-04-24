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
#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class CSVDriver;
struct CSVAccessInfo : public DataSetAccessInfo {
  CSVAccessInfo() = default;
  CSVAccessInfo(const std::string& file_path) : file_path_(file_path) {}
  std::string toString() override;
  retcode fromJsonString(const std::string& access_info) override;
  retcode ParseFromJsonImpl(const nlohmann::json& access_info) override;
  retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) override;
  retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) override;

 public:
  std::string file_path_;
};

class CSVCursor : public Cursor {
 public:
  CSVCursor(std::string filePath, std::shared_ptr<CSVDriver> driver);
  CSVCursor(std::string filePath,
            const std::vector<int>& colnum_index,
            std::shared_ptr<CSVDriver> driver);
  ~CSVCursor();
  std::shared_ptr<primihub::Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit);
  int write(std::shared_ptr<Dataset> dataset) override;
  void close() override;

 protected:
  /**
   * convert column index to column name
  */
  retcode ColumnIndexToColumnName(const std::string& file_path,
                                  const std::vector<int>& column_index,
                                  const char delimiter,
                                  std::vector<std::string>* column_name);
  /**
   * customize convert option
   * if Specific columns selected, just get data from the specific columns
   * if column data type provided when the dataset register,
   * using registed data type
  */
  retcode BuildConvertOptions(char delimiter,
                              arrow::csv::ConvertOptions* convert_option);

 private:
  std::string filePath;
  unsigned long long offset = 0;
  std::shared_ptr<CSVDriver> driver_;
  std::vector<int> colum_index_;
};

class CSVDriver : public DataDriver,
                  public std::enable_shared_from_this<CSVDriver> {
public:
  explicit CSVDriver(const std::string &nodelet_addr);
  CSVDriver(const std::string &nodelet_addr,
            std::unique_ptr<DataSetAccessInfo> access_info);
  ~CSVDriver() {}
  std::unique_ptr<Cursor> read() override;
  std::unique_ptr<Cursor> read(const std::string &filePath) override;
  std::unique_ptr<Cursor> GetCursor() override;
  std::unique_ptr<Cursor> GetCursor(const std::vector<int>& col_index) override;
  std::unique_ptr<Cursor> initCursor(const std::string &filePath) override;
  std::string getDataURL() const override;
  // FIXME to be deleted
  int write(std::shared_ptr<arrow::Table> table, const std::string &filePath);

protected:
  void setDriverType();

private:
  std::string filePath_;
};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
