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

#include "src/primihub/data_store/driver.h"

namespace primihub {
// TODO (xxxxxxxxx) move to common operation
std::shared_ptr<arrow::DataType> DataSetAccessInfo::MakeArrowDataType(int type) {
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

retcode DataSetAccessInfo::MakeArrowSchema() {
  std::vector<std::shared_ptr<arrow::Field>> arrow_fields;
  for (const auto& it : schema) {
    auto name = std::get<0>(it);
    int type = std::get<1>(it);
    auto data_type = MakeArrowDataType(type);
    // std::shared_ptr<arrow::Field>
    auto arrow_filed = arrow::field(name, std::move(data_type));
    arrow_fields.push_back(std::move(arrow_filed));
  }
  this->arrow_schema = arrow::schema(std::move(arrow_fields));
  VLOG(5) << "arrow_schema: " << arrow_schema->field_names().size();
  return retcode::SUCCESS;
}

retcode DataSetAccessInfo::fromJsonString(const std::string& meta_info) {
  retcode ret{retcode::SUCCESS};
  try {
    nlohmann::json js_meta = nlohmann::json::parse(meta_info);
    if (js_meta.contains("schema")) {
      nlohmann::json js_schema = nlohmann::json::parse(js_meta["schema"].get<std::string>());
      ret = ParseSchema(js_schema);
    }
    auto js_access_meta = nlohmann::json::parse(js_meta["access_meta"].get<std::string>());
    ret = ParseFromJsonImpl(js_access_meta);
  } catch (std::exception& e) {
    LOG(ERROR) << "parse access info from json string failed, " << e.what();
    return retcode::FAIL;
  }
  return ret;
}

retcode DataSetAccessInfo::fromYamlConfig(const YAML::Node& meta_info) {
  retcode ret{retcode::SUCCESS};
  try {
    if (meta_info["schema"]) {
      ret = ParseSchema(meta_info["schema"].as<YAML::Node>());
    }
    ret = ParseFromYamlConfigImpl(meta_info);
  } catch (std::exception& e) {
    LOG(ERROR) << "parse access info from yaml config string failed, " << e.what();
    return retcode::FAIL;
  }
  return ret;
}

retcode DataSetAccessInfo::ParseSchema(const nlohmann::json& json_schema) {
  try {
    const auto& filed_list = json_schema;
    for (const auto& filed : filed_list) {
      for (const auto& [key, value] : filed.items()) {
        schema.push_back(std::make_tuple(key, value));
      }
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Parse schema from json failed: " << json_schema;
    return retcode::FAIL;
  }
  return MakeArrowSchema();
}

retcode DataSetAccessInfo::ParseSchema(const YAML::Node& yaml_schema) {
  this->schema.clear();
  try {
    for (const auto& field : yaml_schema) {
      auto col_name = field["name"].as<std::string>();
      auto type = field["type"].as<int>();
      schema.push_back(std::make_tuple(col_name, type));
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Parse schema from yaml config failed";
    return retcode::FAIL;
  }
  return MakeArrowSchema();
}
retcode DataSetAccessInfo::SetDatasetSchema(const std::vector<FieldType>& schema_info) {
  this->schema.clear();
  for (const auto& field : schema_info) {
    this->schema.push_back(field);
  }
  return MakeArrowSchema();
}

retcode DataSetAccessInfo::SetDatasetSchema(std::vector<FieldType>&& schema_info) {
  this->schema = std::move(schema_info);
  return MakeArrowSchema();
}

////////////////////// DataDriver /////////////////////////////
std::string DataDriver::getDriverType() const {
  return driver_type;
}

std::string DataDriver::getNodeletAddress() const {
  return nodelet_address;
}

}  // namespace primihub
