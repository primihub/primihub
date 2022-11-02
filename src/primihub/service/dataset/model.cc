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
#include <libp2p/multi/content_identifier_codec.hpp>

#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/driver.h"


namespace primihub {
    class DataDriver;
}
namespace primihub::service {



    // ======== DatasetSchema ========

    std::vector<std::string> TableSchema::extractFields(const std::string &json) {
        nlohmann::json oJson = nlohmann::json::parse(json);
        return extractFields(oJson);
    }

     std::vector<std::string> TableSchema::extractFields(const nlohmann::json& oJson) {
        std::vector<std::string> fields;
        for (auto &it : oJson.items()) {
            fields.push_back(it.key());
        }
        return fields;
    }

    std::string TableSchema::toJSON () {
        std::string json = "{";

        for (int iCol = 0; iCol < schema->num_fields(); ++iCol) {
            std::shared_ptr<arrow::Field> field = schema->field(iCol);
            json += "\"" + field->name() + "\":[],";
        }

        json = json.substr(0, json.size() - 1);
        json += "}";

        return json;
    }

    void TableSchema::fromJSON(std::string json) {
        std::vector<std::string> fieldNames = this->extractFields(json);
        fromJSON(fieldNames);
    }

    void TableSchema::fromJSON(const nlohmann::json& json) {
        std::vector<std::string> fieldNames = this->extractFields(json);
        fromJSON(fieldNames);
    }

    void TableSchema::fromJSON(std::vector<std::string>& fieldNames) {
        std::vector<std::shared_ptr<arrow::Field>> arrowFields;
        for (auto &it : fieldNames) {
            std::shared_ptr<arrow::Field> arrowField = arrow::field(it, arrow::int64());
            arrowFields.push_back(arrowField);
        }
        this->schema = arrow::schema({});
        this->schema = arrow::schema(arrowFields);
    }



    // ======= DataSetMeta =======

    // Constructor from local dataset object.
    DatasetMeta::DatasetMeta(const std::shared_ptr<primihub::Dataset> & dataset,
                             const std::string & description,
                             const DatasetVisbility &visibility)
                            : visibility(visibility) {

        SchemaConstructorParamType arrowSchemaParam = std::get<0>(dataset->data)->schema(); // TODO schema may not be available
        this->data_type = DatasetType::TABLE;   // FIXME only support table now
        this->schema = NewDatasetSchema(data_type, arrowSchemaParam);
        this->total_records = std::get<0>(dataset->data)->num_rows();
        // TODO Maybe description is duplicated with other dataset?
        this->id = DatasetId(description);
        this->description = description;
        this->driver_type =  dataset->getDataDriver()->getDriverType();
        auto nodelet_addr = dataset->getDataDriver()->getNodeletAddress();
        this->data_url =  nodelet_addr + ":" + dataset->getDataDriver()->getDataURL(); // TODO string connect node address
        VLOG(5) << "data_url: " << this->data_url;
    }

    DatasetMeta::DatasetMeta(const std::string& json) {
        fromJSON(json);
    }

    std::string DatasetMeta::toJSON()
    {
        std::stringstream ss;
        auto sid = libp2p::multi::ContentIdentifierCodec::toString(libp2p::multi::ContentIdentifierCodec::decode(id.data).value());
        nlohmann::json j;
        j["id"] = sid.value();
        j["data_url"] = data_url;
        j["data_type"] = static_cast<int>(data_type);
        j["description"] = description;
        j["visibility"] = static_cast<int>(visibility);
        j["schema"] = schema->toJSON();
        j["driver_type"] = driver_type;
        ss << std::setw(4) << j;
        return ss.str();
    }


    void DatasetMeta::fromJSON(const std::string &json)
    {
        nlohmann::json oJson = nlohmann::json::parse(json);

        auto sid = oJson["id"].get<std::string>();
        auto _id_data = libp2p::multi::ContentIdentifierCodec::encode(libp2p::multi::ContentIdentifierCodec::fromString(sid).value());
        this->id.data = _id_data.value();

        this->data_url = oJson["data_url"].get<std::string>();
        this->description = oJson["description"].get<std::string>();
        this->data_type = oJson["data_type"].get<primihub::service::DatasetType>();
        this->visibility = oJson["visibility"].get<DatasetVisbility>(); // string to enum
        // Construct schema from data type(Table, Array, Tensor)
        SchemaConstructorParamType oJsonSchemaParam = oJson["schema"];
        this->schema = NewDatasetSchema(this->data_type, oJsonSchemaParam);
        this->driver_type = oJson["driver_type"].get<std::string>();
    }

}  // namespace primihub::service
