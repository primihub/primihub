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

#include "src/primihub/data_store/seatunnel/seatunnel_driver.h"
#include <sys/stat.h>
#include <glog/logging.h>

#include <fstream>
#include <variant>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <future>
#include <nlohmann/json.hpp>
#include "arrow/io/memory.h"

#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/thread_local_data.h"
#include "pybind11/embed.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/config/server_config.h"

namespace py = pybind11;
namespace primihub {
// SeatunnelAccessInfo
std::string SeatunnelAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["type"] = "seatunnel";
  js["host"] = this->ip;
  js["port"] = this->port;
  js["username"] = this->user_name;
  js["password"] = this->password;
  js["database"] = this->database;
  js["dbName"] = this->db_name;
  js["tableName"] = this->table_name;
  js["dbUrl"] = this->db_url;
  js["dbDriver"] = this->db_driver_;
  js["source_type"] = this->source_type;
  js["schema"] = SchemaToJsonString();
  // ss << std::setw(4) << js;
  ss << js;
  return ss.str();
}

retcode SeatunnelAccessInfo::ParseFromJsonImpl(
    const nlohmann::json& meta_info) {
  const auto& js = meta_info;
  try {
    this->ip = js["host"].get<std::string>();
    this->port = js["port"].get<uint32_t>();
    this->user_name = js["username"].get<std::string>();
    this->password = js["password"].get<std::string>();
    this->db_name = js["dbName"].get<std::string>();
    this->table_name = js["tableName"].get<std::string>();
    if (js.contains("dbUrl")) {
      this->db_url = js["dbUrl"].get<std::string>();
    }
    if (js.contains("dbDriver")) {
      this->db_driver_ = js["dbDriver"].get<std::string>();
    }
    if (js.contains("source_type")) {
      this->source_type = js["source_type"].get<std::string>();
    } else if (js.contains("type")) {
      this->source_type = js["type"].get<std::string>();
    }

  } catch (std::exception& e) {
    std::stringstream ss;
    ss << "parse access info encountes error, " << e.what();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode SeatunnelAccessInfo::ParseFromYamlConfigImpl(
    const YAML::Node& meta_info) {
  try {
    this->ip = meta_info["host"].as<std::string>();
    this->port = meta_info["port"].as<uint32_t>();
    this->user_name = meta_info["username"].as<std::string>();
    this->password = meta_info["password"].as<std::string>();
    this->db_name = meta_info["dbName"].as<std::string>();
    this->table_name = meta_info["tableName"].as<std::string>();
    if (meta_info["dbUrl"]) {
      this->db_url = meta_info["dbUrl"].as<std::string>();
    }
    if (meta_info["dbDriver"]) {
      this->db_driver_ = meta_info["dbDriver"].as<std::string>();
    }
    this->source_type = meta_info["source_type"].as<std::string>();
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode SeatunnelAccessInfo::ParseFromMetaInfoImpl(
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
    LOG(INFO) << "meta_infoxxxxxxxx: " << access_info;
    ret = ParseFromJsonImpl(js_access_info);
  } catch (std::exception& e) {
    std::stringstream ss;
    ss << "parse access info failed, " << e.what();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    ret = retcode::FAIL;
  }
  return ret;
}

// csv cursor implementation
SeatunnelCursor::SeatunnelCursor(const std::string& sql,
                                 std::shared_ptr<SeatunnelDriver> driver) {
  this->sql_ = sql;
  this->driver_ = driver;
}

SeatunnelCursor::SeatunnelCursor(const std::vector<int>& query_colum_index,
                                 std::shared_ptr<SeatunnelDriver> driver):
                                 Cursor(query_colum_index) {
  this->driver_ = driver;
}

SeatunnelCursor::~SeatunnelCursor() { this->close(); }

void SeatunnelCursor::close() {}

std::shared_ptr<Dataset> SeatunnelCursor::readMeta() {
  auto& access_info = this->driver_->dataSetAccessInfo();
  auto arrow_scheam = access_info->ArrowSchema();
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  auto table = arrow::Table::Make(arrow_scheam, array_data);
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> SeatunnelCursor::read(
    const std::shared_ptr<arrow::Schema>& data_schema) {
  // auto fut = std::async(
  //   std::launch::async,
  //   [&]() -> std::shared_ptr<arrow::Table> {
  //     auto link_mode = primihub::network::LinkMode::GRPC;
  //     auto link_ctx = primihub::network::LinkFactory::createLinkContext(link_mode);
  //     auto& ins = ServerConfig::getInstance();
  //     auto& proxy_node = ins.ProxyServerCfg();
  //     auto channel = link_ctx->getChannel(proxy_node);
  //     if (channel == nullptr) {
  //       LOG(ERROR) << "link_ctx->getChannel(peer_node); failed";
  //       return nullptr;
  //     }
  //     VLOG(5) << "begin to FetchData";
  //     std::string request_id = this->TraceId();
  //     std::string dataset_id = this->DatasetId();
  //     auto table = channel->FetchData(request_id, dataset_id, data_schema);
  //     if (table == nullptr) {
  //       LOG(ERROR) << this->TraceId() << " FetchData failed";
  //       return nullptr;
  //     }
  //     return table;
  //   });

  std::string new_task_content;
  std::string seatunnel_config;
  std::string task_content;
  auto& ins = ServerConfig::getInstance();
  auto& config_file_path = ins.SeatunnelConfigFile();
  auto& task_template_file = ins.SeatunnelTaskTemplateFile();
  auto ret = primihub::ReadFileContents(config_file_path, &seatunnel_config);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << this->TraceId() << " read task configure from file: "
              << config_file_path << " failed";
    return nullptr;
  }
  ret = primihub::ReadFileContents(task_template_file, &task_content);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << this->TraceId()
        << " read task template from template file: "
        << task_template_file << " failed";
    return nullptr;
  }
  try {
    auto& proxy_node = ins.ProxyServerCfg();
    LOG(ERROR) << "proxy_node: " << proxy_node.to_string();
    nlohmann::json j_task_config = nlohmann::json::parse(task_content);
    auto& j_sink = j_task_config["sink"];
    for (auto& item : j_sink) {
      item["dataSetId"] = this->DatasetId();
      item["traceId"] = this->TraceId();
      item["host"] = proxy_node.ip();
      item["port"] = proxy_node.port();
    }
    auto& j_source = j_task_config["source"];
    auto access_info = dynamic_cast<SeatunnelAccessInfo*>(
        this->driver_->dataSetAccessInfo().get());
    if (access_info == nullptr) {
      LOG(ERROR) << this->TraceId() << " access info is invalid";
      return nullptr;
    }
    std::string query_sql;
    auto ret = BuildQuerySql(data_schema, *access_info, -1, &query_sql);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "BuildQuerySql failed";
      return nullptr;
    }
    for (auto& item : j_source) {
      item["password"] = access_info->password;
      item["user"] = access_info->user_name;
      item["url"] = access_info->db_url;
      item["driver"] = access_info->db_driver_;
      item["query"] = query_sql;
    }

    new_task_content = j_task_config.dump();
    LOG(ERROR) << "new_task_content: " << new_task_content;
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return nullptr;
  }
  {
    py::gil_scoped_acquire acquire;
    VLOG(1) << "<<<<<<<<< Trigger get dataset  <<<<<<<<<";
    py::object SeatunnelClient =
        py::module::import("primihub.client.seatunnel_client")
            .attr("SeatunnelClient");
    py::object client = SeatunnelClient(py::bytes(seatunnel_config));
    py::object submit_task = client.attr("SubmitTask");
    submit_task(py::bytes(new_task_content));
    submit_task.release();
    client.release();
    SeatunnelClient.release();
    VLOG(1) << "<<<<<<<<< end of Trigger get dataset  <<<<<<<<<";
  }

  std::shared_ptr<arrow::Table> table{nullptr};
  {
    auto link_mode = primihub::network::LinkMode::GRPC;
    auto link_ctx = primihub::network::LinkFactory::createLinkContext(link_mode);
    auto& ins = ServerConfig::getInstance();
    auto& proxy_node = ins.ProxyServerCfg();
    auto channel = link_ctx->getChannel(proxy_node);
    if (channel == nullptr) {
      LOG(ERROR) << "link_ctx->getChannel(peer_node); failed";
      return nullptr;
    }
    VLOG(5) << "begin to FetchData";
    std::string request_id = this->TraceId();
    std::string dataset_id = this->DatasetId();
    VLOG(0) << "Receive data from seatunel success, "
            << "fetch data from proxy node: " << proxy_node.to_string();
    table = channel->FetchData(request_id, dataset_id, data_schema);
    if (table == nullptr) {
      LOG(ERROR) << this->TraceId() << " FetchData failed";
      return nullptr;
    }
  }
  // table = fut.get();
  if (table == nullptr) {
    LOG(ERROR) << "get data failed";
    return nullptr;
  }
  LOG(INFO) << this->TraceId() << " FetchData finished:";
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<arrow::Schema> SeatunnelCursor::MakeArrowSchema() {
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
      std::string err_msg = ss.str();
      SetThreadLocalErrorMsg(err_msg);
      LOG(ERROR) << err_msg;
      return nullptr;
    }
  }
  return std::make_shared<arrow::Schema>(result_schema_filed);
}

