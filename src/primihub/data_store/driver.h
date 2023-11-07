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


#ifndef SRC_PRIMIHUB_DATA_STORE_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_DRIVER_H_

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <tuple>
#include <utility>
#include <nlohmann/json.hpp>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/common/common.h"

namespace primihub {

class Dataset;
// ====== Data Store Driver ======
using FieldType = std::tuple<std::string, int>;
struct DataSetAccessInfo {
  DataSetAccessInfo() = default;
  virtual ~DataSetAccessInfo() = default;
  virtual std::string toString() = 0;
  virtual retcode fromJsonString(const std::string& access_info);
  virtual retcode fromYamlConfig(const YAML::Node& meta_info);
  virtual retcode FromMetaInfo(const DatasetMetaInfo& meta_info);
  virtual retcode ParseFromJsonImpl(const nlohmann::json& access_info) = 0;
  virtual retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) = 0;
  virtual retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) = 0;
  retcode ParseSchema(const nlohmann::json& json_schema);
  retcode ParseSchema(const YAML::Node& yaml_schema);
  retcode SetDatasetSchema(const std::vector<FieldType>& schema);
  retcode SetDatasetSchema(std::vector<FieldType>&& schema);
  std::shared_ptr<arrow::Schema> ArrowSchema() {return arrow_schema;}
  const std::vector<FieldType>& Schema() const {return schema;}

 protected:
  std::shared_ptr<arrow::DataType> MakeArrowDataType(int type);
  retcode MakeArrowSchema();
  std::string SchemaToJsonString();

 public:
  std::vector<FieldType> schema;
  std::shared_ptr<arrow::Schema> arrow_schema{nullptr};

 private:
  std::shared_mutex schema_mtx;
};

class Cursor {
 public:
  Cursor() = default;
  explicit Cursor(const std::vector<int>& selected_column) {
    for (const auto& col : selected_column) {
      selected_column_index_.push_back(col);
    }
  }
  virtual ~Cursor() = default;
  virtual std::shared_ptr<Dataset> readMeta() = 0;
  virtual std::shared_ptr<Dataset> read() = 0;
  virtual std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) = 0;
  virtual std::shared_ptr<Dataset> read(const std::vector<FieldType>& data_schema) {
    auto arrow_schema = MakeArrowSchema(data_schema);
    return read(arrow_schema);
  }

  virtual std::shared_ptr<Dataset>
      read(const std::shared_ptr<arrow::Schema>& data_schema) = 0;
  virtual int write(std::shared_ptr<Dataset> dataset) = 0;
  virtual void close() = 0;
  std::vector<int>& SelectedColumnIndex() {return selected_column_index_;}
  void SetDatasetId(const std::string& dataset_id) {dataset_id_ = dataset_id;};
  std::string DatasetId() {return dataset_id_;}
  std::string TraceId() {return trace_id_;}
  void SetTraceId(const std::string& trace_id) {trace_id_ = trace_id;};

 protected:
  std::shared_ptr<arrow::Schema>
      MakeArrowSchema(const std::vector<FieldType>& data_schema);

 public:
  std::vector<int> selected_column_index_;
  std::string dataset_id_;
  std::string trace_id_;
};

class DataDriver {
 public:
  explicit DataDriver(const std::string& nodelet_addr) {
    nodelet_address = nodelet_addr;
  }
  DataDriver(const std::string& nodelet_addr,
             std::unique_ptr<DataSetAccessInfo> access_info) {
    nodelet_address = nodelet_addr;
    access_info_ = std::move(access_info);
  }
  virtual ~DataDriver() = default;
  virtual std::string getDataURL() const = 0;
  [[deprecated("use read instead")]]
  virtual std::unique_ptr<Cursor> read(const std::string &dataURL) = 0;
  /**
   * data access info read from internal access_info_
  */
  virtual std::unique_ptr<Cursor> read() = 0;
  virtual std::unique_ptr<Cursor> GetCursor() = 0;
  virtual std::unique_ptr<Cursor> GetCursor(
      const std::vector<int>& col_index) = 0;

  [[deprecated("use GetCursor instead")]]
  virtual std::unique_ptr<Cursor> initCursor(const std::string &dataURL) = 0;
  // std::unique_ptr<Cursor> getCursor();
  std::string getDriverType() const;
  std::string getNodeletAddress() const;

  std::unique_ptr<DataSetAccessInfo>& dataSetAccessInfo() {
    return access_info_;
  }

 protected:
  void setCursor(std::unique_ptr<Cursor> cursor) {
      this->cursor = std::move(cursor);
  }
  std::unique_ptr<Cursor> cursor{nullptr};
  std::string nodelet_address;
  std::string driver_type;
  std::unique_ptr<DataSetAccessInfo> access_info_{nullptr};
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_DATA_STORE_DRIVER_H_
