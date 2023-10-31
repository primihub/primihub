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

#ifndef SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>

#include <memory>
#include <vector>
#include <string>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class CSVDriver;
struct CSVAccessInfo : public DataSetAccessInfo {
  CSVAccessInfo() = default;
  explicit CSVAccessInfo(const std::string& file_path) :
      file_path_(file_path) {}
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
  using ReadOptions = arrow::csv::ReadOptions;
  using ParseOptions = arrow::csv::ParseOptions;
  using ConvertOptions = arrow::csv::ConvertOptions;

  struct CsvOptions {
    ReadOptions read_options{ReadOptions::Defaults()};
    ParseOptions parse_options{ParseOptions::Defaults()};
    ConvertOptions convert_options{ConvertOptions::Defaults()};
  };
  CSVCursor(const std::string& file_path, std::shared_ptr<CSVDriver> driver);
  CSVCursor(const std::string& file_path,
            const std::vector<int>& colnum_index,
            std::shared_ptr<CSVDriver> driver);
  ~CSVCursor();
  std::shared_ptr<Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(
      const std::shared_ptr<arrow::Schema>& data_schema) override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) override;
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
   * using registered data type
  */
  retcode BuildConvertOptions(ConvertOptions* convert_option);
  /**
   * customize convert option
   * convert option is specified by data schema
   *
  */
  retcode BuildConvertOptions(const std::shared_ptr<arrow::Schema>& data_schema,
                              ConvertOptions* convert_option);
  retcode MakeCsvOptions(CsvOptions* options);
  retcode MakeCsvOptions(const std::shared_ptr<arrow::Schema>& data_schema,
                         CsvOptions* options);

  std::shared_ptr<Dataset> ReadImpl(const std::string& file_path,
                                    const ReadOptions& read_opt,
                                    const ParseOptions& parse_opt,
                                    const ConvertOptions& convert_opt);

  std::shared_ptr<Dataset> ReadImpl(std::string_view content_buf,
                                    const ReadOptions& read_opt,
                                    const ParseOptions& parse_opt,
                                    const ConvertOptions& convert_opt);

 private:
  std::string file_path_;
  unsigned long long offset_{0};   // NOLINT
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
  std::string filePath_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_DATA_STORE_CSV_CSV_DRIVER_H_
