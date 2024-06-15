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

#ifndef SRC_PRIMIHUB_DATA_STORE_PARQUET_PARQUET_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_PARQUET_PARQUET_DRIVER_H_

#include <arrow/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>

#include <memory>
#include <vector>
#include <string>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class ParquetDriver;
struct ParquetAccessInfo : public DataSetAccessInfo {
  ParquetAccessInfo() = default;
  explicit ParquetAccessInfo(const std::string& file_path) :
      file_path_(file_path) {}
  std::string toString() override;
  retcode fromJsonString(const std::string& access_info) override;
  retcode ParseFromJsonImpl(const nlohmann::json& access_info) override;
  retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) override;
  retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) override;

 public:
  std::string file_path_;
};

class ParquetCursor : public Cursor {
 public:
  ParquetCursor(const std::string& file_path,
                std::shared_ptr<ParquetDriver> driver);
  ParquetCursor(const std::string& file_path,
                const std::vector<int>& colnum_index,
                std::shared_ptr<ParquetDriver> driver);
  ~ParquetCursor();
  std::shared_ptr<Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(
      const std::shared_ptr<arrow::Schema>& data_schema) override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) override;
  int write(std::shared_ptr<Dataset> dataset) override;
  void close() override;

 private:
  std::string file_path_;
  unsigned long long offset_{0};   // NOLINT
  std::shared_ptr<ParquetDriver> driver_;
  std::vector<int> colum_index_;
};

class ParquetDriver : public DataDriver,
                  public std::enable_shared_from_this<ParquetDriver> {
 public:
  explicit ParquetDriver(const std::string &nodelet_addr);
  ParquetDriver(const std::string &nodelet_addr,
            std::unique_ptr<DataSetAccessInfo> access_info);
  ~ParquetDriver() {}
  std::unique_ptr<Cursor> read() override;
  std::unique_ptr<Cursor> read(const std::string &filePath) override;
  std::unique_ptr<Cursor> GetCursor() override;
  std::unique_ptr<Cursor> GetCursor(const std::vector<int>& col_index) override;
  std::unique_ptr<Cursor> initCursor(const std::string &filePath) override;
  std::string getDataURL() const override;
  /**
   *  table: data need to write
   *  file_path: file location
  */
  int write(std::shared_ptr<arrow::Table> table,
            const std::string& file_path);
  /**
   * write csv title using customer define column name
   * and ignore the title defined by table schema
  */
  retcode Write(const std::vector<std::string>& fields_name,
                std::shared_ptr<arrow::Table> table,
                const std::string& file_path);

 protected:
  void setDriverType();
  retcode GetColumnNames(const char delimiter,
                         std::vector<std::string>* column_names);

 private:
  std::string file_path_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_DATA_STORE_PARQUET_PARQUET_DRIVER_H_
