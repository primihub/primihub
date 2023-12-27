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

#include "src/primihub/data_store/sqlite/sqlite_driver.h"
#include <glog/logging.h>
#include <arrow/api.h>
#include <arrow/io/api.h>

#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <variant>
#include <sstream>

#include <nlohmann/json.hpp>
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/arrow_wrapper_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/thread_local_data.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub {
// SQLiteAccessInfo implementation
std::string SQLiteAccessInfo::toString() {
    std::stringstream ss;
    nlohmann::json js;
    js["type"] = kDriveType[DriverType::SQLITE];
    js["db_path"] = this->db_path_;
    js["tableName"] = this->table_name_;
    // ss << std::setw(4) << js;
    ss << js;
    return ss.str();
}

retcode SQLiteAccessInfo::ParseFromJsonImpl(const nlohmann::json& access_info) {
  const auto& js = access_info;
  try {
    this->db_path_ = js["db_path"].get<std::string>();
    this->table_name_ = js["tableName"].get<std::string>();
  } catch (std::exception& e) {
    std::stringstream ss;
    ss << "parse sqlite access info failed, " << e.what()
        << " origin access info: [" << access_info << "]";
    RaiseException(ss.str());
  }
  return retcode::SUCCESS;
}

retcode SQLiteAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  try {
    this->db_path_ = meta_info["source"].as<std::string>();
    this->table_name_ = meta_info["table_name"].as<std::string>();
  } catch (std::exception& e) {
    LOG(ERROR) << "parse sqlite access info encountes error, " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode SQLiteAccessInfo::ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) {
  auto ret{retcode::SUCCESS};
  auto& access_info = meta_info.access_info;
  if (access_info.empty()) {
    LOG(WARNING) << "access_info is empty for id: " << meta_info.id;
    return retcode::SUCCESS;
  }
  try {
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    ret = ParseFromJsonImpl(js_access_info);
  } catch (std::exception& e) {
    LOG(ERROR) << "parse access info failed";
    ret = retcode::FAIL;
  }
  return ret;
}

// sqlite cursor implementation
SQLiteCursor::SQLiteCursor(const std::string &sql,
                           std::shared_ptr<SQLiteDriver> driver) {
  this->sql_ = sql;
  this->driver_ = std::move(driver);
  auto& schema = this->driver_->dataSetAccessInfo()->Schema();
  for (size_t i = 0; i < schema.size(); i++) {
    selected_column_index_.push_back(i);
  }
}

SQLiteCursor::SQLiteCursor(const std::string& sql,
    const std::vector<int>& col_index,
    std::shared_ptr<SQLiteDriver> driver) : Cursor(col_index) {
  this->sql_ = sql;
  this->driver_ = std::move(driver);
}

SQLiteCursor::~SQLiteCursor() { this->close(); }

void SQLiteCursor::close() {}

std::shared_ptr<Dataset> SQLiteCursor::readMeta() {
  std::string query_meta_sql = sql_;
  query_meta_sql.append(" limit 100");
  VLOG(5) << "meta query sql: " << sql_;
  return readInternal(query_meta_sql);
}

// read all data from csv file
std::shared_ptr<Dataset> SQLiteCursor::read() {
  VLOG(5) << "sql_sql_sql_sql_: " << sql_;
  return readInternal(sql_);
}

std::shared_ptr<Dataset> SQLiteCursor::read(const std::shared_ptr<arrow::Schema>& data_schema) {
  return nullptr;
}

std::shared_ptr<Dataset> SQLiteCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

std::shared_ptr<Dataset> SQLiteCursor::readInternal(const std::string& query_sql) {
  std::shared_ptr<arrow::Table> table{nullptr};
  auto& db_connector = this->driver_->getDBConnector();
  if (db_connector == nullptr) {
    std::stringstream ss;
    ss << "db connector for sqlite is invalid";
    RaiseException(ss.str());
  }
  SQLite::Statement sql_query(*db_connector, query_sql);

  std::vector<std::vector<std::string>> query_result;
  query_result.resize(SelectedColumnIndex().size());
  while (sql_query.executeStep()) {
    for (int i = 0; i < sql_query.getColumnCount(); i++) {
      std::string result = sql_query.getColumn(i).getString();
      query_result[i].push_back(std::move(result));
    }
  }
  // convert data to arrow format
  auto table_schema = this->driver_->dataSetAccessInfo()->ArrowSchema();
  if (VLOG_IS_ON(5)) {
    for (const auto& name :  table_schema->field_names()) {
      VLOG(5) << "name: " << name << " "
              << "size: " << table_schema->field_names().size();
    }
  }
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  std::vector<std::shared_ptr<arrow::Field>> result_schema_filed;
  int schema_fields = table_schema->num_fields();
  auto& selected_fields = this->SelectedColumnIndex();
  size_t number_selected_fields = selected_fields.size();
  size_t i = 0;
  VLOG(5) << "selected_fields: " << number_selected_fields;
  for (const auto index : selected_fields) {
    if (index < schema_fields) {
      auto& field_ptr = table_schema->field(index);
      result_schema_filed.push_back(field_ptr);
      int field_type = field_ptr->type()->id();
      VLOG(7) << "name: " << field_ptr->name() << " type: " << field_type;
      auto array = arrow_wrapper::util::MakeArrowArray(field_type, query_result[i]);
      array_data.push_back(std::move(array));
    } else {
      std::stringstream ss;
      ss << "index out of range, current index: " << i << " "
          << "total colnum fields: " << schema_fields;
      RaiseException(ss.str());
    }
    i++;
  }
  VLOG(5) << "end of fetch data: " << array_data.size();
  auto schema = std::make_shared<arrow::Schema>(result_schema_filed);
  table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
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
    for (int i = 0; i < sql_query.getColumnCount(); i++) {
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

int SQLiteCursor::write(std::shared_ptr<Dataset> dataset) {return 0;}

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
  driver_type = kDriveType[DriverType::SQLITE];
}

std::unique_ptr<Cursor> SQLiteDriver::read() {
  auto access_info_ptr = dynamic_cast<SQLiteAccessInfo*>(this->access_info_.get());
  if (access_info_ptr == nullptr) {
    LOG(ERROR) << "sqlite access info is not unavailable";
    return nullptr;
  }
  if (this->db_connector == nullptr) {
    try {
      this->db_connector = std::make_unique<SQLite::Database>(access_info_ptr->db_path_);
    } catch (std::exception& e) {
      std::stringstream ss;
      ss << "create cursor failed: " << e.what()
          << " db path: " << access_info_ptr->db_path_;
      RaiseException(ss.str());
    }
  }
  if (access_info_ptr->Schema().empty()) {
    auto ret = GetDBTableSchema();
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "get table schema failed";
      return nullptr;
    }
  }
  std::string query_sql = buildQuerySQL(access_info_ptr);
  if (query_sql.empty()) {
    LOG(ERROR) << "get query sql failed";
    return nullptr;
  }
  return std::make_unique<SQLiteCursor>(query_sql, shared_from_this());
}

std::unique_ptr<Cursor> SQLiteDriver::read(const std::string &conn_str) {
  // return this->initCursor(conn_str);
  return nullptr;
}

std::string SQLiteDriver::buildQuerySQL(SQLiteAccessInfo* access_info) {
  auto& schema = access_info->Schema();
  if (schema.empty()) {
    RaiseException("dataset schema is empty");
    return std::string("");
  }
  auto& table_name = access_info->table_name_;
  std::string sql_str = "SELECT ";
  for (const auto& field : schema) {
    auto& field_name = std::get<0>(field);
    sql_str.append("`").append(field_name).append("`,");
  }
  sql_str[sql_str.size()-1] = ' ';
  sql_str.append("FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

std::string SQLiteDriver::buildQuerySQL(const std::string& table_name,
    const std::string& query_condition) {
  // std::string sql_str = "SELECT ";
  // if (query_condition.empty()) {
  //   sql_str.append("*");
  // } else {
  //   sql_str.append(query_condition);
  // }
  // sql_str.append(" FROM ").append(table_name);
  // VLOG(5) << "query sql: " << sql_str;
  // return sql_str;
  LOG(ERROR) << "deprecated method";
  return std::string("");
}

std::unique_ptr<Cursor> SQLiteDriver::GetCursor() {
  return nullptr;
}

std::unique_ptr<Cursor> SQLiteDriver::GetCursor(const std::vector<int>& col_index) {
  auto& access_info = this->dataSetAccessInfo();
  auto sqlite_access_info = dynamic_cast<SQLiteAccessInfo*>(access_info.get());
  if (sqlite_access_info == nullptr) {
    LOG(ERROR) << "get SQLite access info failed";
    return nullptr;
  }
  std::vector<std::string> column_names;
  std::string query_sql = BuildQuerySQL(*sqlite_access_info, col_index, &column_names);
  if (query_sql.empty()) {
    RaiseException("get query sql failed");
  }
  return std::make_unique<SQLiteCursor>(query_sql, col_index, shared_from_this());
}

std::unique_ptr<Cursor> SQLiteDriver::initCursor(const std::string &conn_str) {
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
    std::stringstream ss;
    ss << "create cursor failed: " << e.what();
    RaiseException(ss.str());
  }

  std::string &query_condition = conn_info[CONN_FIELDS::QUERY_CONDITION];
  auto sql_str = buildQuerySQL(table_name, query_condition);
  return std::make_unique<SQLiteCursor>(sql_str, shared_from_this());
}
// write data to specify table
int SQLiteDriver::write(std::shared_ptr<arrow::Table> table,
                        const std::string &table_name) {
  return 0;
}

std::string SQLiteDriver::getDataURL() const { return conn_info_; };

retcode SQLiteDriver::GetDBTableSchema() {
  auto& access_info = this->dataSetAccessInfo();
  auto sqlite_access_info = dynamic_cast<SQLiteAccessInfo*>(access_info.get());
  if (sqlite_access_info == nullptr) {
    LOG(ERROR) << "get sqlite access info failed";
    return retcode::FAIL;
  }
  std::string schema_query_sql{
      "PRAGMA table_info(" + sqlite_access_info->table_name_ + ")"};
  SQLite::Statement sql_query(*db_connector, schema_query_sql);
  nlohmann::json js_schema = nlohmann::json::array();
  while (sql_query.executeStep()) {
    int32_t cid = sql_query.getColumn(0).getInt();
    std::string column_name = sql_query.getColumn(1).getString();
    std::string column_type = sql_query.getColumn(2).getString();
    table_schema_[column_name] = column_type;
    table_cols_.push_back(column_name);
    int arrow_type;
    arrow_wrapper::util::SqlType2ArrowType(column_type, &arrow_type);
    nlohmann::json item;
    item[column_name] = arrow_type;
    js_schema.emplace_back(item);
    VLOG(5) << "cid: " << cid << " "
        << "column name: " << column_name << " "
        << "column type: " << column_type;
  }
  // fill access info
  this->dataSetAccessInfo()->ParseSchema(js_schema);
  return retcode::SUCCESS;
}

std::string SQLiteDriver::BuildQuerySQL(const SQLiteAccessInfo& access_info,
    const std::vector<int>& col_index,
    std::vector<std::string>* colum_names) {
  auto& table_name = access_info.table_name_;
  auto& schema = access_info.Schema();
  int number_fields = schema.size();
  std::string sql_str = "SELECT ";
  for (const auto index : col_index) {
    if (index < number_fields) {
      std::string col_name = std::get<0>(schema[index]);
      sql_str.append("`").append(col_name).append("`,");
    } else {
      std::stringstream ss;
      ss << "query index is out of range, "
          << "index: " << index << " total columns: " << table_cols_.size();
      RaiseException(ss.str());
    }
  }
  sql_str[sql_str.size()-1] = ' ';
  sql_str.append(" FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

} // namespace primihub
