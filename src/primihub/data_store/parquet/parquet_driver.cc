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
#include <nlohmann/json.hpp>
#include "arrow/io/memory.h"
#include "arrow/compute/api.h"

#include "src/primihub/data_store/parquet/parquet_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/thread_local_data.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub {
// ParquetAccessInfo
std::string ParquetAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["type"] = kDriveType[DriverType::PARQUET];
  js["data_path"] = this->file_path_;
  js["schema"] = SchemaToJsonString();
  ss << js;
  return ss.str();
}

retcode ParquetAccessInfo::fromJsonString(const std::string& access_info) {
  retcode ret{retcode::SUCCESS};
  try {
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    if (js_access_info.contains("schema")) {
      auto schema_json =
          nlohmann::json::parse(js_access_info["schema"].get<std::string>());
      ret = ParseSchema(schema_json);
    }
    ret = ParseFromJsonImpl(js_access_info);
  } catch (std::exception& e) {
    LOG(WARNING) << "parse access info from json string failed, reason ["
        << e.what() << "] "
        << "item: " << access_info;
    this->file_path_ = access_info;
  }
  return ret;
}

retcode ParquetAccessInfo::ParseFromJsonImpl(const nlohmann::json& meta_info) {
  try {
    // this->file_path_ = access_info["access_meta"];
    std::string access_info = meta_info["access_meta"].get<std::string>();
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    this->file_path_ = js_access_info["data_path"].get<std::string>();
  } catch (std::exception& e) {
    this->file_path_ = meta_info["access_meta"];
    if (this->file_path_.empty()) {
      std::stringstream ss;
      ss << "get dataset path failed, " << e.what() << " "
          << "detail: " << meta_info;
      RaiseException(ss.str());
    }
  }
  return retcode::SUCCESS;
}

retcode ParquetAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  this->file_path_ = meta_info["source"].as<std::string>();
  return retcode::SUCCESS;
}

retcode ParquetAccessInfo::ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) {
  auto& access_info = meta_info.access_info;
  if (access_info.empty()) {
    LOG(WARNING) << "no access info for " << meta_info.id;
    return retcode::SUCCESS;
  }
  try {
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    this->file_path_ = js_access_info["data_path"].get<std::string>();
  } catch (std::exception& e) {
    this->file_path_ = access_info;
    // check validattion of the path
    std::ifstream csv_data(file_path_, std::ios::in);
    if (!csv_data.is_open()) {
      std::stringstream ss;
      ss << "file_path: " << file_path_ << " is not exist";
      RaiseException(ss.str());
    }
    return retcode::SUCCESS;
  }
  return retcode::SUCCESS;
}

// parquet cursor implementation
ParquetCursor::ParquetCursor(const std::string& file_path,
                     std::shared_ptr<ParquetDriver> driver) {
  this->file_path_ = file_path;
  this->driver_ = driver;
}

ParquetCursor::ParquetCursor(const std::string& file_path,
                              const std::vector<int>& colnum_index,
                              std::shared_ptr<ParquetDriver> driver)
                              : Cursor(colnum_index) {
  this->file_path_ = file_path;
  this->driver_ = driver;
}

ParquetCursor::~ParquetCursor() {
  this->close();
}

void ParquetCursor::close() {
}

