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

#include "src/primihub/data_store/mysql/mysql_driver.h"
#include <glog/logging.h>
#include <arrow/api.h>
#include <arrow/io/api.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/thread_local_data.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub {
// MySQLAccessInfo
std::string MySQLAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["type"] = kDriveType[DriverType::MYSQL];
  js["host"] = this->ip;
  js["port"] = this->port;
  js["username"] = this->user_name;
  js["password"] = this->password;
  js["database"] = this->database;
  js["dbName"] = this->db_name;
  js["tableName"] = this->table_name;
  if (!query_colums.empty()) {
    std::string quey_col_info;
    for (const auto& col : query_colums) {
      quey_col_info.append(col).append(",");
    }
    js["query_index"] = std::move(quey_col_info);
  }
  js["dbUrl"] = this->db_url;
  js["schema"] = SchemaToJsonString();
  // ss << std::setw(4) << js;
  ss << js;
  return ss.str();
}

retcode MySQLAccessInfo::ParseFromJsonImpl(const nlohmann::json& access_info) {
  const auto& js = access_info;
  try {
    this->ip = js["host"].get<std::string>();
    this->port = js["port"].get<uint32_t>();
    this->user_name = js["username"].get<std::string>();
    this->password = js["password"].get<std::string>();
    if (js.contains("database")) {
      this->database = js["database"].get<std::string>();
    }
    this->db_name = js["dbName"].get<std::string>();
    this->table_name = js["tableName"].get<std::string>();
    if (js.contains("dbUrl")) {
      this->db_url = js["dbUrl"].get<std::string>();
    }
    this->query_colums.clear();
  } catch (std::exception& e) {
    std::stringstream ss;
    ss << "parse access info encountes error, " << e.what();
    RaiseException(ss.str());
  }
  return retcode::SUCCESS;
}

retcode MySQLAccessInfo::ParseFromMetaInfoImpl(
    const DatasetMetaInfo& meta_info) {
  auto ret{retcode::SUCCESS};
  auto& access_info = meta_info.access_info;
  if (access_info.empty()) {
    LOG(WARNING) << "access_info is empty for id: " << meta_info.id;
    return retcode::SUCCESS;
  }
  try {
    LOG(INFO) << "meta_info: " << access_info;
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    ret = ParseFromJsonImpl(js_access_info);
  } catch (std::exception& e) {
    std::stringstream ss;
    ss << "parse access info failed, " << e.what();
    RaiseException(ss.str());
  }
  return ret;
}

retcode MySQLAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  try {
    this->ip = meta_info["host"].as<std::string>();
    this->port = meta_info["port"].as<uint32_t>();
    this->user_name = meta_info["username"].as<std::string>();
    this->password = meta_info["password"].as<std::string>();
    if (meta_info["database"]) {
      this->database = meta_info["database"].as<std::string>();
    }
    this->db_name = meta_info["dbName"].as<std::string>();
    this->table_name = meta_info["tableName"].as<std::string>();
    if (meta_info["dbUrl"]) {
      this->db_url = meta_info["dbUrl"].as<std::string>();
    }
  } catch (std::exception& e) {
    size_t len = strlen(e.what());
    len = len > 1024 ? 1024 : len;
    auto err_msg = std::string(e.what(), len);
    RaiseException(err_msg);
  }
  return retcode::SUCCESS;
}

// mysql cursor implementation
auto sql_result_deleter = [](MYSQL_RES* result) {
  if (result) {
    mysql_free_result(result);
  }
};

MySQLCursor::MySQLCursor(const std::string& sql, std::shared_ptr<MySQLDriver> driver) {
  this->sql_ = sql;
  this->driver_ = driver;
  auto& schema = this->driver_->dataSetAccessInfo()->Schema();
  for (size_t i = 0; i < schema.size(); i++) {
    selected_column_index_.push_back(i);
  }
}

MySQLCursor::MySQLCursor(const std::string& sql,
    const std::vector<int>& query_colum_index,
    std::shared_ptr<MySQLDriver> driver)
    : Cursor(query_colum_index) {
  this->sql_ = sql;
  this->driver_ = driver;
}

MySQLCursor::~MySQLCursor() {
  this->close();
}

void MySQLCursor::close() {}

