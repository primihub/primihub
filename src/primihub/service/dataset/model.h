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
#ifndef SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_

#include <arrow/api.h>
#include <libp2p/protocol/kademlia/content_id.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/util/arrow_wrapper_util.h"

namespace primihub {
using FieldType = std::tuple<std::string, int>;
namespace service {
enum class DatasetVisbility {
    PRIVATE = 0,
    PUBLIC = 1,
};

enum class DatasetType {
    TABLE = 0,
    ARRAY = 1,
    TENSOR = 2,
};

using SchemaConstructorParamType =
    std::variant<std::string, nlohmann::json, std::shared_ptr<arrow::Schema>>;

class DatasetSchema {
 public:
    virtual ~DatasetSchema() {}
    virtual std::string toJSON() = 0;
    virtual void fromJSON(const std::string& json) = 0;
    virtual void fromJSON(const nlohmann::json &json) = 0;
};

// Two-Dimensional table Schema
class TableSchema : public DatasetSchema {
 public:
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
    void fromJSON(std::vector<FieldType> &fieldNames);

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
      break;
    case DatasetType::TENSOR:
      break;
    default:
      break;
    }
  } catch (std::bad_variant_access &e) {
    std::cerr << "bad_variant_access: " << e.what() << std::endl;
  }
  return nullptr;
}

using DatasetId = libp2p::protocol::kademlia::ContentId;

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
    DatasetMeta &operator=(const DatasetMeta &meta) {
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

    std::string toJSON();
    void fromJSON(const std::string &json);

    std::string getDriverType() const { return driver_type; }
    std::string& getDataURL() { return data_url; }
    std::string getDescription()  { return description; }
    std::string getAccessInfo() {return this->access_meta_; }
    std::string getServerInfo() {return this->server_meta_;}
    void setDataURL(const std::string &data_url) { this->data_url = data_url; }
    void setServerInfo(const std::string &server_info) { this->server_meta_ = server_info; }
    std::shared_ptr<DatasetSchema> getSchema() { return schema; }
    uint64_t getTotalRecords() { return total_records; }
    DatasetId id;
    DatasetMeta saveAsPublic();

  protected:
    /**
     * some meta data like data access info must treat as private,
     * such as: database access info or csv file stored path
     * such infomation must be keep inside instead of releasing to meta server,
     * so before release, first erase this infomation
    */
    void removePrivateMeta();
    void clearAccessMeta();

  private:
    // NOTE data_url format [nodeid:ip:port:/path/to/data]
    std::string data_url;
    std::string server_meta_;              // location_id:ip:port:use_tls           public
    std::string access_meta_;              // data access info must store in local  private
    std::string description;
    DatasetVisbility visibility;
    std::string driver_type;
    DatasetType data_type;
    std::shared_ptr<DatasetSchema> schema;
    uint64_t total_records;
};

} // namespace service
} // namespace primihub

#endif // SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_