std::shared_ptr<Dataset> ParquetCursor::readMeta() {
  arrow::Status st;
  auto _start = std::chrono::high_resolution_clock::now();
  arrow::MemoryPool* pool = arrow::default_memory_pool();
  std::shared_ptr<arrow::io::RandomAccessFile> input =
      arrow::io::ReadableFile::Open(file_path_.c_str()).ValueOrDie();

  // Open Parquet file reader
  std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
  st = parquet::arrow::OpenFile(input, pool, &arrow_reader);
  if (!st.ok()) {
    // Handle error instantiating file reader...
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  arrow_reader->set_use_threads(true);
  std::shared_ptr<::arrow::Schema> schema;
  st = arrow_reader->GetSchema(&schema);
  if (!st.ok()) {
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  std::vector<std::shared_ptr<arrow::Array>> array_data;
  auto table = arrow::Table::Make(schema, array_data);
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> ParquetCursor::read(
    const std::shared_ptr<arrow::Schema>& data_schema) {
  arrow::Status st;
  auto _start = std::chrono::high_resolution_clock::now();
  arrow::MemoryPool* pool = arrow::default_memory_pool();
  std::shared_ptr<arrow::io::RandomAccessFile> input =
      arrow::io::ReadableFile::Open(file_path_.c_str()).ValueOrDie();

  // Open Parquet file reader
  std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
  st = parquet::arrow::OpenFile(input, pool, &arrow_reader);
  if (!st.ok()) {
    // Handle error instantiating file reader...
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  arrow_reader->set_use_threads(true);
  std::shared_ptr<::arrow::Schema> schema;
  st = arrow_reader->GetSchema(&schema);
  if (!st.ok()) {

  }
  // Read entire file as a single Arrow table
  std::shared_ptr<arrow::Table> table;
  auto& selected_col = SelectedColumnIndex();
  LOG(ERROR) << "selected_col: " << selected_col.size();
  st = arrow_reader->ReadTable(selected_col, &table);
  if (!st.ok()) {
    // Handle error reading Parquet data...
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  auto src_field = table->fields();
  auto& dest_field = data_schema->fields();
  std::map<int, std::shared_ptr<arrow::Array>> casted_colum;
  for (size_t i = 0; i < table->schema()->num_fields(); i++) {
    auto src_id = src_field[i]->type()->id();
    auto dest_id = dest_field[i]->type()->id();
    if (src_id == dest_id) {
      continue;
    }
    auto chunk_arr = table->column(i);
    auto src_arr = arrow::Concatenate(chunk_arr->chunks(),
                                      arrow::default_memory_pool()).ValueOrDie();
    arrow::compute::ExecContext exec_context(arrow::default_memory_pool());
    arrow::compute::CastOptions cast_options;

    // 执行转换
    std::shared_ptr<arrow::DataType> output_type = dest_field[i]->type();
    std::shared_ptr<arrow::Array> dest_array =
        arrow::compute::Cast(*src_arr, output_type, cast_options, &exec_context).ValueOrDie();
    casted_colum[i] = dest_array;
  }
  if (!casted_colum.empty()) {
    LOG(INFO) << "casted_colum: " << casted_colum.size();
    std::vector<std::shared_ptr<arrow::Array>> new_columns;
    for (size_t i = 0; i < table->schema()->num_fields(); i++) {
      if (casted_colum.find(i) != casted_colum.end()) {
        new_columns.push_back(casted_colum[i]);
      } else {
        auto chunk_arr = table->column(i);
        auto src_arr = arrow::Concatenate(chunk_arr->chunks(),
                                          arrow::default_memory_pool()).ValueOrDie();
        new_columns.push_back(src_arr);
      }
    }
    auto new_table = arrow::Table::Make(data_schema, new_columns, table->num_rows());
    auto dataset = std::make_shared<Dataset>(new_table, this->driver_);
    return dataset;
  } else {
    auto dataset = std::make_shared<Dataset>(table, this->driver_);
    return dataset;
  }
}

// read all data from csv file
std::shared_ptr<Dataset> ParquetCursor::read() {
  arrow::Status st;
  auto _start = std::chrono::high_resolution_clock::now();
  arrow::MemoryPool* pool = arrow::default_memory_pool();
  std::shared_ptr<arrow::io::RandomAccessFile> input =
      arrow::io::ReadableFile::Open(file_path_.c_str()).ValueOrDie();

  // Open Parquet file reader
  std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
  st = parquet::arrow::OpenFile(input, pool, &arrow_reader);
  if (!st.ok()) {
    // Handle error instantiating file reader...
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  arrow_reader->set_use_threads(true);
  std::shared_ptr<::arrow::Schema> schema;
  st = arrow_reader->GetSchema(&schema);
  if (!st.ok()) {
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  // Read entire file as a single Arrow table
  std::shared_ptr<arrow::Table> table;
  auto& selected_col = SelectedColumnIndex();
  LOG(ERROR) << "selected_col: " << selected_col.size();
  st = arrow_reader->ReadTable(selected_col, &table);
  if (!st.ok()) {
    // Handle error reading Parquet data...
    LOG(ERROR) << st.message();
    RaiseException(st.message());
  }
  auto dataset = std::make_shared<Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> ParquetCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

int ParquetCursor::write(std::shared_ptr<Dataset> dataset) {
  return 0;
}

// ======== Parquet Driver implementation ========

ParquetDriver::ParquetDriver(const std::string& nodelet_addr)
    : DataDriver(nodelet_addr) {
  setDriverType();
}

ParquetDriver::ParquetDriver(const std::string& nodelet_addr,
    std::unique_ptr<DataSetAccessInfo> access_info)
    : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
}

void ParquetDriver::setDriverType() {
  driver_type = kDriveType[DriverType::PARQUET];
}

std::unique_ptr<Cursor> ParquetDriver::read() {
  auto access_info = dynamic_cast<ParquetAccessInfo*>(this->access_info_.get());
  if (access_info == nullptr) {
    RaiseException("file access info is unavailable");
  }
  VLOG(5) << "access_info_ptr schema column size: "
          << access_info->schema.size();
  if (access_info->Schema().empty()) {
    arrow::Status st;
    arrow::MemoryPool* pool = arrow::default_memory_pool();
    std::shared_ptr<arrow::io::RandomAccessFile> input =
      arrow::io::ReadableFile::Open(
        access_info->file_path_.c_str()).ValueOrDie();

    // Open Parquet file reader
    std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
    st = parquet::arrow::OpenFile(input, pool, &arrow_reader);
    if (!st.ok()) {
      // Handle error instantiating file reader...
      LOG(ERROR) << st.message();
      RaiseException(st.message());
    }
    arrow_reader->set_use_threads(true);
    std::shared_ptr<::arrow::Schema> schema;
    st = arrow_reader->GetSchema(&schema);
    if (!st.ok()) {
      LOG(ERROR) << st.message();
      RaiseException(st.message());
    }
    std::vector<FieldType> fields;
    auto& arrow_fields = schema->fields();
    for (const auto& field : arrow_fields) {
      const auto& name = field->name();
      int type = field->type()->id();
      fields.emplace_back(std::make_tuple(name, type));
    }
    access_info->SetDatasetSchema(std::move(fields));
  }
  return this->initCursor(access_info->file_path_);
}

std::unique_ptr<Cursor> ParquetDriver::read(const std::string& filePath) {
  return this->initCursor(filePath);
}

std::unique_ptr<Cursor> ParquetDriver::GetCursor() {
  return read();
}

std::unique_ptr<Cursor> ParquetDriver::GetCursor(
    const std::vector<int>& col_index) {
  auto access_info = dynamic_cast<ParquetAccessInfo*>(this->access_info_.get());
  if (access_info == nullptr) {
    RaiseException("file access info is unavailable");
  }
  file_path_ = access_info->file_path_;
  return std::make_unique<ParquetCursor>(file_path_,
                                          col_index, shared_from_this());
}

std::unique_ptr<Cursor> ParquetDriver::initCursor(const std::string& file_path) {
  file_path_ = file_path;
  return std::make_unique<ParquetCursor>(file_path, shared_from_this());
}

int ParquetDriver::write(std::shared_ptr<arrow::Table> table,
                     const std::string& file_path) {
  return 0;
}

retcode ParquetDriver::Write(const std::vector<std::string>& fields_name,
                         std::shared_ptr<arrow::Table> table,
                         const std::string& file_path) {
  return retcode::SUCCESS;
}

std::string ParquetDriver::getDataURL() const {
  return file_path_;
}

}  // namespace primihub
