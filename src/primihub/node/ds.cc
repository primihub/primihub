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

#include "src/primihub/node/ds.h"
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/factory.h"

using primihub::service::DatasetMeta;

namespace primihub {
grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context, const NewDatasetRequest *request,
                            NewDatasetResponse *response) {
    std::string driver_type = request->driver();
    std::string path = request->path();
    std::string fid = request->fid();
    VLOG(5) << "driver_type: " << driver_type << " fid: " << fid
        << " paht: " << path;
    LOG(INFO) << "start to create dataset."<<" path: "<< path
            <<" fid: "<< fid <<" driver_type: "<< driver_type;
    std::shared_ptr<DataDriver> driver{nullptr};
    try {
        driver = DataDirverFactory::getDriver(driver_type, nodelet_addr_);
        processMetaData(driver_type, &path);   // if modify needed, inplace
        [[maybe_unused]] auto cursor = driver->read(path);
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load dataset from path: "<< path <<" "
                << "driver_type: "<< driver_type << " "
                << "fid: "<< fid << " "
                << "exception:" << e.what();
        response->set_ret_code(2);
        return grpc::Status::OK;
    }

    DatasetMeta mate;
    auto dataset = dataset_service_->newDataset(driver, fid, mate);

    response->set_ret_code(0);
    response->set_dataset_url(mate.getDataURL());
    LOG(INFO) << "dataurl: " << mate.getDataURL();

    return grpc::Status::OK;
}

int DataServiceImpl::processMetaData(const std::string& driver_type, std::string* meta_data) {
    std::string driver_type_ = driver_type;
    // to upper
    std::transform(driver_type_.begin(), driver_type_.end(), driver_type_.begin(), ::toupper);
    if (driver_type_ == "SQLITE") {
        nlohmann::json js = nlohmann::json::parse(*meta_data);
        // driver_type#db_path#table_name#column
        auto& meta = *meta_data;
        meta = driver_type;
        VLOG(5) << meta;
        if (!js.contains("db_path")) {
            LOG(ERROR) << "key: db_path is not found";
            return -1;
        }
        meta.append("#").append(js["db_path"]);
        if (!js.contains("tableName")) {
            LOG(ERROR) << "key: tableName is not found";
            return -1;
        }
        meta.append("#").append(js["tableName"]);
        if (js.contains("query_index")) {
            meta.append("#").append(js["query_index"]);
        } else {
            meta.append("#");
        }
        VLOG(5) << "sqlite info: " << meta;
    }
    return 0;
}

} // namespace primihub