// read all data according Seatunnel
std::shared_ptr<Dataset> SeatunnelCursor::read() {
  auto arrow_schema = MakeArrowSchema();
  if (arrow_schema == nullptr) {
    LOG(ERROR) << "MakeArrowSchema failed";
    return nullptr;
  }
  return read(arrow_schema);
}

std::shared_ptr<Dataset> SeatunnelCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

int SeatunnelCursor::write(std::shared_ptr<Dataset> dataset) {
  return 0;
}

retcode SeatunnelCursor::BuildQuerySql(
    const std::shared_ptr<arrow::Schema>& data_schema,
    const SeatunnelAccessInfo& access_info,
    int64_t query_limit,
    std::string* query_sql) {
  std::vector<std::string> field_names = data_schema->field_names();
  if (field_names.empty()) {
    LOG(ERROR) << "no field is selected";
    return retcode::FAIL;
  }
  const auto& source_type = access_info.source_type;
  std::string low_cast_source_type = strToLower(source_type);
  auto ret{retcode::SUCCESS};
  if (source_type == "mysql") {
    ret = MysqlBuildQuerySql(field_names, access_info, query_limit, query_sql);
  } else if (source_type == "dm") {
    ret = DmBuildQuerySql(field_names, access_info, query_limit, query_sql);
  } else if (source_type == "sqlserver") {
    ret = SqlServerBuildQuerySql(field_names, access_info,
                                 query_limit, query_sql);
  } else if (source_type == "oracle") {
    ret = OracleBuildQuerySql(field_names, access_info, query_limit, query_sql);
  } else if (source_type == "hive" || source_type == "hive2") {
    ret = HiveBuildQuerySql(field_names, access_info, query_limit, query_sql);
  } else {
    LOG(ERROR) << "unsupported source type: " << source_type;
    ret = retcode::FAIL;
  }
  return ret;
}

