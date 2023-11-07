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

#include <glog/logging.h>
#include <arrow/type.h>

#include <string>
#include <sstream>  // string stream
#include <iterator>  // ostream_iterator
#include <nlohmann/json.hpp>

#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/driver.h"

namespace primihub::service {
class DataDriver;

// ======== DatasetSchema ========
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

std::string TableSchema::toJSON() {
  std::stringstream ss;
  nlohmann::json js = nlohmann::json::array();
  if (schema == nullptr) {
    // do nothing
  } else {
    for (int iCol = 0; iCol < schema->num_fields(); ++iCol) {
      std::shared_ptr<arrow::Field> field = schema->field(iCol);
      nlohmann::json item;
      item[field->name()] = field->type()->id();
      js.emplace_back(item);
    }
  }
  ss << js;
  return ss.str();
}

void TableSchema::fromJSON(const std::string& json) {
  auto field_info = this->extractFields(json);
  FromFields(field_info);
}

void TableSchema::fromJSON(const nlohmann::json& json) {
  auto field_info = this->extractFields(json);
  FromFields(field_info);
}

void TableSchema::FromFields(std::vector<FieldType>& field_info) {
  std::vector<std::shared_ptr<arrow::Field>> arrowFields;
  for (const auto& it : field_info) {
    auto name = std::get<0>(it);
    int type = std::get<1>(it);
    auto data_type = arrow_wrapper::util::MakeArrowDataType(type);
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
  // this->total_records = std::get<0>(dataset->data)->num_rows();
  // Maybe description is duplicated with other dataset?
  this->id = DatasetId(description);
  this->description = description;
  this->driver_type = dataset->getDataDriver()->getDriverType();
  auto nodelet_addr = dataset->getDataDriver()->getNodeletAddress();
  this->data_url = nodelet_addr + ":" + dataset->getDataDriver()->getDataURL();
  this->server_meta_ = nodelet_addr;
  this->access_meta_ = dataset_access_info;
  VLOG(5) << "dataset_access_info: " << this->access_meta_;
}

DatasetMeta::DatasetMeta(const std::string& json) {
    fromJSON(json);
}

std::string DatasetMeta::toJSON() const {
    std::stringstream ss;
    nlohmann::json j;
    j["id"] = id;
    j["data_url"] = data_url;
    j["data_type"] = static_cast<int>(data_type);
    j["description"] = description;
    j["visibility"] = static_cast<int>(visibility);
    if (schema == nullptr) {
      j["schema"] = "[]";  // empty
    } else {
      j["schema"] = schema->toJSON();
    }
    j["driver_type"] = driver_type;
    j["server_meta"] = server_meta_;
    j["access_meta"] = access_meta_;
    // ss << std::setw(4) << j;
    ss << j;
    return ss.str();
}

void DatasetMeta::fromJSON(const std::string &json) {
    nlohmann::json oJson = nlohmann::json::parse(json);
    auto sid = oJson["id"].get<std::string>();
    this->id = sid;
    this->data_url = oJson["data_url"].get<std::string>();
    this->description = oJson["description"].get<std::string>();
    this->data_type = oJson["data_type"].get<primihub::service::DatasetType>();
    this->visibility = oJson["visibility"].get<DatasetVisbility>();  // string to enum
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