std::shared_ptr<arrow::Schema> MySQLCursor::makeArrowSchema() {
  auto arrow_schema = this->driver_->dataSetAccessInfo()->ArrowSchema();
  std::vector<std::shared_ptr<arrow::Field>> result_schema_filed;
  auto& selected_colum = SelectedColumnIndex();
  size_t num_fields = arrow_schema->num_fields();
  for (const auto index : selected_colum) {
    if (index < num_fields) {
      auto& field = arrow_schema->field(index);
      result_schema_filed.push_back(field);
    } else {
      std::stringstream ss;
      ss << "index out of range, current index: " << index << " "
          << "total column: " << num_fields;
      RaiseException(ss.str());
    }
  }
  return std::make_shared<arrow::Schema>(result_schema_filed);
}

auto MySQLCursor::getDBConnector(
    std::unique_ptr<DataSetAccessInfo>& access_info_ptr) ->
    std::unique_ptr<MYSQL, decltype(conn_threadsafe_dctor)> {
  auto access_info = reinterpret_cast<MySQLAccessInfo*>(access_info_ptr.get());
  const std::string& host = access_info->ip;
  const std::string& user = access_info->user_name;
  const std::string& password = access_info->password;
  const std::string& db_name = access_info->db_name;
  const uint32_t db_port = access_info->port;
  using MySqlThreadSafePtr =
      std::unique_ptr<MYSQL, decltype(conn_threadsafe_dctor)>;
  MySqlThreadSafePtr db_connector_{nullptr, conn_threadsafe_dctor};
  mysql_thread_init();
  db_connector_.reset(mysql_init(nullptr));
  int connect_timeout_ms = 3000;
  mysql_options(db_connector_.get(),
                MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout_ms);
  auto mysql_ptr = mysql_real_connect(db_connector_.get(),
      host.c_str(), user.c_str(), password.c_str(),
      db_name.c_str(), db_port, /*unix_socket*/nullptr, /*client_flag*/0);
  if (mysql_ptr == nullptr) {
    std::stringstream ss;
    ss << "connect failed:" << mysql_error(db_connector_.get());
    db_connector_.reset();
    RaiseException(ss.str());
  }
  LOG(INFO) << "connect to mysql db success";
  return db_connector_;
}

retcode MySQLCursor::fetchData(const std::string& query_sql,
    const std::shared_ptr<arrow::Schema>& table_schema,
    std::vector<std::shared_ptr<arrow::Array>>* data_arr) {
  SCopedTimer timer;
  VLOG(0) << "FetchData using Query SQL: [" << this->sql_ << "]";
  std::vector<std::vector<std::string>> result_data;
  // fetch data from db
  auto db_conn_ptr = this->getDBConnector(this->driver_->dataSetAccessInfo());
  auto db_connector = db_conn_ptr.get();
  if (query_sql.empty()) {
    RaiseException("empty query sql is invalid");
  }
  if (0 != mysql_real_query(db_connector,
                            query_sql.data(), query_sql.length())) {
    std::stringstream ss;
    ss << "query execute failed: " << mysql_error(db_connector);
    RaiseException(ss.str());
  }
  // fetch data
  using ResultPtr = std::unique_ptr<MYSQL_RES, decltype(sql_result_deleter)>;
  ResultPtr result{nullptr, sql_result_deleter};
  result.reset(mysql_use_result(db_connector));
  if (result == nullptr) {
    std::stringstream ss;
    ss << "fetch result failed: " << mysql_error(db_connector);
    RaiseException(ss.str());
  }
  uint32_t num_fields = mysql_num_fields(result.get());
  VLOG(5) << "numbers of fields: " << num_fields;
  size_t selected_fields = this->SelectedColumnIndex().size();
  if (num_fields != selected_fields) {
    std::stringstream ss;
    ss << "query column size does not match, query size: " << num_fields
        << " expected: " << selected_fields;
    RaiseException(ss.str());
  }
  result_data.resize(num_fields);
  MYSQL_ROW row;
  while (nullptr != (row = mysql_fetch_row(result.get()))) {
    unsigned long* lengths;
    lengths = mysql_fetch_lengths(result.get());
    for (uint32_t i = 0; i < num_fields; i++) {
      std::string item{"NA"};
      if (lengths[i] > 0) {
        item = std::string(row[i], lengths[i]);
      }
      // VLOG(7) << "fetch item: " << item << " length: " << lengths[i];
      result_data[i].push_back(std::move(item));
    }
  }
  auto fetch_cost = timer.timeElapse();
  VLOG(5) << "mysql_fetch_row cost(ms): " << fetch_cost;
  auto start_ = timer.timeElapse();
  int schema_fields = table_schema->num_fields();
  auto& all_select_index = this->SelectedColumnIndex();
  for (size_t i = 0; i < selected_fields; i++) {
    // int index = all_select_index[i];
    if (i < schema_fields) {
      auto& field_ptr = table_schema->field(i);
      int field_type = field_ptr->type()->id();
      VLOG(7) << "field_name: " << field_ptr->name() << " type: " << field_type;
      auto array = arrow_wrapper::util::MakeArrowArray(field_type, result_data[i]);
      data_arr->push_back(std::move(array));
    } else {
      std::stringstream ss;
      ss << "index out of range, current index: " << i << " "
          << "total colnum fields: " << schema_fields;
      RaiseException(ss.str());
    }
  }
  auto end_ = timer.timeElapse();
  auto buid_data_cost = end_ - start_;
  VLOG(5) << "build arrow data time cost(ms): " << buid_data_cost
          << " total cost(ms): " << end_;

  VLOG(5) << "end of fetch data: " << data_arr->size();
  return retcode::SUCCESS;
}

