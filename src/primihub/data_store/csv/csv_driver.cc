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

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/thread_local_data.h"
#include "src/primihub/common/value_check_util.h"

namespace primihub {
namespace csv {
using ReadOptions = arrow::csv::ReadOptions;
using ParseOptions = arrow::csv::ParseOptions;
using ConvertOptions = arrow::csv::ConvertOptions;
static const uint8_t kBOM[] = {0xEF, 0xBB, 0xBF};
retcode SkipUTF8BOM(const std::string& origin_data, std::string* new_data) {
  int64_t i;
  new_data->clear();
  if (origin_data.empty()) {
    return retcode::SUCCESS;
  }
  auto data = reinterpret_cast<const uint8_t*>(origin_data.data());
  size_t size = origin_data.size();
  for (i = 0; i < static_cast<int64_t>(sizeof(kBOM)); ++i) {
    if (size == 0) {
      if (i == 0) {
        // Empty string
        return retcode::SUCCESS;
      } else {
        std::stringstream ss;
        ss << "UTF8 string too short (truncated byte order mark?)";
        RaiseException(ss.str());
      }
    }
    if (*(data+i) != kBOM[i]) {
      // BOM not found
      *new_data = origin_data;
      return retcode::SUCCESS;
    }
    --size;
  }
  // BOM found
  *new_data = std::string(origin_data.data() + i, size);
  return retcode::SUCCESS;
}

// -------------------------write operation------------------------------
/**
   *  write csv title to file
*/
retcode WriteHeader(const std::vector<std::string>& col_name,
                    const std::string& file_path) {
  auto ret = ValidateDir(file_path);
  if (ret != 0) {
    LOG(ERROR) << "something wrong with operating file path: " << file_path;
    return retcode::FAIL;
  }
  if (col_name.empty()) {
    LOG(WARNING) << "no field name need to write";
    return retcode::SUCCESS;
  }
  std::string header_str;
  // write BOM
  header_str.append(std::begin(primihub::csv::kBOM),
                    std::end(primihub::csv::kBOM));
  for (const auto& item : col_name) {
    header_str.append(item).append(",");
  }
  header_str[header_str.size() - 1] = '\n';
  std::ofstream csv_data(file_path, std::ios::out);
  csv_data << header_str;
  return retcode::SUCCESS;
}
/**
   * write table data to file
   * table: data need to write to file
   * file_path: the location of data file
   * include_header:
   *  true: write csv title line using table schema (default)
   *  false: do not write csv title line using table schema
   * append_flag:
   *  true: append table data to existing file_path
   *  false: truncated to 0 bytes, deleting any existing data (default)
*/
retcode WriteContent(std::shared_ptr<arrow::Table> table,
                     const std::string& file_path,
                     bool include_header = true,
                     bool append_flag = false) {
//
  auto ret = ValidateDir(file_path);
  if (ret != 0) {
    LOG(ERROR) << "something wrong with operating file path: " << file_path;
    return retcode::FAIL;
  }
  auto result = arrow::io::FileOutputStream::Open(file_path, append_flag);
  if (!result.ok()) {
    std::stringstream ss;
    ss << "Open file " << file_path << " failed. " << result.status();
    RaiseException(ss.str());
  }

  auto stream = result.ValueOrDie();
  auto options = arrow::csv::WriteOptions::Defaults();
  options.include_header = include_header;
  auto csv_table = table.get();
  auto mem_pool = arrow::default_memory_pool();
  arrow::Status status = arrow::csv::WriteCSV(
      *(csv_table), options, mem_pool,
      reinterpret_cast<arrow::io::OutputStream *>(stream.get()));
  if (!status.ok()) {
    std::stringstream ss;
    ss << "Write content to csv file failed. " << status;
    RaiseException(ss.str());
  }
  return retcode::SUCCESS;
}

retcode WriteImpl(const std::vector<std::string>& fields_name,
                  std::shared_ptr<arrow::Table> table,
                  const std::string& file_path) {
  auto rtcode = WriteHeader(fields_name, file_path);
  if (rtcode != retcode::SUCCESS) {
    LOG(ERROR) << "WriteHeader to " << file_path << " failed";
    return retcode::FAIL;
  }
  bool include_header{false};
  bool append_data{true};
  rtcode = WriteContent(table, file_path, include_header, append_data);
  if (rtcode != retcode::SUCCESS) {
    LOG(ERROR) << "write data to " << file_path << " failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode WriteImpl(std::shared_ptr<arrow::Table> table,
                  const std::string& file_path) {
  auto colum_names = table->ColumnNames();
  return WriteImpl(colum_names, table, file_path);
}

// ---------------------------Read operation----------------------------------
std::shared_ptr<arrow::Table> Read(
    std::shared_ptr<arrow::io::InputStream> input,
    const arrow::csv::ReadOptions& read_opt,
    const arrow::csv::ParseOptions& parse_opt,
    const arrow::csv::ConvertOptions& convert_opt) {
  arrow::io::IOContext io_context = arrow::io::default_io_context();
  // Instantiate TableReader from input stream and options
  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_opt, parse_opt, convert_opt);
  if (!maybe_reader.ok()) {
    std::stringstream ss;
    ss << "read data failed, " << "detail: " << maybe_reader.status();
    RaiseException(ss.str());
  }
  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;
  // Read table from CSV file
  SCopedTimer timer;
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    // (for example a CSV syntax error or failed type conversion)
    std::stringstream ss;
    ss << "read data failed, " << "detail: " << maybe_table.status();
    RaiseException(ss.str());
  }
  auto time_cost = timer.timeElapse();
  VLOG(5) << "read csv data time cost(ms): " << time_cost;
  return *maybe_table;
}

std::shared_ptr<arrow::Table> ReadCSVData(std::string_view data_buff,
                                          const ReadOptions& read_opt,
                                          const ParseOptions& parse_opt,
                                          const ConvertOptions& convert_opt) {
  auto input = std::make_shared<arrow::io::BufferReader>(
      reinterpret_cast<uint8_t*>(const_cast<char*>(data_buff.data())),
      data_buff.size());
  return Read(input, read_opt, parse_opt, convert_opt);
}

std::shared_ptr<arrow::Table> ReadCSVFile(const std::string& file_path,
                                          const ReadOptions& read_opt,
                                          const ParseOptions& parse_opt,
                                          const ConvertOptions& convert_opt) {
  auto local_fs_options = arrow::fs::LocalFileSystemOptions::Defaults();
  local_fs_options.use_mmap = true;
  arrow::fs::LocalFileSystem local_fs(local_fs_options);
  auto result_ifstream = local_fs.OpenInputStream(file_path);
  if (!result_ifstream.ok()) {
    std::stringstream ss;
    ss << "Failed to open file: " << file_path << " "
        << "detail: " << result_ifstream.status();
    RaiseException(ss.str());
  }
  int64_t file_size = FileSize(file_path);
  ReadOptions read_opt_ = read_opt;
  read_opt_.block_size = file_size;
  std::shared_ptr<arrow::io::InputStream> input = result_ifstream.ValueOrDie();
  return Read(input, read_opt_, parse_opt, convert_opt);
}

std::string ReadRawData(const std::string& file_path, int64_t line_number) {
  // read data first 100 lines
  std::ifstream csv_data(file_path, std::ios::in);
  if (!csv_data.is_open()) {
    std::stringstream ss;
    ss << "open csv file: " << file_path << " failed";
    RaiseException(ss.str());
  }
  std::string content_buf;
  std::string tmp_buf;
  for (int i = 0; i < line_number; i++) {
    if (csv_data.eof()) {
      break;
    }
    std::getline(csv_data, tmp_buf);
    if (tmp_buf.empty()) {
      continue;
    }
    content_buf.append(tmp_buf).append("\n");
  }
  return content_buf;
}


}  // namespace csv

