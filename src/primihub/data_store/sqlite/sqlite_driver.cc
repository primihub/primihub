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

#include <variant>
#include <glog/logging.h>
#include "src/primihub/data_store/sqlite/sqlite_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"

#include <arrow/api.h>
// #include <arrow/csv/api.h>
// #include <arrow/csv/writer.h>
// #include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>

namespace primihub {

// sqlite cursor implementation
SQLiteCursor::SQLiteCursor(const std::string& sql, std::shared_ptr<SQLiteDriver> driver) {
  this->sql_ = sql;
  this->driver_ = driver;
}

SQLiteCursor::~SQLiteCursor() { this->close(); }

void SQLiteCursor::close() {}

// read all data from csv file
std::shared_ptr<primihub::Dataset>
SQLiteCursor::read() {
  VLOG(5) << "sql_sql_sql_sql_: " << sql_;
  std::shared_ptr<arrow::Table> table{nullptr};
  auto& db_connector = this->driver_->getDBConnector();
  SQLite::Statement sql_query(*db_connector, sql_);
  std::map<std::string, std::unique_ptr<TypeContainer>> query_result;
  std::vector<std::tuple<std::string, sql_type_t>> col_metas;
  bool col_meta_collected{false};
  std::vector<std::shared_ptr<arrow::Field>> result_schema_filed;
  while (sql_query.executeStep()) {
    for (size_t i = 0; i < sql_query.getColumnCount(); i++) {
      std::string col_type = sql_query.getColumnDeclaredType(i);
      auto col_name = sql_query.getColumnOriginName(i);
      if (!col_meta_collected) {
        sql_type_t sql_col_type = this->get_sql_type_by_type_name(col_type);
        switch (sql_col_type) {
        case sql_type_t::STRING:
          col_metas.emplace_back(std::make_tuple(col_name, sql_col_type));
          result_schema_filed.push_back(arrow::field(col_name, arrow::binary()));
          break;
        case sql_type_t::INT:
          col_metas.emplace_back(std::make_tuple(col_name, sql_col_type));
          result_schema_filed.push_back(arrow::field(col_name, arrow::int64()));
          break;
        case sql_type_t::DOUBLE:
          col_metas.emplace_back(std::make_tuple(col_name, sql_col_type));
          result_schema_filed.push_back(arrow::field(col_name, arrow::float64()));
          break;
        default:
          break;
        }
      }
      // process data
      switch (std::get<1>(col_metas[i])) {
      case sql_type_t::STRING: {
        std::string col_value = sql_query.getColumn(i);
        if (query_result.find(col_name) == query_result.end()) {
          query_result[col_name] = std::make_unique<TypeContainer>(sql_type_t::STRING);
        }
        query_result[col_name]->string_values.emplace_back(std::move(col_value));
      }
      break;
      case sql_type_t::INT: {
        int64_t col_value = sql_query.getColumn(i);
        if (query_result.find(col_name) == query_result.end()) {
          query_result[col_name] = std::make_unique<TypeContainer>(sql_type_t::INT);
        }
        query_result[col_name]->int_values.emplace_back(col_value);
      }
      break;
      case sql_type_t::DOUBLE: {
        double col_value = sql_query.getColumn(i);
        if (query_result.find(col_name) == query_result.end()) {
          query_result[col_name] = std::make_unique<TypeContainer>(sql_type_t::DOUBLE);

        }
        query_result[col_name]->double_values.emplace_back(col_value);
      }
      break;
      default:
      LOG(ERROR) << "unknown sql type: ";
      break;
      }
    }
    col_meta_collected = true;
  }
  // convert data to arrow array
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  for (size_t i = 0; i < col_metas.size(); i++) {
    auto& col_name = std::get<0>(col_metas[i]);
    auto& col_type = std::get<1>(col_metas[i]);
    switch (col_type) {
    case sql_type_t::STRING:{
      arrow::StringBuilder builder;
      auto& string_values = query_result[col_name]->string_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(string_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    }
    break;
    case sql_type_t::INT:{
      arrow::NumericBuilder<arrow::Int64Type> builder;
      auto& int_values = query_result[col_name]->int_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(int_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    }
    break;
    case sql_type_t::DOUBLE: {
      arrow::NumericBuilder<arrow::DoubleType> builder;
      auto& double_values = query_result[col_name]->double_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(double_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    }
    break;
    }
  }
  auto schema = std::make_shared<arrow::Schema>(result_schema_filed);
  table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<primihub::Dataset>
SQLiteCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

int SQLiteCursor::write(std::shared_ptr<primihub::Dataset> dataset) {

}

// ======== SQLite Driver implementation ========

SQLiteDriver::SQLiteDriver(const std::string &nodelet_addr) : DataDriver(nodelet_addr) {
  driver_type = "SQLITE";
}

std::shared_ptr<Cursor>& SQLiteDriver::read(const std::string& conn_str) {
   return this->initCursor(conn_str);
}

std::shared_ptr<Cursor>& SQLiteDriver::initCursor(const std::string& conn_str) {
  // parse conn_str
  VLOG(5) << "conn_strconn_strconn_strconn_str: " << conn_str;
  conn_info_ = conn_str;
  std::vector<std::string> conn_info;
  char sep_operator = '#';
  str_split(conn_str, &conn_info, sep_operator);
  auto driver_type = conn_info[CONN_FIELDS::DRIVER_TYPE];
  auto& db_path = conn_info[CONN_FIELDS::DB_PATH];
  db_path_ = db_path;
  std::string& table_name = conn_info[CONN_FIELDS::TABLE_NAME];
  VLOG(5) << "db_path: " << db_path << " table_name: " << table_name << " conn_info size: " << conn_info.size();
  try {
    this->db_connector = std::make_unique<SQLite::Database>(db_path);
  } catch (std::exception& e) {
    LOG(ERROR) << "create cursor failed: " << e.what();
    return getCursor();  // nullptr
  }

  std::string& query_condition = conn_info[CONN_FIELDS::QUERY_CONDITION];
  std::string sql_str = "select ";
  if (query_condition.empty()) {
    sql_str.append("*");
  } else {
    sql_str.append(query_condition);
  }
  sql_str.append(" from ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  this->cursor = std::make_shared<SQLiteCursor>(sql_str, shared_from_this());
  return getCursor();
}

// write data to specifiy table
int SQLiteDriver::write(std::shared_ptr<arrow::Table> table, const std::string& table_name) {
  return 0;
}

std::string SQLiteDriver::getDataURL() const { return conn_info_; };

} // namespace primihub
