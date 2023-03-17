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
#include <utility>
#include <nlohmann/json.hpp>
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/util/util.h"

using primihub::service::DatasetMeta;

namespace primihub {
grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context,
        const NewDatasetRequest *request, NewDatasetResponse *response) {
    std::string driver_type = request->driver();
    const auto& meta_info = request->path();
    const auto& fid = request->fid();
    LOG(INFO) << "start to create dataset. meta info: " << meta_info << " "
            << "fid: " << fid << " driver_type: " << driver_type;
    std::shared_ptr<DataDriver> driver{nullptr};
    try {
        auto access_info = this->dataset_service_->createAccessInfo(driver_type, meta_info);
        if (access_info == nullptr) {
            std::string err_msg = "create access info failed";
            throw std::invalid_argument(err_msg);
        }
        driver = DataDirverFactory::getDriver(driver_type, nodelet_addr_, std::move(access_info));
        this->dataset_service_->registerDriver(fid, driver);
    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load dataset from path: " << meta_info << " "
                << "driver_type: " << driver_type << " "
                << "fid: " << fid << " "
                << "exception: " << e.what();
        response->set_ret_code(2);
        return grpc::Status::OK;
    }

    DatasetMeta mate;
    auto dataset = dataset_service_->newDataset(driver, fid, meta_info, mate);

    response->set_ret_code(0);
    response->set_dataset_url(mate.getDataURL());
    LOG(INFO) << "dataurl: " << mate.getDataURL();

    return grpc::Status::OK;
}

}  // namespace primihub
