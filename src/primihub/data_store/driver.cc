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

#include "src/primihub/data_store/driver.h"
#include "src/primihub/util/arrow_wrapper_util.h"

namespace primihub {

retcode DataSetAccessInfo::MakeArrowSchema() {
  std::vector<std::shared_ptr<arrow::Field>> arrow_fields;
  for (const auto& it : schema) {
    auto name = std::get<0>(it);
    int type = std::get<1>(it);
    auto data_type = arrow_wrapper::util::MakeArrowDataType(type);
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

retcode DataSetAccessInfo::FromMetaInfo(const DatasetMetaInfo& meta_info) {
  retcode ret{retcode::SUCCESS};
  // try {
    if (!meta_info.schema.empty()) {
      for (const auto& field : meta_info.schema) {
        std::string name = std::get<0>(field);
        int type = std::get<1>(field);
        schema.push_back(std::make_tuple(name, type));
      }
      MakeArrowSchema();
    }
    ret = ParseFromMetaInfoImpl(meta_info);
  // } catch (std::exception& e) {
  //   LOG(ERROR) << "parse access info from yaml config string failed, " << e.what();
  //   return retcode::FAIL;
  // }
  return ret;
}

retcode DataSetAccessInfo::ParseSchema(const nlohmann::json& json_schema) {
  {
    std::shared_lock<std::shared_mutex> lck(this->schema_mtx);
    if (!this->schema.empty()) {
      return retcode::SUCCESS;
    }
  }
  std::lock_guard<std::shared_mutex> lck(this->schema_mtx);
  if (!this->schema.empty()) {
    return retcode::SUCCESS;
  }
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
  {
    std::shared_lock<std::shared_mutex> lck(this->schema_mtx);
    if (!this->schema.empty()) {
      return retcode::SUCCESS;
    }
  }
  std::lock_guard<std::shared_mutex> lck(this->schema_mtx);
  if (!this->schema.empty()) {
    return retcode::SUCCESS;
  }
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
  {
    std::shared_lock<std::shared_mutex> lck(this->schema_mtx);
    if (!this->schema.empty()) {
      return retcode::SUCCESS;
    }
  }
  std::lock_guard<std::shared_mutex> lck(this->schema_mtx);
  if (!this->schema.empty()) {
    return retcode::SUCCESS;
  }
  for (const auto& field : schema_info) {
    this->schema.push_back(field);
  }
  return MakeArrowSchema();
}

retcode DataSetAccessInfo::SetDatasetSchema(std::vector<FieldType>&& schema_info) {
  {
    std::shared_lock<std::shared_mutex> lck(this->schema_mtx);
    if (!this->schema.empty()) {
      return retcode::SUCCESS;
    }
  }
  std::lock_guard<std::shared_mutex> lck(this->schema_mtx);
  if (!this->schema.empty()) {
    return retcode::SUCCESS;
  }
  this->schema = std::move(schema_info);
  return MakeArrowSchema();
}

std::string DataSetAccessInfo::SchemaToJsonString() {
  auto schema = this->ArrowSchema();
  if (schema == nullptr) {
    return std::string("");
  }
  nlohmann::json js_schema = nlohmann::json::array();
  for (int col_index = 0; col_index < schema->num_fields(); ++col_index) {
    auto field = schema->field(col_index);
    nlohmann::json item;
    item[field->name()] = field->type()->id();
    js_schema.emplace_back(item);
  }
  return js_schema.dump();
}


// cursor
std::shared_ptr<arrow::Schema>
Cursor::MakeArrowSchema(const std::vector<FieldType>& data_schema) {
  std::vector<std::shared_ptr<arrow::Field>> arrow_fields;
  for (const auto& it : data_schema) {
    auto name = std::get<0>(it);
    int type = std::get<1>(it);
    auto data_type = arrow_wrapper::util::MakeArrowDataType(type);
    // std::shared_ptr<arrow::Field>
    auto arrow_filed = arrow::field(name, std::move(data_type));
    arrow_fields.push_back(std::move(arrow_filed));
  }
  auto arrow_schema = arrow::schema(std::move(arrow_fields));
  VLOG(5) << "arrow_schema: " << arrow_schema->field_names().size();
  return arrow_schema;
}
////////////////////// DataDriver /////////////////////////////
std::string DataDriver::getDriverType() const {
  return driver_type;
}

std::string DataDriver::getNodeletAddress() const {
  return nodelet_address;
}

}  // namespace primihub
