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

#include <sys/stat.h>
#include <glog/logging.h>

#include <fstream>
#include <variant>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/thread_local_data.h"

namespace primihub {
namespace csv {
std::shared_ptr<arrow::Table> ReadCSVFile(const std::string& file_path,
                                        const arrow::csv::ReadOptions& read_opt,
                                        const arrow::csv::ParseOptions& parse_opt,
                                        const arrow::csv::ConvertOptions& convert_opt) {
  arrow::io::IOContext io_context = arrow::io::default_io_context();
  arrow::fs::LocalFileSystem local_fs(arrow::fs::LocalFileSystemOptions::Defaults());
  auto result_ifstream = local_fs.OpenInputStream(file_path);
  if (!result_ifstream.ok()) {
    std::stringstream ss;
    ss << "Failed to open file: " << file_path << " "
        << "detail: " << result_ifstream.status();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return nullptr;
  }
  std::shared_ptr<arrow::io::InputStream> input = result_ifstream.ValueOrDie();
  // Instantiate TableReader from input stream and options
  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_opt, parse_opt, convert_opt);
  if (!maybe_reader.ok()) {
    std::stringstream ss;
    ss << "read data failed, " << "detail: " << maybe_reader.status();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return nullptr;
  }
  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;
  // Read table from CSV file
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    // (for example a CSV syntax error or failed type conversion)
    std::stringstream ss;
    ss << "read data failed, " << "detail: " << maybe_table.status();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return nullptr;
  }

  return *maybe_table;
}
}  // namespace csv

// CSVAccessInfo
std::string CSVAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["type"] = "csv";
  js["data_path"] = this->file_path_;
  ss << js;
  return ss.str();
}

retcode CSVAccessInfo::fromJsonString(const std::string& access_info) {
  retcode ret{retcode::SUCCESS};
  try {
    nlohmann::json js_access_info = nlohmann::json::parse(access_info);
    if (js_access_info.contains("schema")) {
      auto schema_json = nlohmann::json::parse(js_access_info["schema"].get<std::string>());
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

retcode CSVAccessInfo::ParseFromJsonImpl(const nlohmann::json& meta_info) {
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
      std::string err_msg = ss.str();
      SetThreadLocalErrorMsg(err_msg);
      LOG(ERROR) << err_msg;
      return retcode::FAIL;
    }
  }
  return retcode::SUCCESS;
}

retcode CSVAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  this->file_path_ = meta_info["source"].as<std::string>();
  return retcode::SUCCESS;
}

retcode CSVAccessInfo::ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) {
  try {
    nlohmann::json js_access_info = nlohmann::json::parse(meta_info.access_info);
    this->file_path_ = js_access_info["data_path"].get<std::string>();
  } catch (std::exception& e) {
    this->file_path_ = meta_info.access_info;
    // check validattion of the path
    std::ifstream csv_data(file_path_, std::ios::in);
    if (!csv_data.is_open()) {
      std::stringstream ss;
      ss << "file_path: " << file_path_ << " is not exist";
      std::string err_msg = ss.str();
      SetThreadLocalErrorMsg(err_msg);
      LOG(ERROR) << err_msg;
      return retcode::FAIL;
    }
    return retcode::SUCCESS;
  }
  return retcode::SUCCESS;
}

// csv cursor implementation
CSVCursor::CSVCursor(std::string filePath, std::shared_ptr<CSVDriver> driver) {
  this->filePath = filePath;
  this->driver_ = driver;
}

CSVCursor::CSVCursor(std::string filePath,
                    const std::vector<int>& colnum_index,
                    std::shared_ptr<CSVDriver> driver) : Cursor(colnum_index) {
  this->filePath = filePath;
  this->driver_ = driver;
}

CSVCursor::~CSVCursor() { this->close(); }

void CSVCursor::close() {
  // TODO
}

