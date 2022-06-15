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
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/factory.h"

using primihub::service::DatasetMeta;

namespace primihub {
    grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context, const NewDatasetRequest *request,
                                NewDatasetResponse *response) {
        LOG(INFO) << "start to create dataset.";
        std::string driver_type = request->driver();
        std::string path = request->path();
        std::string fid = request->fid();

	std::shared_ptr<DataDriver> driver =
            DataDirverFactory::getDriver(driver_type, nodelet_addr_);
        try {
            [[maybe_unused]] auto cursor = driver->read(path);
        } catch (std::exception &e) {
            LOG(ERROR) << "Failed to load dataset from path: "
                       << e.what();
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

} // namespace primihub
