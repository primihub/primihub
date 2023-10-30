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

#ifndef SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "SQLiteCpp/Column.h"
#include <iomanip>
#include <vector>

namespace primihub {
class SQLiteDriver;

struct SQLiteAccessInfo : public DataSetAccessInfo {
  SQLiteAccessInfo() = default;
  SQLiteAccessInfo(const std::string& db_path, const std::string& tab_name,
      const std::vector<std::string>& query_colums)
      : db_path_(db_path), table_name_(tab_name) {}
  std::string toString() override;
  retcode ParseFromJsonImpl(const nlohmann::json& access_info) override;
  retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) override;
  retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) override;

 public:
  std::string db_path_;
  std::string table_name_;
  // std::vector<std::string> query_colums_;
};

class SQLiteCursor : public Cursor {
public:
  SQLiteCursor(const std::string& sql, std::shared_ptr<SQLiteDriver> driver);
  /**
   * build read cursor by selected column
   * parameter:
   * sql: query sql
   * col_index: select col index, must be match with sql
   * driver: dataset driver
  */
  SQLiteCursor(const std::string& sql,
              const std::vector<int>& col_index,
              std::shared_ptr<SQLiteDriver> driver);
  ~SQLiteCursor();
  std::shared_ptr<Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(const std::shared_ptr<arrow::Schema>& data_schema) override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) override;
  std::shared_ptr<Dataset> readInternal(const std::string& query_sql);
  std::shared_ptr<arrow::Table>
  read_from_abnormal(std::map<std::string, uint32_t> col_type,
                     std::map<std::string, std::vector<int>> &index);
  int write(std::shared_ptr<Dataset> dataset) override;
  void close() override;

 protected:
  enum class sql_type_t : int8_t{
    STRING = 0,
    INT,
    INT64,
    FLOAT,
    DOUBLE,
    UNKNOWN,
  };
  struct TypeContainer {
    explicit TypeContainer(sql_type_t col_type) : col_type_(col_type) {}
    std::vector<std::string> string_values;
    std::vector<float> float_values;
    std::vector<double> double_values;
    std::vector<int64_t> int_values;
    sql_type_t col_type_;
  };
  sql_type_t get_sql_type_by_type_name(const std::string& type_name) {
    auto it = sql_type_name_to_enum.find(type_name);
    if (it != sql_type_name_to_enum.end()) {
      return it->second;
    }
    else {
      if(std::string::npos != type_name.find("VARCHAR"))
        return sql_type_t::STRING;
    }
    return sql_type_t::UNKNOWN;
  }

 private:
  std::string sql_;
  unsigned long long offset_{0};
  std::shared_ptr<SQLiteDriver> driver_{nullptr};
  std::map<std::string, sql_type_t> sql_type_name_to_enum {
    {"TEXT", sql_type_t::STRING},
    {"INTEGER", sql_type_t::INT64},
    {"INT", sql_type_t::INT},
    {"DOUBLE", sql_type_t::DOUBLE},
  };
};

class SQLiteDriver : public DataDriver, public std::enable_shared_from_this<SQLiteDriver> {
public:
  // explicit SQLiteDriver(const std::string &nodelet_addr);
  explicit SQLiteDriver(const std::string &nodelet_addr);
  SQLiteDriver(const std::string &nodelet_addr, std::unique_ptr<DataSetAccessInfo> access_info);
  ~SQLiteDriver() = default;
  std::unique_ptr<Cursor> read() override;
  std::unique_ptr<Cursor> read(const std::string& conn_str) override;
  std::unique_ptr<Cursor> GetCursor() override;
  std::unique_ptr<Cursor> GetCursor(const std::vector<int>& col_index) override;
  std::unique_ptr<Cursor> initCursor(const std::string& conn_str) override;
  std::string getDataURL() const override;
  std::unique_ptr<SQLite::Database>& getDBConnector() { return db_connector; }
  // write data to specify db table
  int write(std::shared_ptr<arrow::Table> table, const std::string& table_name);
 protected:
  void setDriverType();
  enum CONN_FIELDS {
    DRIVER_TYPE = 0,
    DB_PATH,
    TABLE_NAME,
    QUERY_CONDITION,
  };
  std::string buildQuerySQL(SQLiteAccessInfo* access_info);
  std::string buildQuerySQL(const std::string& table_name,
                            const std::string& query_index);
  retcode GetDBTableSchema();
  std::string BuildQuerySQL(const SQLiteAccessInfo& access_info,
                            const std::vector<int>& col_index,
                            std::vector<std::string>* colum_names);

 private:
  std::string conn_info_;
  std::string db_path_;
  std::unique_ptr<SQLite::Database> db_connector{nullptr};
  std::map<std::string, std::string> table_schema_;   // dbtable schema
  std::vector<std::string> table_cols_;               // db table column name
};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_