retcode CSVCursor::ColumnIndexToColumnName(const std::string& file_path,
    const std::vector<int>& column_index,
    const char delimiter,
    std::vector<std::string>* column_name) {
  auto& arrow_schema = this->driver_->dataSetAccessInfo()->arrow_schema;
  int num_fields = arrow_schema->num_fields();
  if (arrow_schema != nullptr) {
    for (auto index : column_index) {
      if (index < num_fields) {
        auto field = arrow_schema->field(index);
        column_name->push_back(field->name());
      } else {
        std::stringstream ss;
        ss << "index [" << index << "] is invalid";
        std::string err_msg = ss.str();
        SetThreadLocalErrorMsg(err_msg);
        LOG(ERROR) << err_msg;
        return retcode::FAIL;
      }
    }
  } else {
    LOG(ERROR) << "dataset schema is empty";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode CSVCursor::BuildConvertOptions(arrow::csv::ConvertOptions* convert_options_ptr) {
  if (this->driver_->dataSetAccessInfo()->schema.empty()) {
    LOG(ERROR) << "no schema is set for dataset";
    return retcode::FAIL;
  }
  auto& include_columns = convert_options_ptr->include_columns;
  auto& column_types = convert_options_ptr->column_types;
  auto& selected_index = this->SelectedColumnIndex();
  auto& arrow_schema = this->driver_->dataSetAccessInfo()->arrow_schema;
  int number_fields = arrow_schema->num_fields();
  if (!selected_index.empty()) {
    for (const auto index : selected_index) {
      if (index < number_fields) {
        auto field_ptr = arrow_schema->field(index);
        const auto& name = field_ptr->name();
        const auto& field_type = field_ptr->type();
        include_columns.push_back(name);
        column_types[name] = field_type;
        VLOG(7) << "name: [" << name << "] type: " << field_type->id();
      } else {
        std::stringstream ss;
        ss << "index is out of range, index: " << index
            << " total columns: " << number_fields;
        std::string err_msg = ss.str();
        SetThreadLocalErrorMsg(err_msg);
        LOG(ERROR) << err_msg;
        return retcode::FAIL;
      }
    }
  } else {
    auto all_fields = arrow_schema->fields();
    for (const auto& field : all_fields) {
      const auto& col_name = field->name();
      const auto& field_type = field->type();
      include_columns.push_back(col_name);
      column_types[col_name] = field_type;
      VLOG(7) << "name: " << col_name << " type: " << field_type->id();
    }
  }
  return retcode::SUCCESS;
}

std::shared_ptr<primihub::Dataset> CSVCursor::readMeta() {
  return read();
}

// read all data from csv file
std::shared_ptr<Dataset> CSVCursor::read() {
  auto read_options = arrow::csv::ReadOptions::Defaults();
  read_options.skip_rows = 1;  // skip title row
  auto& arrow_schema = this->driver_->dataSetAccessInfo()->arrow_schema;
  auto field_names = arrow_schema->field_names();
  read_options.column_names = field_names;
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();
  auto ret = BuildConvertOptions(&convert_options);
  if (ret != retcode::SUCCESS) {
    return nullptr;
  }
  auto arrow_table = csv::ReadCSVFile(filePath, read_options, parse_options, convert_options);
  if (arrow_table == nullptr) {
    return nullptr;
  }
  auto dataset = std::make_shared<Dataset>(arrow_table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> CSVCursor::read(int64_t offset, int64_t limit) {
  // TODO
  return nullptr;
}

int CSVCursor::write(std::shared_ptr<Dataset> dataset) {
  // check existence of file directory
  auto ret = ValidateDir(this->filePath);
  if (ret != 0) {
    LOG(ERROR) << "something wrong with operatating file path: " << this->filePath;
    return -1;
  }
  // write Dataset to csv file
  auto result = arrow::io::FileOutputStream::Open(this->filePath);
  if (!result.ok()) {
    std::stringstream ss;
    ss << "Open file " << filePath << " failed, " << "detail: " << result.status();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return -1;
  }

  auto stream = result.ValueOrDie();
  auto options = arrow::csv::WriteOptions::Defaults();
  auto csv_table = std::get<std::shared_ptr<arrow::Table>>(dataset->data);
  auto mem_pool = arrow::default_memory_pool();
  arrow::Status status = arrow::csv::WriteCSV(
      *(csv_table), options, mem_pool,
      reinterpret_cast<arrow::io::OutputStream *>(stream.get()));
  if (!status.ok()) {
    std::stringstream ss;
    ss << "Write content to csv file failed. " << status;
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return -2;
  }
  return 0;
}

// ======== CSV Driver implementation ========

CSVDriver::CSVDriver(const std::string &nodelet_addr)
    : DataDriver(nodelet_addr) {
  setDriverType();
}

CSVDriver::CSVDriver(const std::string &nodelet_addr,
    std::unique_ptr<DataSetAccessInfo> access_info)
    : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
}

void CSVDriver::setDriverType() {
  driver_type = "CSV";
}

retcode CSVDriver::GetColumnNames(const char delimiter,
                                  std::vector<std::string>* column_names) {
  column_names->clear();
  auto& colum_names = *column_names;
  auto csv_access_info = dynamic_cast<CSVAccessInfo*>(this->access_info_.get());
  if (csv_access_info == nullptr) {
    LOG(ERROR) << "file access info is unavailable";
    return retcode::FAIL;
  }
  std::ifstream csv_data(csv_access_info->file_path_, std::ios::in);
  if (!csv_data.is_open()) {
    std::stringstream ss;
    ss << "open csv file: " << csv_access_info->file_path_ << " failed";
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  std::string tile_row;
  std::getline(csv_data, tile_row);
  str_split(tile_row, &colum_names, delimiter);
  if (VLOG_IS_ON(5)) {
    std::string title_name;
    for (const auto& name : colum_names) {
      title_name.append("[").append(name).append("] ");
    }
    VLOG(5) << title_name;
  }
  if (!colum_names.empty()) {
    auto& last_item = colum_names[colum_names.size() - 1];
    auto it = std::find(last_item.begin(), last_item.end(), '\n');
    if (it != last_item.end()) {
      last_item.erase(it);
    }
    it = std::find(last_item.begin(), last_item.end(), '\r');
    if (it != last_item.end()) {
      last_item.erase(it);
    }
  }
  if (VLOG_IS_ON(5)) {
    for (const auto& name : colum_names) {
      VLOG(5) << "column name: [" << name << "]";
    }
  }
  return retcode::SUCCESS;
}

std::unique_ptr<Cursor> CSVDriver::read() {
  auto csv_access_info = dynamic_cast<CSVAccessInfo*>(this->access_info_.get());
  if (csv_access_info == nullptr) {
    LOG(ERROR) << "file access info is unavailable";
    return nullptr;
  }
  VLOG(5) << "access_info_ptr schema column size: " << csv_access_info->schema.size();
  if (csv_access_info->Schema().empty()) {
    auto read_options = arrow::csv::ReadOptions::Defaults();
    auto parse_options = arrow::csv::ParseOptions::Defaults();
    auto convert_options = arrow::csv::ConvertOptions::Defaults();
    auto ret = GetColumnNames(parse_options.delimiter, &read_options.column_names);
    if (ret != retcode::SUCCESS) {
      return nullptr;
    }

    read_options.skip_rows = 1;  // skip title row

    auto arrow_data = csv::ReadCSVFile(csv_access_info->file_path_, read_options,
                                      parse_options, convert_options);
    if (arrow_data == nullptr) {
      return nullptr;
    }

    std::vector<FieldType> fileds;
    auto arrow_fileds = arrow_data->schema()->fields();
    for (const auto& field : arrow_fileds) {
      const auto& name = field->name();
      int type = field->type()->id();
      fileds.emplace_back(std::make_tuple(name, type));
    }
    csv_access_info->SetDatasetSchema(std::move(fileds));
  }
  return this->initCursor(csv_access_info->file_path_);
}

std::unique_ptr<Cursor> CSVDriver::read(const std::string &filePath) {
  return this->initCursor(filePath);
}

std::unique_ptr<Cursor> CSVDriver::GetCursor() {
  return read();
}

std::unique_ptr<Cursor> CSVDriver::GetCursor(const std::vector<int>& col_index) {
  auto csv_access_info = dynamic_cast<CSVAccessInfo*>(this->access_info_.get());
  if (csv_access_info == nullptr) {
    LOG(ERROR) << "file access info is unavailable";
    return nullptr;
  }
  filePath_ = csv_access_info->file_path_;
  return std::make_unique<CSVCursor>(filePath_, col_index, shared_from_this());
}

std::unique_ptr<Cursor> CSVDriver::initCursor(const std::string &filePath) {
  filePath_ = filePath;
  return std::make_unique<CSVCursor>(filePath, shared_from_this());
}

// FIXME to be deleted write file in driver directly.
int CSVDriver::write(std::shared_ptr<arrow::Table> table,
                     const std::string& filePath) {
  auto ret = ValidateDir(filePath);
  if (ret != 0) {
    LOG(ERROR) << "something wrong with operatating file path: " << filePath;
    return -1;
  }
  auto result = arrow::io::FileOutputStream::Open(filePath);
  if (!result.ok()) {
    std::stringstream ss;
    ss << "Open file " << filePath << " failed. " << result.status();
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return -1;
  }

  auto stream = result.ValueOrDie();
  auto options = arrow::csv::WriteOptions::Defaults();
  auto csv_table = table.get();
  auto mem_pool = arrow::default_memory_pool();
  arrow::Status status = arrow::csv::WriteCSV(
      *(csv_table), options, mem_pool,
      reinterpret_cast<arrow::io::OutputStream *>(stream.get()));
  if (!status.ok()) {
    std::stringstream ss;
    ss << "Write content to csv file failed. " << status;
    std::string err_msg = ss.str();
    SetThreadLocalErrorMsg(err_msg);
    LOG(ERROR) << err_msg;
    return -2;
  }
  return 0;
}

std::string CSVDriver::getDataURL() const {
  return filePath_;
};

} // namespace primihub
