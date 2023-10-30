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
#ifndef SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_

#include <arrow/api.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <tuple>
#include <utility>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/util/arrow_wrapper_util.h"

namespace primihub::service {
class DatasetMeta;
using DatasetWithParamTag = std::pair<std::string, std::string>;

using DatasetMetaWithParamTag =
    std::pair<std::shared_ptr<DatasetMeta>, std::string>;

using FoundMetaHandler =
    std::function<retcode(std::shared_ptr<DatasetMeta> &)>;

using ReadDatasetHandler =
    std::function<void(std::shared_ptr<primihub::Dataset> &)>;

using FoundMetaListHandler =
    std::function<void(std::vector<DatasetMetaWithParamTag> &)>;

using FieldType = std::tuple<std::string, int>;

using SchemaConstructorParamType =
    std::variant<std::string, nlohmann::json, std::shared_ptr<arrow::Schema>>;

using DatasetId = std::string;

enum class DatasetVisbility {
  PUBLIC = 0,
  PRIVATE = 1,
};

enum class DatasetType {
  TABLE = 0,
  ARRAY = 1,
  TENSOR = 2,
};

class DatasetSchema {
 public:
  virtual ~DatasetSchema() {}
  virtual std::string toJSON() = 0;
  virtual void fromJSON(const std::string& json) = 0;
  virtual void fromJSON(const nlohmann::json &json) = 0;
  virtual void FromFields(std::vector<FieldType>& fields) = 0;
};

// Two-Dimensional table Schema
class TableSchema : public DatasetSchema {
 public:
  TableSchema() = default;
  explicit TableSchema(std::shared_ptr<arrow::Schema> schema)
      : schema(std::move(schema)) {}

  explicit TableSchema(std::string &json) {
    try {
      nlohmann::json j = nlohmann::json::parse(json);
      fromJSON(j);
    } catch (std::exception& e) {
      this->schema = arrow::schema({});
      LOG(ERROR) << "load dataset schema failed: " << e.what();
    }
  }

  explicit TableSchema(nlohmann::json &oJson) { fromJSON(oJson); }
  ~TableSchema() {}

  std::string toJSON() override;
  void fromJSON(const std::string& json) override;
  void fromJSON(const nlohmann::json &json) override;
  void FromFields(std::vector<FieldType>& fields) override;
  std::shared_ptr<arrow::Schema>& ArrowSchema() {return schema;}

 private:
  void init(const std::vector<std::string> &fields);
  std::vector<FieldType> extractFields(const std::string &json);
  std::vector<FieldType> extractFields(const nlohmann::json &oJson);
  std::shared_ptr<arrow::Schema> schema;
};

// TODO(chenhongbo) Array, Tensor schema

static std::shared_ptr<DatasetSchema>
NewDatasetSchema(DatasetType type, SchemaConstructorParamType param) {
  try {
    switch (type) {
    case DatasetType::TABLE: {
      switch (param.index()) {
      case 0: {  // string
        auto& string_param = std::get<std::string>(param);
        return std::make_shared<TableSchema>(string_param);
      }
      case 1: {  // json
        auto& json_param = std::get<nlohmann::json>(param);
        return std::make_shared<TableSchema>(json_param);
      }
      case 2: {  // arrow schema
        auto& arrow_param = std::get<std::shared_ptr<arrow::Schema>>(param);
        return std::make_shared<TableSchema>(arrow_param);
      }
      default:
        break;
      }
      break;
    }
    case DatasetType::ARRAY:
      LOG(WARNING) << "UnImplement!";
      break;
    case DatasetType::TENSOR:
      LOG(WARNING) << "UnImplement!";
      break;
    default:
      break;
    }
  } catch (std::bad_variant_access &e) {
    LOG(ERROR) << "bad_variant_access: " << e.what()
        << " type: " << static_cast<int>(type);
  }
  return nullptr;
}

class DatasetMeta {
 public:
  DatasetMeta() {}
  // Constructor from dataset object.
  DatasetMeta(const std::shared_ptr<primihub::Dataset> &dataset,
              const std::string &description,
              const DatasetVisbility &visibility,
              const std::string& dataset_access_info);
  // Constructor from json string.
  explicit DatasetMeta(const std::string &json);

  // copy constructor
  DatasetMeta& operator=(const DatasetMeta &meta) {
    this->visibility = meta.visibility;
    this->schema = meta.schema;
    this->description = meta.description;
    this->id = meta.id;
    this->data_url = meta.data_url;
    this->driver_type = meta.driver_type;
    this->data_type = meta.data_type;
    this->access_meta_ = meta.access_meta_;
    this->server_meta_ = meta.server_meta_;
    return *this;
  }

  ~DatasetMeta() {}

  std::string toJSON() const;
  void fromJSON(const std::string &json);

  std::string getDriverType() const { return driver_type; }
  std::string& getDataURL() { return data_url; }
  std::string getDescription() const { return id; }
  std::string getAccessInfo() const {return this->access_meta_; }
  std::string getServerInfo() const {return this->server_meta_;}
  DatasetVisbility Visibility() const {return visibility;}
  std::shared_ptr<DatasetSchema> getSchema() const { return schema; }
  uint64_t getTotalRecords() const { return total_records; }
  DatasetMeta saveAsPublic();

  void setDataURL(const std::string& data_url) {
    this->data_url = data_url;
  }

  void setServerInfo(const std::string& server_info) {
    this->server_meta_ = server_info;
  }

  void SetAccessInfo(const std::string& access_info) {
    access_meta_ = access_info;
  }

  void SetVisibility(int visibility_) {
    visibility = static_cast<DatasetVisbility>(visibility_);
  }

  void SetDatasetId(const std::string& dataset_id) {
    id = dataset_id;
  }

  void SetDriverType(const std::string& type) {
    driver_type = type;
  }

  void SetDataSchema(std::vector<FieldType>& data_types) {
    data_type = DatasetType::TABLE;
    schema = std::make_shared<TableSchema>();
    schema->FromFields(data_types);
  }

 protected:
  /**
   * some meta data like data access info must treat as private,
   * such as: database access info or csv file stored path
   * such information must be keep inside instead of releasing to meta server,
   * so before release, first erase this information
  */
  void removePrivateMeta();
  void clearAccessMeta();

 public:
  DatasetId id;

 private:
  // NOTE data_url format [nodeid:ip:port:/path/to/data]
  std::string data_url;
  std::string server_meta_;    // location_id:ip:port:use_tls           public
  std::string access_meta_;    // data access info must store in local  private
  std::string description;
  DatasetVisbility visibility;
  std::string driver_type;
  DatasetType data_type;
  std::shared_ptr<DatasetSchema> schema;
  uint64_t total_records;
};

}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_
