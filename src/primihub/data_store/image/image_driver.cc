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
#include "src/primihub/data_store/image/image_driver.h"

#include <sys/stat.h>
#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>
#include <glog/logging.h>

#include <variant>
#include <fstream>
#include <iostream>
#include <utility>
#include <nlohmann/json.hpp>

#include "src/primihub/data_store/driver.h"
namespace primihub {
// ImageAccessInfo
std::string ImageAccessInfo::toString() {
  std::stringstream ss;
  nlohmann::json js;
  js["image_dir"] = this->image_dir_;
  js["annotations_file"] = this->annotations_file_;
  js["type"] = kDriveType[DriverType::IMAGE];
  ss << js;
  return ss.str();
}
retcode ImageAccessInfo::ParseFromJsonImpl(const nlohmann::json& access_info) {
  try {
    this->image_dir_ = access_info["image_dir"].get<std::string>();
    this->annotations_file_ = access_info["annotations_file"].get<std::string>();
  } catch (std::exception& e) {
    LOG(ERROR) << "parse access info encountes error, " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode ImageAccessInfo::ParseFromYamlConfigImpl(const YAML::Node& meta_info) {
  this->image_dir_ = meta_info["image_dir"].as<std::string>();
  this->annotations_file_ = meta_info["annotations_file"].as<std::string>();
  return retcode::SUCCESS;
}

retcode ImageAccessInfo::ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) {
  auto ret{retcode::SUCCESS};
  try {
    nlohmann::json access_info = nlohmann::json::parse(meta_info.access_info);
    ret = ParseFromJsonImpl(access_info);
  } catch (std::exception& e) {
    LOG(ERROR) << "parse access info failed";
    ret = retcode::FAIL;
  }
  return ret;
}

// image cursor implementation
ImageCursor::ImageCursor(std::shared_ptr<ImageDriver> driver) {
  this->driver_ = driver;
}

ImageCursor::~ImageCursor() { this->close(); }

void ImageCursor::close() {}

std::shared_ptr<Dataset> ImageCursor::readMeta() {
  return read();
}

// read all data from image file
std::shared_ptr<Dataset> ImageCursor::read() {
  auto access_info = this->driver_->dataSetAccessInfo().get();
  auto access_info_ptr = dynamic_cast<ImageAccessInfo*>(access_info);
  std::string annotations_file = access_info_ptr->annotations_file_;
  arrow::io::IOContext io_context = arrow::io::default_io_context();
  arrow::fs::LocalFileSystem local_fs(
      arrow::fs::LocalFileSystemOptions::Defaults());
  auto result_ifstream = local_fs.OpenInputStream(annotations_file);
  if (!result_ifstream.ok()) {
    LOG(ERROR) << "Failed to open file: " << annotations_file << ", "
        << "detail: " << result_ifstream.status();
    return nullptr;
  }
  std::shared_ptr<arrow::io::InputStream> input = result_ifstream.ValueOrDie();

  auto read_options = arrow::csv::ReadOptions::Defaults();
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();

  // Instantiate TableReader from input stream and options
  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_options, parse_options, convert_options);
  if (!maybe_reader.ok()) {
    LOG(ERROR) << "read annotations_file failed, "
        << "detail: " << maybe_reader.status();
    return nullptr;
  }
  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;

  // Read table from CSV file
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    LOG(ERROR) << "read annotations_file failed, "
        << "detail: " << maybe_table.status();
    return nullptr;
  }
  std::shared_ptr<arrow::Table> table = *maybe_table;
  auto dataset = std::make_shared<primihub::Dataset>(table, this->driver_);
  return dataset;
}

std::shared_ptr<Dataset> ImageCursor::read(int64_t offset, int64_t limit) {
  return nullptr;
}

std::shared_ptr<Dataset> ImageCursor::read(const std::shared_ptr<arrow::Schema>& data_schema) {
  return nullptr;
}

int ImageCursor::write(std::shared_ptr<Dataset> dataset) {
  return 0;
}

// ======== image Driver implementation ========

ImageDriver::ImageDriver(const std::string &nodelet_addr): DataDriver(nodelet_addr) {
  setDriverType();
}

ImageDriver::ImageDriver(const std::string &nodelet_addr,
    std::unique_ptr<DataSetAccessInfo> access_info)
    : DataDriver(nodelet_addr, std::move(access_info)) {
  setDriverType();
}

void ImageDriver::setDriverType() {
  driver_type = kDriveType[DriverType::IMAGE];
}

std::unique_ptr<Cursor> ImageDriver::read() {
  return this->initCursor();
}

std::unique_ptr<Cursor> ImageDriver::read(const std::string& access_info) {
  return this->initCursor();
}

std::unique_ptr<Cursor> ImageDriver::initCursor(const std::string& access_info) {
  return initCursor();
}

std::unique_ptr<Cursor> ImageDriver::initCursor() {
  return std::make_unique<ImageCursor>(shared_from_this());
}

std::unique_ptr<Cursor> ImageDriver::GetCursor() {
  return nullptr;
}

std::unique_ptr<Cursor> ImageDriver::GetCursor(const std::vector<int>& col_index) {
  return nullptr;
}

std::string ImageDriver::getDataURL() const {
  return std::string("");
}

}  // namespace primihub