std::shared_ptr<Dataset> MySQLCursor::readMeta() {
  std::string meta_query_sql = this->sql_;
  meta_query_sql.append(" LIMIT 10");
  auto schema = makeArrowSchema();
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  auto table_schema = this->driver_->dataSetAccessInfo()->ArrowSchema();
  auto ret = fetchData(meta_query_sql, table_schema, &array_data);
  if (ret != retcode::SUCCESS) {
      return nullptr;
  }
  auto table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}

// read all data from mysql
std::shared_ptr<Dataset> MySQLCursor::read() {
  auto schema = makeArrowSchema();
  return ReadImpl(schema);
}

std::shared_ptr<Dataset> MySQLCursor::read(
    const std::shared_ptr<arrow::Schema>& data_schema) {
  return ReadImpl(data_schema);
}

std::shared_ptr<Dataset> MySQLCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

std::shared_ptr<Dataset> MySQLCursor::ReadImpl(
    const std::shared_ptr<arrow::Schema>& schema) {
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  VLOG(5) << "sql_sql_sql_: " << this->sql_;
  auto ret = fetchData(this->sql_, schema, &array_data);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "fetchdata failed using sql: " << this->sql_;
    return nullptr;
  }
  auto table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}


int MySQLCursor::write(std::shared_ptr<Dataset> dataset) {}

// ======== MySQL Driver implementation ========
MySQLDriver::MySQLDriver(const std::string& nodelet_addr)
        : DataDriver(nodelet_addr) {
  setDriverType();
  initMySqlLib();
}

MySQLDriver::MySQLDriver(const std::string& nodelet_addr,
                         std::unique_ptr<DataSetAccessInfo> access_info)
                         : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
  initMySqlLib();
}

MySQLDriver::~MySQLDriver() {
  releaseMySqlLib();
}

void MySQLDriver::setDriverType() {
  driver_type = kDriveType[DriverType::MYSQL];;
}

retcode MySQLDriver::releaseMySqlLib() {
  // mysql_library_end();
  return retcode::SUCCESS;
}

retcode MySQLDriver::initMySqlLib() {
  mysql_library_init(0, nullptr, nullptr);
  return retcode::SUCCESS;
}

std::string MySQLDriver::getMySqlError() {
  return std::string(mysql_error(db_connector_.get()));
}

retcode MySQLDriver::connect(MySQLAccessInfo* access_info) {
  const auto& host = access_info->ip;
  const auto& user = access_info->user_name;
  const auto& password = access_info->password;
  const auto& db_name = access_info->db_name;
  const uint32_t db_port = access_info->port;
  if (connected.load()) {
    return retcode::SUCCESS;
  }
  db_connector_.reset(mysql_init(nullptr));
  mysql_options(db_connector_.get(),
                MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout_ms);
  auto mysql_handler = mysql_real_connect(db_connector_.get(), host.c_str(),
      user.c_str(), password.c_str(),
      db_name.c_str(), db_port, /*unix_socket*/nullptr, /*client_flag*/0);
  if (mysql_handler == nullptr) {
    std::stringstream ss;
    ss << "connect failed, " << getMySqlError();
    RaiseException(ss.str());
  }
  LOG(INFO) << "connect to mysql db success";
  connected.store(true);
  return retcode::SUCCESS;
}