retcode SeatunnelCursor::MysqlBuildQuerySql(
    const std::vector<std::string>& field_names,
    const SeatunnelAccessInfo& access_info,
    int64_t query_limit,
    std::string* query_sql) {
  std::string sql = "SELECT ";
  for (const auto& name : field_names) {
    sql.append("`").append(name).append("`,");
  }
  sql[sql.length()-1] = ' ';
  sql.append("FROM ")
      .append("`").append(access_info.db_name).append("`.")    // db name
      .append("`").append(access_info.table_name).append("`");    // tablename
  if (query_limit > 0) {
    sql.append(" LIMIT ").append(std::to_string(query_limit));
  }
  *query_sql = std::move(sql);
  LOG(INFO) << "MysqlBuildQuerySql: " << *query_sql;
  return retcode::SUCCESS;
}

retcode SeatunnelCursor::DmBuildQuerySql(
    const std::vector<std::string>& field_names,
    const SeatunnelAccessInfo& access_info,
    int64_t query_limit,
    std::string* query_sql) {
  std::string sql = "SELECT ";
  if (query_limit > 0) {
    sql.append("TOP ").append(std::to_string(query_limit)).append(" ");
  }
  for (const auto& name : field_names) {
    sql.append(name).append(",");
  }
  sql[sql.length()-1] = ' ';
  sql.append("FROM ")
      .append("\"").append(access_info.db_name).append("\".")    // db name
      .append("\"").append(access_info.table_name).append("\""); // tablename
  *query_sql = std::move(sql);
  LOG(INFO) << "DmBuildQuerySql: " << *query_sql;
  return retcode::SUCCESS;
}

