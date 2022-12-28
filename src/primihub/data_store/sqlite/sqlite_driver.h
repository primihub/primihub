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

#ifndef SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "SQLiteCpp/Column.h"
#include <iomanip>

namespace primihub {
class SQLiteDriver;

class SQLiteCursor : public Cursor {
public:
  SQLiteCursor(const std::string& sql, std::shared_ptr<SQLiteDriver> driver);
  ~SQLiteCursor();
  std::shared_ptr<primihub::Dataset> read() override;
  std::shared_ptr<primihub::Dataset> read(int64_t offset, int64_t limit);
  std::shared_ptr<arrow::Table> 
  read_from_abnormal(std::map<std::string, uint32_t> col_type,
                     std::map<std::string, std::vector<int>> &index);
  int write(std::shared_ptr<primihub::Dataset> dataset) override;
  void close() override;

 protected:
  enum class sql_type_t : int8_t{
    STRING = 0,
    INT,
    INT64,
    FLOAT,
    DOUBLE,
    UNKONW,
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
    return sql_type_t::UNKONW;
  }

 private:
  std::string sql_;
  unsigned long long offset = 0;
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
  SQLiteDriver(const std::string &nodelet_addr);
  ~SQLiteDriver() = default;

  std::shared_ptr<Cursor>& read(const std::string& conn_str) override;
  std::shared_ptr<Cursor>& initCursor(const std::string& conn_str) override;
  std::string getDataURL() const override;
  std::unique_ptr<SQLite::Database>& getDBConnector() { return db_connector; }
  // write data to specifiy db table
  int write(std::shared_ptr<arrow::Table> table, const std::string& table_name);
 protected:
  enum CONN_FIELDS {
    DRIVER_TYPE = 0,
    DB_PATH,
    TABLE_NAME,
    QUERY_CONDITION,
  };
 private:
  std::string conn_info_;
  std::string db_path_;
  std::unique_ptr<SQLite::Database> db_connector{nullptr};


};

} // namespace primihub

#endif // SRC_PRIMIHUB_DATA_STORE_SQLITE_SQLITE_DRIVER_H_
