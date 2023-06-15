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
#include "src/primihub/util/thread_local_data.h"

using primihub::service::DatasetMeta;

namespace primihub {
grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context,
                                          const rpc::NewDatasetRequest *request,
                                          rpc::NewDatasetResponse *response) {
  DatasetMetaInfo meta_info;
  // convert pb to DatasetMetaInfo
  ConvertToDatasetMetaInfo(request->meta_info(), &meta_info);
  auto op_type = request->op_type();
  switch (op_type) {
  case rpc::NewDatasetRequest::REGISTER:
    RegisterDatasetProcess(meta_info, response);
    break;
  case rpc::NewDatasetRequest::UNREGISTER:
    UnRegisterDatasetProcess(meta_info, response);
    break;
  case rpc::NewDatasetRequest::UPDATE:
    UpdateDatasetProcess(meta_info, response);
    break;
  default: {
    std::string err_msg = "unsupported operation type: ";
    err_msg.append(std::to_string(op_type));
    SetResponseErrorMsg(std::move(err_msg), response);
  }
  }
  return grpc::Status::OK;
}

retcode DataServiceImpl::RegisterDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  auto& driver_type = meta_info.driver_type;
  VLOG(2) << "start to create dataset."
      << "meta info: " << meta_info.access_info << " "
      << "fid: " << meta_info.id << " "
      << "driver_type: " << meta_info.driver_type;
  std::shared_ptr<DataDriver> driver{nullptr};
  std::string access_meta;
  try {
    auto access_info = this->GetDatasetService()->createAccessInfo(driver_type, meta_info);
    if (access_info == nullptr) {
      std::string err_msg = "create access info failed";
      throw std::invalid_argument(err_msg);
    }
    access_meta = access_info->toString();
    driver = DataDirverFactory::getDriver(driver_type,
                                          DatasetLocation(),
                                          std::move(access_info));
    this->GetDatasetService()->registerDriver(meta_info.id, driver);
  } catch (std::exception& e) {
    auto& err_info = ThreadLocalErrorMsg();
    LOG(ERROR) << "Failed to load dataset from: "
            << meta_info.access_info << " "
            << "driver_type: " << driver_type << " "
            << "fid: " << meta_info.id << " "
            << "exception: " << err_info;
    SetResponseErrorMsg(err_info, reply);
    ResetThreadLocalErrorMsg();
    return retcode::FAIL;
  }

  DatasetMeta mate;
  auto dataset = GetDatasetService()->newDataset(
      driver, meta_info.id, access_meta, &mate);
  if (dataset == nullptr) {
    auto& err_msg = ThreadLocalErrorMsg();
    SetResponseErrorMsg(err_msg, reply);
    ResetThreadLocalErrorMsg();
    LOG(ERROR) << "register dataset " << meta_info.id << " failed";
    this->GetDatasetService()->unRegisterDriver(meta_info.id);
    return retcode::FAIL;
  } else {
    reply->set_ret_code(rpc::NewDatasetResponse::SUCCESS);
    reply->set_dataset_url(mate.getDataURL());
    LOG(INFO) << "end of register dataset, dataurl: " << mate.getDataURL();
  }

  return retcode::SUCCESS;
}

retcode DataServiceImpl::UnRegisterDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  reply->set_ret_code(rpc::NewDatasetResponse::SUCCESS);
  return retcode::SUCCESS;
}

retcode DataServiceImpl::UpdateDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  return RegisterDatasetProcess(meta_info, reply);
}

retcode DataServiceImpl::ConvertToDatasetMetaInfo(
    const rpc::MetaInfo& meta_info_pb,
    DatasetMetaInfo* meta_info_ptr) {
  auto& meta_info = *meta_info_ptr;
  meta_info.driver_type = meta_info_pb.driver();
  meta_info.access_info = meta_info_pb.access_info();
  meta_info.id = meta_info_pb.id();
  meta_info.visibility = static_cast<Visibility>(meta_info_pb.visibility());
  auto& schema = meta_info.schema;
  const auto& data_field_type = meta_info_pb.data_type();
  for (const auto& field : data_field_type) {
    auto& name = field.name();
    int type = field.type();
    schema.push_back(std::make_tuple(name, type));
  }
  return retcode::SUCCESS;
}

template<typename T>
void DataServiceImpl::SetResponseErrorMsg(std::string&& err_msg, T* reply) {
  reply->set_ret_code(T::FAIL);
  reply->set_ret_msg(std::move(err_msg));
}

template<typename T>
void DataServiceImpl::SetResponseErrorMsg(const std::string& err_msg, T* reply) {
  reply->set_ret_code(T::FAIL);
  reply->set_ret_msg(err_msg);
}

}  // namespace primihub