// CSVAccessInfo
std::string CSVAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["type"] = kDriveType[DriverType::CSV];
  js["data_path"] = this->file_path_;
  js["schema"] = SchemaToJsonString();
  ss << js;
  return ss.str();
}

retcode CSVAccessInfo::fromJsonString(const std::string& access_info) {
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
      RaiseException(ss.str());
    }
  }
  return retcode::SUCCESS;
}

retcode CSVAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  this->file_path_ = meta_info["source"].as<std::string>();
  return retcode::SUCCESS;
}

retcode CSVAccessInfo::ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) {
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

// csv cursor implementation
CSVCursor::CSVCursor(const std::string& file_path,
                     std::shared_ptr<CSVDriver> driver) {
  this->file_path_ = file_path;
  this->driver_ = driver;
}

CSVCursor::CSVCursor(const std::string& file_path,
                     const std::vector<int>& colnum_index,
                     std::shared_ptr<CSVDriver> driver)
                     : Cursor(colnum_index) {
  this->file_path_ = file_path;
  this->driver_ = driver;
}

CSVCursor::~CSVCursor() { this->close(); }

void CSVCursor::close() {
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
        RaiseException(ss.str());
      }
    }
  } else {
    RaiseException("dataset schema is empty");
  }
  return retcode::SUCCESS;
}