retcode SeatunnelCursor::SqlServerBuildQuerySql(
      const std::vector<std::string>& field_names,
      const SeatunnelAccessInfo& access_info,
      int64_t query_limit,
      std::string* query_sql) {
  std::string sql = "SELECT ";
  if (query_limit > 0) {
    sql.append("TOP ").append(std::to_string(query_limit)).append(" ");
  }
  for (const auto& name : field_names) {
    sql.append(name).append(",");
  }
  sql[sql.length()-1] = ' ';
  sql.append("FROM ")
     .append(access_info.table_name); // tablename
  *query_sql = std::move(sql);
  LOG(INFO) << "SqlServerBuildQuerySql: " << *query_sql;
  return retcode::SUCCESS;
}

retcode SeatunnelCursor::OracleBuildQuerySql(
    const std::vector<std::string>& field_names,
    const SeatunnelAccessInfo& access_info,
    int64_t query_limit,
    std::string* query_sql) {
//
  std::string sql = "SELECT ";
  for (const auto& name : field_names) {
    sql.append(name).append(",");
  }
  sql[sql.length()-1] = ' ';
  sql.append("FROM ")
     .append(access_info.table_name);    // tablename
  if (query_limit > 0) {
    sql.append(" WHERE rownum <= ").append(std::to_string(query_limit));
  }
  *query_sql = std::move(sql);
  LOG(INFO) << "OracleBuildQuerySql: " << *query_sql;
  return retcode::SUCCESS;
}

retcode SeatunnelCursor::HiveBuildQuerySql(
    const std::vector<std::string>& field_names,
    const SeatunnelAccessInfo& access_info,
    int64_t query_limit,
    std::string* query_sql) {
  std::string sql = "SELECT ";
  for (const auto& name : field_names) {
    sql.append(name).append(",");
  }
  sql[sql.length()-1] = ' ';
  sql.append("FROM ")
     .append(access_info.table_name);    // tablename
  if (query_limit > 0) {
    sql.append(" LIMIT ").append(std::to_string(query_limit));
  }
  *query_sql = std::move(sql);
  LOG(INFO) << "HiveBuildQuerySql: " << *query_sql;
  return retcode::SUCCESS;
}
// ======== Seatunnel Driver implementation ========

SeatunnelDriver::SeatunnelDriver(const std::string &nodelet_addr)
    : DataDriver(nodelet_addr) {
  setDriverType();
}

SeatunnelDriver::SeatunnelDriver(const std::string &nodelet_addr,
    std::unique_ptr<DataSetAccessInfo> access_info)
    : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
}

void SeatunnelDriver::setDriverType() {
  driver_type = "Seatunnel";
}

std::unique_ptr<Cursor> SeatunnelDriver::read() {
  return GetCursor();
}

std::unique_ptr<Cursor> SeatunnelDriver::read(const std::string &filePath) {
  return this->initCursor(filePath);
}

std::unique_ptr<Cursor> SeatunnelDriver::GetCursor() {
  return std::make_unique<SeatunnelCursor>(shared_from_this());
}

std::unique_ptr<Cursor> SeatunnelDriver::GetCursor(
    const std::vector<int>& col_index) {
  auto access_info =
      dynamic_cast<SeatunnelAccessInfo*>(this->access_info_.get());
  if (access_info == nullptr) {
    LOG(ERROR) << "file access info is unavailable";
    return nullptr;
  }
  // return std::make_unique<SeatunnelCursor>(col_index, shared_from_this());
  // filePath_ = access_info->file_path_;
  // return std::make_unique<SeatunnelCursor>(filePath_, col_index, shared_from_this());
  // TODO(cuibo)
  return std::make_unique<SeatunnelCursor>(shared_from_this());
}

std::unique_ptr<Cursor> SeatunnelDriver::initCursor(
    const std::string &filePath) {
  // filePath_ = filePath;
  // return std::make_unique<SeatunnelDriver>(filePath, shared_from_this());
  return nullptr;
}

int SeatunnelDriver::write(std::shared_ptr<arrow::Table> table,
                     const std::string& file_path) {
  return 0;
}

retcode SeatunnelDriver::Write(const std::vector<std::string>& fields_name,
                         std::shared_ptr<arrow::Table> table,
                         const std::string& file_path) {
  return retcode::SUCCESS;
}

std::string SeatunnelDriver::getDataURL() const {
  return filePath_;
}

}  // namespace primihub
