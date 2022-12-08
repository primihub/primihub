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
#include <sys/stat.h>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/driver.h"

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>

namespace primihub {

// csv cursor implementation
CSVCursor::CSVCursor(std::string filePath, std::shared_ptr<CSVDriver> driver) {
  this->filePath = filePath;
  this->driver_ = driver;
}

CSVCursor::~CSVCursor() { this->close(); }

void CSVCursor::close() {
  // TODO
}

// read all data from csv file
std::shared_ptr<primihub::Dataset> CSVCursor::read() {
  arrow::io::IOContext io_context = arrow::io::default_io_context();
  arrow::fs::LocalFileSystem local_fs(
      arrow::fs::LocalFileSystemOptions::Defaults());
  auto result_ifstream = local_fs.OpenInputStream(filePath);
  if (!result_ifstream.ok()) {
    std::cout << "Failed to open file: " << filePath << std::endl;
    return nullptr; // TODO throw exception
  }
  std::shared_ptr<arrow::io::InputStream> input = result_ifstream.ValueOrDie();

  auto read_options = arrow::csv::ReadOptions::Defaults();
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();

  // Instantiate TableReader from input stream and options
  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_options, parse_options, convert_options);
  if (!maybe_reader.ok()) {
    // TODO Handle TableReader instantiation error...
  }
  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;

  // Read table from CSV file
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    // TODO Handle CSV read error
    // (for example a CSV syntax error or failed type conversion)
  }
  std::shared_ptr<arrow::Table> table = *maybe_table;
  auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<primihub::Dataset> CSVCursor::read(int64_t offset,
                                                   int64_t limit) {
  // TODO
  return nullptr;
}

int CSVCursor::write(std::shared_ptr<primihub::Dataset> dataset) {
  // write Dataset to csv file
  auto result = arrow::io::FileOutputStream::Open(this->filePath);
  if (!result.ok()) {
    LOG(ERROR) << "Open file " << filePath << " failed.";
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
    LOG(ERROR) << "Write content to csv file failed.";
    return -2;
  }
  return 0;
}

// ======== CSV Driver implementation ========

CSVDriver::CSVDriver(const std::string &nodelet_addr)
    : DataDriver(nodelet_addr) {
  driver_type = "CSV";
}

std::shared_ptr<Cursor> &CSVDriver::read(const std::string &filePath) {
   return this->initCursor(filePath);
}

std::shared_ptr<Cursor> &CSVDriver::initCursor(const std::string &filePath) {
  filePath_ = filePath;
  this->cursor = std::make_shared<CSVCursor>(filePath, shared_from_this());
  return getCursor();
}

// FIXME to be deleted write file in driver directly.
int CSVDriver::write(std::shared_ptr<arrow::Table> table,
                     std::string &filePath) {
  filePath_ = filePath;
  auto result = arrow::io::FileOutputStream::Open(filePath);
  if (!result.ok()) {
    LOG(ERROR) << "Open file " << filePath << " failed.";
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
    LOG(ERROR) << "Write content to csv file failed.";
    return -2;
  }

  return 0;
}

std::string CSVDriver::getDataURL() const { return filePath_; };

} // namespace primihub