retcode CSVCursor::BuildConvertOptions(
    arrow::csv::ConvertOptions* convert_options_ptr) {
  if (this->driver_->dataSetAccessInfo()->schema.empty()) {
    RaiseException("no schema is set for dataset");
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
        RaiseException(ss.str());
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

retcode CSVCursor::BuildConvertOptions(
    const std::shared_ptr<arrow::Schema>& data_schema,
    arrow::csv::ConvertOptions* convert_option) {
  if (data_schema == nullptr) {
    LOG(ERROR) << "data schema is invalid";
    return retcode::FAIL;
  }
  auto& include_columns = convert_option->include_columns;
  auto& column_types = convert_option->column_types;
  int number_fields = data_schema->num_fields();
  for (int i = 0; i < number_fields; i++) {
    auto field_ptr = data_schema->field(i);
    const auto& name = field_ptr->name();
    const auto& field_type = field_ptr->type();
    include_columns.push_back(name);
    column_types[name] = field_type;
  }
  return retcode::SUCCESS;
}

retcode CSVCursor::MakeCsvOptions(CsvOptions* options) {
  return MakeCsvOptions(nullptr, options);
}

retcode CSVCursor::MakeCsvOptions(
    const std::shared_ptr<arrow::Schema>& data_schema,
    CsvOptions* options) {
  auto& read_options = options->read_options;
  read_options.skip_rows = 1;  // skip title row
  auto& arrow_schema = this->driver_->dataSetAccessInfo()->arrow_schema;
  auto field_names = arrow_schema->field_names();
  read_options.column_names = field_names;
  auto& convert_options = options->convert_options;
  auto ret{retcode::SUCCESS};
  if (data_schema == nullptr) {
    ret = BuildConvertOptions(&convert_options);
  } else {
    ret = BuildConvertOptions(data_schema, &convert_options);
  }
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "BuildConvertOptions failed";
  }
  return ret;
}

std::shared_ptr<Dataset> CSVCursor::readMeta() {
  CsvOptions csv_options;
  auto ret = MakeCsvOptions(&csv_options);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "make csv file options failed";
    return nullptr;
  }
  std::string content_buf = csv::ReadRawData(file_path_, 100);
  std::string_view content_sv(content_buf.data(), content_buf.size());
  return ReadImpl(content_sv, csv_options.read_options,
                  csv_options.parse_options, csv_options.convert_options);
}

std::shared_ptr<Dataset> CSVCursor::read(
    const std::shared_ptr<arrow::Schema>& data_schema) {
  CsvOptions csv_options;
  auto ret = MakeCsvOptions(data_schema, &csv_options);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "make csv file options failed";
    return nullptr;
  }
  return ReadImpl(this->file_path_, csv_options.read_options,
                  csv_options.parse_options, csv_options.convert_options);
}

// read all data from csv file
std::shared_ptr<Dataset> CSVCursor::read() {
  CsvOptions csv_options;
  auto ret = MakeCsvOptions(&csv_options);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "make csv file options failed";
    return nullptr;
  }
  return ReadImpl(this->file_path_, csv_options.read_options,
                  csv_options.parse_options, csv_options.convert_options);
}

std::shared_ptr<Dataset> CSVCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

