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

#ifndef SRC_PRIMIHUB_DATA_STORE_SEATUNNEL_SEATUNNEL_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_SEATUNNEL_SEATUNNEL_DRIVER_H_

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
class SeatunnelDriver;
struct SeatunnelAccessInfo : public DataSetAccessInfo {
  SeatunnelAccessInfo() = default;
  std::string toString() override;
  retcode ParseFromJsonImpl(const nlohmann::json& access_info) override;
  retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) override;
  retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) override;

 public:
  std::string ip;
  uint32_t port{0};
  std::string user_name;
  std::string password;
  std::string database;
  std::string db_name;
  std::string table_name;
  std::string db_url;
  std::string source_type;
  std::string db_driver_;
};

class SeatunnelCursor : public Cursor {
 public:
  SeatunnelCursor(const std::string& sql, std::shared_ptr<SeatunnelDriver> driver);
  SeatunnelCursor(std::shared_ptr<SeatunnelDriver> driver) : driver_(driver) {}
  SeatunnelCursor(const std::vector<int>& selected_column_index,
                  std::shared_ptr<SeatunnelDriver> driver);
  ~SeatunnelCursor();
  std::shared_ptr<Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(
      const std::shared_ptr<arrow::Schema>& data_schema) override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) override;
  int write(std::shared_ptr<Dataset> dataset) override;
  void close() override;

 protected:
  retcode BuildQuerySql(const std::shared_ptr<arrow::Schema>& data_schema,
                        const SeatunnelAccessInfo& access_info,
                        int64_t query_limit,
                        std::string* query_sql);
  retcode MysqlBuildQuerySql(const std::vector<std::string>& query_field_name,
                             const SeatunnelAccessInfo& access_info,
                             int64_t query_limit,
                             std::string* query_sql);
  retcode DmBuildQuerySql(const std::vector<std::string>& query_field_name,
                          const SeatunnelAccessInfo& access_info,
                          int64_t query_limit,
                          std::string* query_sql);
  retcode SqlServerBuildQuerySql(
      const std::vector<std::string>& query_field_name,
      const SeatunnelAccessInfo& access_info,
      int64_t query_limit,
      std::string* query_sql);
  retcode OracleBuildQuerySql(const std::vector<std::string>& query_field_name,
                              const SeatunnelAccessInfo& access_info,
                              int64_t query_limit,
                              std::string* query_sql);
  retcode HiveBuildQuerySql(const std::vector<std::string>& query_field_name,
                            const SeatunnelAccessInfo& access_info,
                            int64_t query_limit,
                            std::string* query_sql);
  std::shared_ptr<arrow::Schema> MakeArrowSchema();

 private:
  std::string sql_;
  unsigned long long offset_{0};   // NOLINT
  std::shared_ptr<SeatunnelDriver> driver_;
  std::vector<int> colum_index_;
};

class SeatunnelDriver : public DataDriver,
                        public std::enable_shared_from_this<SeatunnelDriver> {
 public:
  explicit SeatunnelDriver(const std::string &nodelet_addr);
  SeatunnelDriver(const std::string &nodelet_addr,
                  std::unique_ptr<DataSetAccessInfo> access_info);
  ~SeatunnelDriver() {}
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

 private:
  std::string filePath_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_DATA_STORE_SEATUNNEL_SEATUNNEL_DRIVER_H_