bool MySQLDriver::isConnected() {
  return true;
}

bool MySQLDriver::reConnect() {
  return true;
}

retcode MySQLDriver::executeQuery(const std::string& sql_query) {
  if (sql_query.empty()) {
    LOG(ERROR) << "query sql is invalid: ";
    return retcode::FAIL;
  }
  if (0 != mysql_real_query(this->db_connector_.get(),
                            sql_query.data(), sql_query.length())) {
    std::stringstream ss;
    ss << "query execute failed: " << getMySqlError();
    RaiseException(ss.str());
  }
  return retcode::SUCCESS;
}

std::string MySQLDriver::BuildQuerySQL(const MySQLAccessInfo& access_info,
                                       const std::vector<int>& col_index,
                                      std::vector<std::string>* col_names_ptr) {
  const auto& table_name = access_info.table_name;
  const auto& query_cols = access_info.query_colums;
  auto& column_names = *col_names_ptr;
  column_names.clear();
  std::string sql_str = "SELECT ";
  auto& schema = access_info.schema;
  for (const auto index : col_index) {
    if (index < schema.size()) {
      auto& col_name = std::get<0>(schema[index]);
      sql_str.append("`").append(col_name).append("`,");
      column_names.push_back(col_name);
    } else {
      std::stringstream ss;
      ss << "index is out of range, "
         << "current: " << index << " total column: " << schema.size();
      RaiseException(ss.str());
    }
  }

  // remove the last ','
  sql_str[sql_str.size()-1] = ' ';
  sql_str.append(" FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

std::string MySQLDriver::buildQuerySQL(MySQLAccessInfo* access_info) {
  const auto& table_name = access_info->table_name;
  auto& query_cols = access_info->query_colums;
  std::string sql_str = "SELECT ";
  auto& schema = access_info->schema;
  for (const auto& column : schema) {
    auto& col_name = std::get<0>(column);
    sql_str.append("`").append(col_name).append("`,");
  }
  // remove the last ','
  sql_str[sql_str.size()-1] = ' ';
  sql_str.append(" FROM ").append(table_name);
  VLOG(5) << "query sql: " << sql_str;
  return sql_str;
}

retcode MySQLDriver::getTableSchema(const std::string& db_name,
                                    const std::string& table_name) {
  if (!this->isConnected()) {
      this->reConnect();
  }
  std::string sql_table_schema{
      "SELECT `column_name`, `data_type` FROM "
      "information_schema.COLUMNS WHERE TABLE_NAME = '"};
  sql_table_schema.append(table_name);
  sql_table_schema.append("' and TABLE_SCHEMA = '");
  sql_table_schema.append(db_name);
  sql_table_schema.append("' ORDER BY column_name ASC;");
  auto ret = executeQuery(sql_table_schema);
  if (ret != retcode::SUCCESS) {
    return retcode::FAIL;
  }
  // get table schema
  // result = mysql_store_result(mysql);

  using result_t = std::unique_ptr<MYSQL_RES, decltype(sql_result_deleter)>;
  result_t result{nullptr, sql_result_deleter};
  result.reset(mysql_use_result(this->db_connector_.get()));

  if (result == nullptr) {
    std::stringstream ss;
    ss << "fetch result failed: " << getMySqlError();
    RaiseException(ss.str());
  }

  uint32_t num_fields = mysql_num_fields(result.get());
  VLOG(5) << "numbers of result: " << num_fields;
  if (num_fields != 2) { // we just query 2 fields
    std::stringstream ss;
    ss << "2 num_fields is expected, but get " << num_fields;
    RaiseException(ss.str());
  }
  MYSQL_ROW row;
  table_cols_.clear();
  table_schema_.clear();
  nlohmann::json js_schema = nlohmann::json::array();
  while (nullptr != (row = mysql_fetch_row(result.get()))) {
    unsigned long* lengths;
    lengths = mysql_fetch_lengths(result.get());
    std::string column_name = std::string(row[0], lengths[0]);
    std::string data_type = std::string(row[1], lengths[1]);
    VLOG(5) << "column_name: " << column_name << " "
            << "data_type: " << data_type;
    int arrow_type;
    arrow_wrapper::util::SqlType2ArrowType(data_type, &arrow_type);
    nlohmann::json item;
    item[column_name] = arrow_type;
    js_schema.emplace_back(item);
    table_cols_.push_back(column_name);
    table_schema_.insert({std::move(column_name), std::move(data_type)});
  }
  // fill access info
  this->dataSetAccessInfo()->ParseSchema(js_schema);
  return retcode::SUCCESS;
}

std::unique_ptr<Cursor> MySQLDriver::read() {
  return GetCursor();
}

std::unique_ptr<Cursor> MySQLDriver::read(const std::string &conn_str) {
  return this->initCursor(conn_str);
}

std::unique_ptr<Cursor> MySQLDriver::GetCursor() {
  auto access_info_ptr = dynamic_cast<MySQLAccessInfo*>(this->access_info_.get());
  if (access_info_ptr == nullptr) {
    RaiseException("access info for mysql is not available");
  }
  VLOG(5) << "access_info_ptr schema column size: "
          << access_info_ptr->schema.size();
  if (access_info_ptr->Schema().empty()) {
    // first connect to db
    auto ret = this->connect(access_info_ptr);
    if (ret != retcode::SUCCESS) {
      RaiseException("connect to mysql failed");
    }
    std::string sql_change_db{"USE "};
    sql_change_db.append(access_info_ptr->db_name);
    this->executeQuery(sql_change_db);
    ret = this->getTableSchema(access_info_ptr->db_name,
                               access_info_ptr->table_name);
    if (ret != retcode::SUCCESS) {
      return nullptr;
    }
  }
  std::string query_sql = buildQuerySQL(access_info_ptr);
  return std::make_unique<MySQLCursor>(query_sql, shared_from_this());
}

std::unique_ptr<Cursor> MySQLDriver::GetCursor(
    const std::vector<int>& col_index) {
  // first connect to db
  auto access_info_ptr =
      dynamic_cast<MySQLAccessInfo*>(this->access_info_.get());
  if (access_info_ptr == nullptr) {
    RaiseException("access info for mysql is not available");
  }
  if (access_info_ptr->Schema().empty()) {
    auto ret = this->connect(access_info_ptr);
    if (ret != retcode::SUCCESS) {
      return nullptr;
    }
    std::string sql_change_db{"USE "};
    sql_change_db.append(access_info_ptr->db_name);
    this->executeQuery(sql_change_db);
    ret = this->getTableSchema(access_info_ptr->db_name,
                               access_info_ptr->table_name);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "getTableSchema failed";
      return nullptr;
    }
  }
  std::vector<std::string> colum_names;
  std::string query_sql = BuildQuerySQL(*access_info_ptr,
                                        col_index, &colum_names);
  if (query_sql.empty()) {
    RaiseException("query_sql is empty");
  }
  return std::make_unique<MySQLCursor>(query_sql, col_index, shared_from_this());
}

std::unique_ptr<Cursor> MySQLDriver::initCursor(const std::string &conn_str) {
  std::string sql_str;
  return std::make_unique<MySQLCursor>(sql_str, shared_from_this());
}

std::unique_ptr<Cursor> MySQLDriver::MakeCursor(std::vector<int> col_index) {
  // first connect to db
  auto access_info = dynamic_cast<MySQLAccessInfo*>(this->access_info_.get());
  if (access_info == nullptr) {
    RaiseException("access info for mysql is not available");
  }
  auto ret = this->connect(access_info);
  if (ret != retcode::SUCCESS) {
    return nullptr;
  }
  std::string sql_change_db{"USE "};
  sql_change_db.append(access_info->db_name);
  this->executeQuery(sql_change_db);
  ret = this->getTableSchema(access_info->db_name, access_info->table_name);
  if (ret != retcode::SUCCESS) {
    return nullptr;
  }
  std::string query_sql = buildQuerySQL(access_info);
  return std::make_unique<MySQLCursor>(query_sql, shared_from_this());
}

// write data to specify table
int MySQLDriver::write(std::shared_ptr<arrow::Table> table,
                       const std::string &table_name) {
  return 0;
}

std::string MySQLDriver::getDataURL() const {
  return conn_info_;
};

} // namespace primihub
