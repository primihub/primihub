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

#include "src/primihub/data_store/sqlite/sqlite_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include <glog/logging.h>
#include <variant>

#include <arrow/api.h>
// #include <arrow/csv/api.h>
// #include <arrow/csv/writer.h>
// #include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <nlohmann/json.hpp>

namespace primihub {
// SQLiteAccessInfo implementation
std::string SQLiteAccessInfo::toString() {
    std::stringstream ss;
    nlohmann::json js;
    js["db_path"] = this->db_path_;
    js["tableName"] = this->table_name_;
    if (!query_colums_.empty()) {
        std::string quey_col_info;
        for (const auto& col : query_colums_) {
            quey_col_info.append(col).append(",");
        }
        js["query_index"] = std::move(quey_col_info);
    }
    ss << std::setw(4) << js;
    return ss.str();
}

retcode SQLiteAccessInfo::fromJsonString(const std::string& access_info) {
    if (access_info.empty()) {
        LOG(ERROR) << "access info is empty";
        return retcode::FAIL;
    }
    nlohmann::json js = nlohmann::json::parse(access_info);
    try {
        this->db_path_ = js["db_path"].get<std::string>();
        this->table_name_ = js["tableName"].get<std::string>();
        this->query_colums_.clear();
        if (js.contains("query_index")) {
        std::string query_index_info = js["query_index"];
        str_split(query_index_info, &query_colums_, ',');
    }
    } catch (std::exception& e) {
        LOG(ERROR) << "parse sqlite access info failed, " << e.what()
            << " origin access info: [" << access_info << "]";
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

retcode SQLiteAccessInfo::fromYamlConfig(const YAML::Node& meta_info) {
    try {
        this->db_path_ = meta_info["source"].as<std::string>();
        this->table_name_ = meta_info["table_name"].as<std::string>();
        this->query_colums_.clear();
        if (meta_info["query_index"]) {
            std::string query_index = meta_info["query_index"].as<std::string>();
            str_split(query_index, &query_colums_, ',');
        }
    } catch (std::exception& e) {
        LOG(ERROR) << "parse sqlite access info encountes error, " << e.what();
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

// sqlite cursor implementation
SQLiteCursor::SQLiteCursor(const std::string &sql,
                           std::shared_ptr<SQLiteDriver> driver) {
  this->sql_ = sql;
  this->driver_ = driver;
}

SQLiteCursor::~SQLiteCursor() { this->close(); }

void SQLiteCursor::close() {}

// read all data from csv file
std::shared_ptr<primihub::Dataset> SQLiteCursor::read() {
  VLOG(5) << "sql_sql_sql_sql_: " << sql_;
  std::shared_ptr<arrow::Table> table{nullptr};
  auto &db_connector = this->driver_->getDBConnector();
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
          result_schema_filed.push_back(
              arrow::field(col_name, arrow::binary()));
          break;
        case sql_type_t::INT:
          col_metas.emplace_back(std::make_tuple(col_name, sql_col_type));
          result_schema_filed.push_back(arrow::field(col_name, arrow::int64()));
          break;
        case sql_type_t::DOUBLE:
          col_metas.emplace_back(std::make_tuple(col_name, sql_col_type));
          result_schema_filed.push_back(
              arrow::field(col_name, arrow::float64()));
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
          query_result[col_name] =
              std::make_unique<TypeContainer>(sql_type_t::STRING);
        }
        query_result[col_name]->string_values.emplace_back(
            std::move(col_value));
      } break;
      case sql_type_t::INT: {
        int64_t col_value = sql_query.getColumn(i);
        if (query_result.find(col_name) == query_result.end()) {
          query_result[col_name] =
              std::make_unique<TypeContainer>(sql_type_t::INT);
        }
        query_result[col_name]->int_values.emplace_back(col_value);
      } break;
      case sql_type_t::DOUBLE: {
        double col_value = sql_query.getColumn(i);
        if (query_result.find(col_name) == query_result.end()) {
          query_result[col_name] =
              std::make_unique<TypeContainer>(sql_type_t::DOUBLE);
        }
        query_result[col_name]->double_values.emplace_back(col_value);
      } break;
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
    auto &col_name = std::get<0>(col_metas[i]);
    auto &col_type = std::get<1>(col_metas[i]);
    switch (col_type) {
    case sql_type_t::STRING: {
      arrow::StringBuilder builder;
      auto &string_values = query_result[col_name]->string_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(string_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    case sql_type_t::INT: {
      arrow::NumericBuilder<arrow::Int64Type> builder;
      auto &int_values = query_result[col_name]->int_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(int_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    case sql_type_t::DOUBLE: {
      arrow::NumericBuilder<arrow::DoubleType> builder;
      auto &double_values = query_result[col_name]->double_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(double_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    }
  }
  auto schema = std::make_shared<arrow::Schema>(result_schema_filed);
  table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<primihub::Dataset> SQLiteCursor::read(int64_t offset,
                                                      int64_t limit) {
  return nullptr;
}

std::shared_ptr<arrow::Table> SQLiteCursor::read_from_abnormal(
    std::map<std::string, uint32_t> col_type,
    std::map<std::string, std::vector<int>> &index) {
  std::shared_ptr<arrow::Table> table{nullptr};
  auto &db_connector = this->driver_->getDBConnector();
  SQLite::Statement sql_query(*db_connector, sql_);
  std::map<std::string, std::unique_ptr<TypeContainer>> query_result;
  std::vector<std::tuple<std::string, uint32_t>> col_metas;
  // bool col_meta_collected{false};
  std::vector<std::shared_ptr<arrow::Field>> result_schema_filed;

  for (auto itr = col_type.begin(); itr != col_type.end(); itr++) {
    switch (itr->second) {
    // string
    case 0:
      result_schema_filed.push_back(arrow::field(itr->first, arrow::binary()));
      break;
    // int64
    case 1:
      result_schema_filed.push_back(arrow::field(itr->first, arrow::int64()));
      break;
    // double
    case 2:
      result_schema_filed.push_back(arrow::field(itr->first, arrow::float64()));
      break;
    // LONG
    case 3:
      result_schema_filed.push_back(arrow::field(itr->first, arrow::int64()));
      break;
    // //BOOLEAN
    // case 5:
    //   result_schema_filed.push_back(arrow::field(itr->first,
    //   arrow::boolean())); break;
    default:
      break;
    }
  }
  int row = 0;
  while (sql_query.executeStep()) {
    for (size_t i = 0; i < sql_query.getColumnCount(); i++) {
      std::vector<int> vec_index;
      auto col_name = sql_query.getColumnOriginName(i);
      // process data
      SQLite::Column col = sql_query.getColumn(col_name);
      auto iter = col_type.find(col_name);

      if (iter != col_type.end()) {
        std::map<std::string, std::vector<int>>::iterator index_iter;
        if (row == 0) {
          if (iter->second == 0) {
            col_metas.emplace_back(std::make_tuple(col_name, 0));
            query_result[col_name] =
                std::make_unique<TypeContainer>(sql_type_t::STRING);
          }
          if (iter->second == 1) {
            col_metas.emplace_back(std::make_tuple(col_name, 1));
            query_result[col_name] =
                std::make_unique<TypeContainer>(sql_type_t::INT64);
          } else if (iter->second == 2) {
            col_metas.emplace_back(std::make_tuple(col_name, 2));
            query_result[col_name] =
                std::make_unique<TypeContainer>(sql_type_t::DOUBLE);
          } else if (iter->second == 3) {
            col_metas.emplace_back(std::make_tuple(col_name, 3));
            query_result[col_name] =
                std::make_unique<TypeContainer>(sql_type_t::INT64);
          }
          // else if(iter->second==5){
          //   col_metas.emplace_back(std::make_tuple(col_name, 5));
          //   query_result[col_name] =
          //   std::make_unique<TypeContainer>(sql_type_t::BOOLEAN);
          // }
        }
        switch (col.getType()) {
        //  SQLite::SQLITE_NULL
        case 5: {
          // check row data whether is NULL
          if (iter->second == 0) {
            query_result[col_name]->string_values.emplace_back(std::move(""));
          } else {
            index_iter = index.find(col_name);
            if (index_iter == index.end()) {
              index[col_name] = vec_index;
            }
            index[col_name].push_back(row);

            if (iter->second == 1) {
              query_result[col_name]->int_values.emplace_back(0);
            } else if (iter->second == 2) {
              query_result[col_name]->double_values.emplace_back(0);
            } else if (iter->second == 3) {
              query_result[col_name]->double_values.emplace_back(0);
            }
          }
        } break;
        case 3: {
          const char *col_value = sql_query.getColumn(i);

          // process string
          if (iter->second == 0) {
            query_result[col_name]->string_values.emplace_back(
                std::move(col_value));
          } else {
            char *endptr;
            std::string tmp_val = col_value;
            std::string::size_type tmp_index = tmp_val.find('.');
            if (tmp_index == std::string::npos) {
              int int_val = strtol((const char *)col_value, &endptr, 10);
              if ((char *)col_value == endptr) {
                // this data need to be corrected
                index_iter = index.find(col_name);
                if (index_iter == index.end()) {
                  index[col_name] = vec_index;
                }
                index[col_name].push_back(row);

                if (iter->second == 1) {
                  query_result[col_name]->int_values.emplace_back(0);
                } else if (iter->second == 2) {
                  query_result[col_name]->double_values.emplace_back(0);
                } else if (iter->second == 3) {
                  query_result[col_name]->int_values.emplace_back(0);
                }
              } else {
                if (iter->second == 1) {
                  query_result[col_name]->int_values.emplace_back(int_val);
                } else if (iter->second == 2) {
                  query_result[col_name]->double_values.emplace_back(int_val);
                } else if (iter->second == 3) {
                  query_result[col_name]->int_values.emplace_back(int_val);
                }
              }
            } else {
              double d_val = strtod((const char *)col_value, &endptr);
              if ((char *)col_value == endptr) {
                // this data need to be corrected
                index_iter = index.find(col_name);
                if (index_iter == index.end()) {
                  index[col_name] = vec_index;
                }
                index[col_name].push_back(row);

                if (iter->second == 1) {
                  query_result[col_name]->int_values.emplace_back(0);
                } else if (iter->second == 2) {
                  query_result[col_name]->double_values.emplace_back(0);
                } else if (iter->second == 3) {
                  query_result[col_name]->int_values.emplace_back(0);
                }
              } else {
                if (iter->second == 1 || iter->second == 3) {
                  LOG(ERROR)
                      << "The original data type of " << col_name
                      << "is double, but the target data type is integer. "
                         "Please select the correct target type!";
                  return nullptr;
                } else if (iter->second == 2) {
                  query_result[col_name]->double_values.emplace_back(d_val);
                }
              }
            }
          }

        } break;
        case 2: {
          // process double
          double col_value = sql_query.getColumn(i);
          if (iter->second == 0) {
            std::stringstream ss;
            ss << std::setprecision(17) << col_value;
            query_result[col_name]->string_values.emplace_back(
                std::move(ss.str()));
          } else if (iter->second == 1 || iter->second == 3) {
            LOG(ERROR) << "The original data type of " << col_name
                       << "is double, but the target data type is integer. "
                          "Please select the correct target type!";
            return nullptr;
          } else if (iter->second == 2) {
            query_result[col_name]->double_values.emplace_back(col_value);
          }
        } break;
        case 1: {
          // process int64
          int64_t col_value = sql_query.getColumn(i);
          if (iter->second == 0) {
            std::stringstream ss;
            ss << std::setprecision(17) << col_value;
            query_result[col_name]->string_values.emplace_back(
                std::move(ss.str()));
          } else if (iter->second == 1) {
            query_result[col_name]->int_values.emplace_back(col_value);
          } else if (iter->second == 2) {
            query_result[col_name]->double_values.emplace_back(col_value);
          } else if (iter->second == 3) {
            query_result[col_name]->int_values.emplace_back(col_value);
          }
        } break;
        default:
          LOG(ERROR) << "unknown sql type: ";
          return nullptr;
        }
      }
    }
    row++;
  }
  // convert data to arrow array
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  for (size_t i = 0; i < col_metas.size(); i++) {
    auto &col_name = std::get<0>(col_metas[i]);
    auto &col_type = std::get<1>(col_metas[i]);
    switch (col_type) {
    case 0: {
      arrow::StringBuilder builder;
      auto &string_values = query_result[col_name]->string_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(string_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    case 1: {
      arrow::NumericBuilder<arrow::Int64Type> builder;
      auto &int_values = query_result[col_name]->int_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(int_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    case 2: {
      arrow::NumericBuilder<arrow::DoubleType> builder;
      auto &double_values = query_result[col_name]->double_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(double_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    case 3: {
      arrow::NumericBuilder<arrow::Int64Type> builder;
      auto &int_values = query_result[col_name]->int_values;
      std::shared_ptr<arrow::Array> array;
      builder.AppendValues(int_values);
      builder.Finish(&array);
      array_data.push_back(std::move(array));
    } break;
    }
  }
  auto schema = std::make_shared<arrow::Schema>(result_schema_filed);
  table = arrow::Table::Make(schema, array_data);
  std::vector<std::string> local_col_names = table->ColumnNames();

  // LOG(INFO)<<"result_schema_filed.size: "<<result_schema_filed.size();
  // for(auto i=0;i<result_schema_filed.size();i++)
  //   LOG(INFO)<<result_schema_filed[i];
  // LOG(INFO)<<"local_col_names.size: "<<local_col_names.size();
  // for(auto i=0;i<local_col_names.size();i++)
  //   LOG(INFO)<<local_col_names[i];
  // auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
  return table;
}

int SQLiteCursor::write(std::shared_ptr<primihub::Dataset> dataset) {}

// ======== SQLite Driver implementation ========

SQLiteDriver::SQLiteDriver(const std::string &nodelet_addr)
    : DataDriver(nodelet_addr) {
  setDriverType();
}

SQLiteDriver::SQLiteDriver(const std::string &nodelet_addr,
    std::unique_ptr<DataSetAccessInfo> access_info)
    : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
}

void SQLiteDriver::setDriverType() {
  driver_type = "SQLITE";
}

std::shared_ptr<Cursor>& SQLiteDriver::read() {
  auto access_info_ptr = dynamic_cast<SQLiteAccessInfo*>(this->access_info_.get());
  if (access_info_ptr == nullptr) {
    LOG(ERROR) << "sqlite access info is not unavailable";
    return getCursor();
  }
  try {
    this->db_connector = std::make_unique<SQLite::Database>(access_info_ptr->db_path_);
  } catch (std::exception &e) {
    LOG(ERROR) << "create cursor failed: " << e.what();
    return getCursor(); // nullptr
  }
  std::string query_sql = buildQuerySQL(access_info_ptr);
  this->cursor = std::make_shared<SQLiteCursor>(query_sql, shared_from_this());
  return getCursor();
}

std::shared_ptr<Cursor> &SQLiteDriver::read(const std::string &conn_str) {
  return this->initCursor(conn_str);
}

std::string SQLiteDriver::buildQuerySQL(SQLiteAccessInfo* access_info) {
  const auto& table_name = access_info->table_name_;
  const auto& query_cols = access_info->query_colums_;
  std::string sql_str = "SELECT ";
  if (query_cols.empty()) {
    sql_str.append("*");
  } else {
    for (int i = 0; i < query_cols.size(); ++i) {
      auto& col_name = query_cols[i];
      if (col_name.empty()) {
        continue;
      }
      if (i == query_cols.size()-1) {
        sql_str.append(col_name).append(" ");
      } else {
        sql_str.append(col_name).append(",");
      }
    }
  }
  sql_str.append(" FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

std::string SQLiteDriver::buildQuerySQL(const std::string& table_name,
    const std::string& query_condition) {
  std::string sql_str = "SELECT ";
  if (query_condition.empty()) {
    sql_str.append("*");
  } else {
    sql_str.append(query_condition);
  }
  sql_str.append(" FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

std::shared_ptr<Cursor> &SQLiteDriver::initCursor(const std::string &conn_str) {
  // parse conn_str
  VLOG(5) << "conn_strconn_strconn_strconn_str: " << conn_str;
  conn_info_ = conn_str;
  std::vector<std::string> conn_info;
  char sep_operator = '#';
  str_split(conn_str, &conn_info, sep_operator);
  auto driver_type = conn_info[CONN_FIELDS::DRIVER_TYPE];
  auto &db_path = conn_info[CONN_FIELDS::DB_PATH];
  db_path_ = db_path;
  std::string& table_name = conn_info[CONN_FIELDS::TABLE_NAME];
  VLOG(5) << "db_path: " << db_path << " table_name: " << table_name
          << " conn_info size: " << conn_info.size();
  try {
    this->db_connector = std::make_unique<SQLite::Database>(db_path);
  } catch (std::exception& e) {
    LOG(ERROR) << "create cursor failed: " << e.what();
    return getCursor(); // nullptr
  }

  std::string &query_condition = conn_info[CONN_FIELDS::QUERY_CONDITION];
  auto sql_str = buildQuerySQL(table_name, query_condition);
  this->cursor = std::make_shared<SQLiteCursor>(sql_str, shared_from_this());
  return getCursor();
}

// write data to specifiy table
int SQLiteDriver::write(std::shared_ptr<arrow::Table> table,
                        const std::string &table_name) {
  return 0;
}

std::string SQLiteDriver::getDataURL() const { return conn_info_; };

} // namespace primihub
