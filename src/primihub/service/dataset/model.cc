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


#include <string>
#include <sstream> // string stream
#include <iterator> // ostream_iterator
#include <glog/logging.h>
#include <arrow/type.h>
#include <nlohmann/json.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>

#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
    class DataDriver;
}
namespace primihub::service {
    // ======== DatasetSchema ========
// key: filed name, value: data type
// TODO(xxxxx) move to common operation
std::shared_ptr<arrow::DataType> TableSchema::MakeArrowDataType(int type) {
  switch (type) {
  case arrow::Type::type::BOOL:
    return arrow::boolean();
  case arrow::Type::type::UINT8:
    return arrow::uint8();
  case arrow::Type::type::INT8:
    return arrow::int8();
  case arrow::Type::type::UINT16:
    return arrow::uint16();
  case arrow::Type::type::INT16:
    return arrow::int16();
  case arrow::Type::type::UINT32:
    return arrow::uint32();
  case arrow::Type::type::INT32:
    return arrow::int32();
  case arrow::Type::type::UINT64:
    return arrow::uint64();
  case arrow::Type::type::INT64:
    return arrow::int64();
  case arrow::Type::type::HALF_FLOAT:
    return arrow::float16();
  case arrow::Type::type::FLOAT:
    return arrow::float32();
  case arrow::Type::type::DOUBLE:
    return arrow::float64();
  case arrow::Type::type::STRING:
    return arrow::utf8();
  case arrow::Type::type::BINARY:
    return arrow::binary();
  case arrow::Type::type::DATE32:
    return arrow::date32();
  case arrow::Type::type::DATE64:
    return arrow::date64();
  case arrow::Type::type::TIMESTAMP:
    return arrow::timestamp(arrow::TimeUnit::type::MILLI);
  case arrow::Type::type::TIME32:
    return arrow::time32(arrow::TimeUnit::type::MILLI);
  case arrow::Type::type::TIME64:
    return arrow::time64(arrow::TimeUnit::type::MICRO);
  // case arrow::Type::type::DECIMAL128:
  case arrow::Type::type::DECIMAL:
    return arrow::decimal(10, 0);
  case arrow::Type::type::DECIMAL256:
    return arrow::decimal(10, 0);
  default:
    return arrow::utf8();
  }
  return arrow::utf8();
}
std::vector<FieldType> TableSchema::extractFields(const std::string &json) {
  try {
    nlohmann::json oJson = nlohmann::json::parse(json);
    return extractFields(oJson);
  } catch (std::exception& e) {
    LOG(ERROR) << "extractFields failed: " << e.what();
  }
  return std::vector<FieldType>();
}

std::vector<FieldType> TableSchema::extractFields(const nlohmann::json& oJson) {
  std::vector<FieldType> fields;
  if (oJson.is_array()) {
    for (const auto& it : oJson) {
      for (const auto&[name, type] : it.items()) {
        fields.push_back(std::make_tuple(name, type));
      }
    }
  } else {
    LOG(ERROR) << "no need to parser";
  }
  return fields;
}

std::string TableSchema::toJSON () {
  std::stringstream ss;
  nlohmann::json js = nlohmann::json::array();
  for (int iCol = 0; iCol < schema->num_fields(); ++iCol) {
    std::shared_ptr<arrow::Field> field = schema->field(iCol);
    nlohmann::json item;
    item[field->name()] = field->type()->id();
    js.emplace_back(item);
  }
  ss << js;
  return ss.str();
}

void TableSchema::fromJSON(const std::string& json) {
  auto field_info = this->extractFields(json);
  fromJSON(field_info);
}

void TableSchema::fromJSON(const nlohmann::json& json) {
  auto field_info = this->extractFields(json);
  fromJSON(field_info);
}

void TableSchema::fromJSON(std::vector<FieldType>& field_info) {
  std::vector<std::shared_ptr<arrow::Field>> arrowFields;
  for (const auto& it : field_info) {
    auto name = std::get<0>(it);
    int type = std::get<1>(it);
    auto data_type = MakeArrowDataType(type);
    // std::shared_ptr<arrow::Field>
    auto arrow_filed = arrow::field(name, std::move(data_type));
    arrowFields.push_back(std::move(arrow_filed));
  }
  if (arrowFields.empty()) {
    LOG(WARNING) << "arrowFields is empty";
    this->schema = arrow::schema({});
  } else {
    this->schema = arrow::schema(arrowFields);
  }
}



// ======= DataSetMeta =======

// Constructor from local dataset object.
DatasetMeta::DatasetMeta(const std::shared_ptr<primihub::Dataset> & dataset,
                        const std::string& description,
                        const DatasetVisbility& visibility,
                        const std::string& dataset_access_info)
                        : visibility(visibility) {
  SchemaConstructorParamType arrowSchemaParam = std::get<0>(dataset->data)->schema();
  this->data_type = DatasetType::TABLE;   // FIXME only support table now
  this->schema = NewDatasetSchema(data_type, arrowSchemaParam);
  this->total_records = std::get<0>(dataset->data)->num_rows();
  // TODO Maybe description is duplicated with other dataset?
  this->id = DatasetId(description);
  this->description = description;
  this->driver_type = dataset->getDataDriver()->getDriverType();
  auto nodelet_addr = dataset->getDataDriver()->getNodeletAddress();
  this->data_url = nodelet_addr + ":" + dataset->getDataDriver()->getDataURL();
  this->server_meta_ = nodelet_addr;
  this->access_meta_ = dataset_access_info;
  VLOG(5) << "data_url: " << this->data_url;
}

DatasetMeta::DatasetMeta(const std::string& json) {
    fromJSON(json);
}

std::string DatasetMeta::toJSON() {
    std::stringstream ss;
    auto sid = libp2p::multi::ContentIdentifierCodec::toString(
            libp2p::multi::ContentIdentifierCodec::decode(id.data).value());
    nlohmann::json j;
    j["id"] = sid.value();
    j["data_url"] = data_url;
    j["data_type"] = static_cast<int>(data_type);
    j["description"] = description;
    j["visibility"] = static_cast<int>(visibility);
    j["schema"] = schema->toJSON();
    j["driver_type"] = driver_type;
    j["server_meta"] = server_meta_;
    j["access_meta"] = access_meta_;
    ss << std::setw(4) << j;
    return ss.str();
}

void DatasetMeta::fromJSON(const std::string &json) {
    nlohmann::json oJson = nlohmann::json::parse(json);
    auto sid = oJson["id"].get<std::string>();
    auto _id_data = libp2p::multi::ContentIdentifierCodec::encode(
            libp2p::multi::ContentIdentifierCodec::fromString(sid).value());
    this->id.data = _id_data.value();
    this->data_url = oJson["data_url"].get<std::string>();
    this->description = oJson["description"].get<std::string>();
    this->data_type = oJson["data_type"].get<primihub::service::DatasetType>();
    this->visibility = oJson["visibility"].get<DatasetVisbility>(); // string to enum
    // Construct schema from data type(Table, Array, Tensor)
    SchemaConstructorParamType oJsonSchemaParam = oJson["schema"].get<std::string>();
    this->schema = NewDatasetSchema(this->data_type, oJsonSchemaParam);
    this->driver_type = oJson["driver_type"].get<std::string>();
    if (oJson.contains("server_meta")) {
        this->server_meta_ = oJson["server_meta"].get<std::string>();
    }
    if (oJson.contains("access_meta")) {
        this->access_meta_ = oJson["access_meta"].get<std::string>();
    }
}

void DatasetMeta::removePrivateMeta() {
    clearAccessMeta();
}

void DatasetMeta::clearAccessMeta() {
    this->access_meta_ = "";
}

DatasetMeta DatasetMeta::saveAsPublic() {
    DatasetMeta public_dataset_meta = *this;
    public_dataset_meta.removePrivateMeta();
    return public_dataset_meta;
}

}  // namespace primihub::service
