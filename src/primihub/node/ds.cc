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
#include "src/primihub/common/common.h"

using primihub::service::DatasetMeta;

namespace primihub {
grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context,
        const NewDatasetRequest *request, NewDatasetResponse *response) {
  DatasetMetaInfo meta_info;
  meta_info.driver_type = request->driver();
  meta_info.access_info = request->path();
  meta_info.id = request->fid();
  auto& schema = meta_info.schema;
  const auto& data_field_type = request->data_type();
  for (const auto& field : data_field_type) {
    auto& name = field.name();
    int type = field.type();
    schema.push_back(std::make_tuple(name, type));
  }
  auto& driver_type = meta_info.driver_type;
  LOG(INFO) << "start to create dataset."
      << "meta info: " << meta_info.access_info << " "
      << "fid: " << meta_info.id << " "
      << "driver_type: " << meta_info.driver_type;
  std::shared_ptr<DataDriver> driver{nullptr};
  try {
    auto access_info = this->dataset_service_->createAccessInfo(driver_type, meta_info);
    if (access_info == nullptr) {
      std::string err_msg = "create access info failed";
      throw std::invalid_argument(err_msg);
    }
    driver = DataDirverFactory::getDriver(driver_type, nodelet_addr_, std::move(access_info));
    this->dataset_service_->registerDriver(meta_info.id, driver);
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load dataset from: " << meta_info.access_info << " "
            << "driver_type: " << driver_type << " "
            << "fid: " << meta_info.id << " "
            << "exception: " << e.what();
    response->set_ret_code(2);
    return grpc::Status::OK;
  }

  DatasetMeta mate;
  auto dataset = dataset_service_->newDataset(
      driver, meta_info.id, meta_info.access_info, mate);

  response->set_ret_code(0);
  response->set_dataset_url(mate.getDataURL());
  LOG(INFO) << "dataurl: " << mate.getDataURL();

  return grpc::Status::OK;
}

}  // namespace primihub