std::shared_ptr<Dataset> CSVCursor::ReadImpl(const std::string& file_path,
    const ReadOptions& read_options,
    const ParseOptions& parse_options,
    const ConvertOptions& convert_options) {
  auto arrow_table = csv::ReadCSVFile(file_path, read_options,
                                      parse_options, convert_options);
  if (arrow_table == nullptr) {
    return nullptr;
  }
  auto dataset = std::make_shared<Dataset>(arrow_table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> CSVCursor::ReadImpl(std::string_view input_data,
    const ReadOptions& read_options,
    const ParseOptions& parse_options,
    const ConvertOptions& convert_options) {
  auto arrow_table = csv::ReadCSVData(input_data, read_options,
                                      parse_options, convert_options);
  if (arrow_table == nullptr) {
    return nullptr;
  }
  auto dataset = std::make_shared<Dataset>(arrow_table, this->driver_);
  return dataset;
}

int CSVCursor::write(std::shared_ptr<Dataset> dataset) {
  auto csv_table = std::get<std::shared_ptr<arrow::Table>>(dataset->data);
  auto ret = primihub::csv::WriteImpl(csv_table, this->file_path_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "write data to " << this->file_path_ << " failed";
    return -1;
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
  driver_type = kDriveType[DriverType::CSV];
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
  std::string tile_row = csv::ReadRawData(csv_access_info->file_path_, 1);
  if (tile_row.empty()) {
    LOG(ERROR) << "file is empty";
    return retcode::FAIL;
  }
  str_split(tile_row, &colum_names, delimiter);
  if (VLOG_IS_ON(5)) {
    std::string title_name;
    for (const auto& name : colum_names) {
      title_name.append("[").append(name).append("] ");
    }
    VLOG(5) << "origin title content: " << title_name;
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
    // remove bom mark
    auto& first_item = colum_names[0];
    std::string new_first_item;
    auto ret = primihub::csv::SkipUTF8BOM(first_item, &new_first_item);
    if (ret == retcode::SUCCESS) {
      first_item = std::move(new_first_item);
    } else {
      LOG(ERROR) << "check bom and remove failed for item: " << first_item;
      return retcode::FAIL;
    }
    // remove quotation mark if it exists
    for (auto& col_num : colum_names) {
      // Trim leading quotation mark
      col_num.erase(
          col_num.begin(),
          find_if(col_num.begin(),
                  col_num.end(),
                  [](int ch) { return !(ch == 0x22); }));

      // Trim trailing quotation mark
      col_num.erase(
          find_if(col_num.rbegin(),
                  col_num.rend(),
                  [](int ch) {return !(ch == 0x22); }).base(),
          col_num.end());
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
    RaiseException("file access info is unavailable");
  }
  VLOG(5) << "access_info_ptr schema column size: "
          << csv_access_info->schema.size();
  if (csv_access_info->Schema().empty()) {
    auto read_options = arrow::csv::ReadOptions::Defaults();
    read_options.skip_rows = 1;  // skip title row
    auto parse_options = arrow::csv::ParseOptions::Defaults();
    auto convert_options = arrow::csv::ConvertOptions::Defaults();
    auto ret = GetColumnNames(parse_options.delimiter,
                              &read_options.column_names);
    if (ret != retcode::SUCCESS) {
      return nullptr;
    }
    std::string content_buf =
        csv::ReadRawData(csv_access_info->file_path_, 100);
    std::string_view content_sv(content_buf.data(), content_buf.size());
    auto arrow_data = csv::ReadCSVData(content_sv,
                                       read_options, parse_options,
                                       convert_options);
    if (arrow_data == nullptr) {
      return nullptr;
    }

    std::vector<FieldType> fields;
    auto arrow_fields = arrow_data->schema()->fields();
    for (const auto& field : arrow_fields) {
      const auto& name = field->name();
      int type = field->type()->id();
      fields.emplace_back(std::make_tuple(name, type));
    }
    csv_access_info->SetDatasetSchema(std::move(fields));
  }
  return this->initCursor(csv_access_info->file_path_);
}

std::unique_ptr<Cursor> CSVDriver::read(const std::string &filePath) {
  return this->initCursor(filePath);
}

std::unique_ptr<Cursor> CSVDriver::GetCursor() {
  return read();
}

std::unique_ptr<Cursor> CSVDriver::GetCursor(
    const std::vector<int>& col_index) {
  auto csv_access_info = dynamic_cast<CSVAccessInfo*>(this->access_info_.get());
  if (csv_access_info == nullptr) {
    RaiseException("file access info is unavailable");
  }
  filePath_ = csv_access_info->file_path_;
  return std::make_unique<CSVCursor>(filePath_, col_index, shared_from_this());
}

std::unique_ptr<Cursor> CSVDriver::initCursor(const std::string &filePath) {
  filePath_ = filePath;
  return std::make_unique<CSVCursor>(filePath, shared_from_this());
}

int CSVDriver::write(std::shared_ptr<arrow::Table> table,
                     const std::string& file_path) {
  auto ret = primihub::csv::WriteImpl(table, file_path);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "write data to file: " << file_path << " failed";
    return -1;
  }
  return 0;
}

retcode CSVDriver::Write(const std::vector<std::string>& fields_name,
                         std::shared_ptr<arrow::Table> table,
                         const std::string& file_path) {
  return primihub::csv::WriteImpl(fields_name, table, file_path);
}

std::string CSVDriver::getDataURL() const {
  return filePath_;
}

}  // namespace primihub
