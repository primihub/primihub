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

#include <string>

#include <arrow/api.h>
#include <libp2p/protocol/kademlia/content_id.hpp>
#include <nlohmann/json.hpp>

#include "src/primihub/data_store/dataset.h"

namespace primihub {
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
    virtual void fromJSON(std::string json) = 0;
    virtual void fromJSON(const nlohmann::json &json) = 0;
};

// Two-Dimensional table Schema
class TableSchema : public DatasetSchema {
  public:
    explicit TableSchema(std::shared_ptr<arrow::Schema> schema)
        : schema(schema) {}
    explicit TableSchema(std::string &json) {
        nlohmann::json j = nlohmann::json::parse(json);
        fromJSON(j);
    }
    explicit TableSchema(nlohmann::json &oJson) { fromJSON(oJson); }
    ~TableSchema() {}

    std::string toJSON() override;
    void fromJSON(std::string json) override;
    void fromJSON(const nlohmann::json &json) override;
    void fromJSON(std::vector<std::string> &fieldNames);

  private:
    void init(const std::vector<std::string> &fields);
    std::vector<std::string> extractFields(const std::string &json);
    std::vector<std::string> extractFields(const nlohmann::json &oJson);

    std::shared_ptr<arrow::Schema> schema;
};

// TODO(chenhongbo) Array, Tensor schema

static std::shared_ptr<DatasetSchema>
NewDatasetSchema(DatasetType type, SchemaConstructorParamType param) {
    try {
        switch (type) {
        case DatasetType::TABLE: {
            switch (param.index()) {
            case 0:
                break; // TODO string json
            case 1:
                return std::dynamic_pointer_cast<DatasetSchema>(
                    std::make_shared<TableSchema>(
                        std::get<nlohmann::json>(param)));
            case 2:
                return std::make_shared<TableSchema>(
                    std::get<std::shared_ptr<arrow::Schema>>(param));
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
                const DatasetVisbility &visibility);
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
        return *this;
    }

    ~DatasetMeta() {}

    std::string toJSON();
    void fromJSON(const std::string &json);

    std::string getDriverType() const { return driver_type; }
    std::string &getDataURL() { return data_url; }
    std::string getDescription()  { return description; }
    void setDataURL(const std::string &data_url) { this->data_url = data_url; }
    DatasetId id;

  private:
    // TODO nodeid:ip:port:/path/to/data
    std::string data_url; 
    std::string description;
    DatasetVisbility visibility;
    std::string driver_type;
    DatasetType data_type;
    std::shared_ptr<DatasetSchema> schema;
};

} // namespace service
} // namespace primihub

#endif // SRC_PRIMIHUB_SERVICE_DATASET_MODEL_H_
