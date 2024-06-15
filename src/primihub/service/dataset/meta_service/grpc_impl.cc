// Copyright [2023] <PrimiHub>

#include "src/primihub/service/dataset/meta_service/grpc_impl.h"
#include <glog/logging.h>
#include <map>
#include <utility>
#include <tuple>
#include <string>
#include "src/primihub/common/value_check_util.h"

namespace primihub::service {
GrpcDatasetMetaService::GrpcDatasetMetaService(const Node& server_cfg) {
  meta_service_ = server_cfg;
  VLOG(5) << meta_service_.to_string();
  std::string server_address = meta_service_.ip() + ":" + std::to_string(meta_service_.port());
  std::shared_ptr<grpc::ChannelCredentials> creds{nullptr};
  grpc::ChannelArguments channel_args;
  // channel_args.SetMaxReceiveMessageSize(128*1024*1024);
  if (meta_service_.use_tls()) {
    // auto link_context = this->getLinkContext();
    // auto& cert_config = link_context->getCertificateConfig();
    // grpc::SslCredentialsOptions ssl_opts;
    // ssl_opts.pem_root_certs = cert_config.rootCAContent();
    // ssl_opts.pem_private_key = cert_config.keyContent();
    // ssl_opts.pem_cert_chain = cert_config.certContent();
    // creds = grpc::SslCredentials(ssl_opts);
  } else {
    creds = grpc::InsecureChannelCredentials();
  }
  grpc_channel_ = grpc::CreateCustomChannel(server_address, creds, channel_args);
  stub_ = rpc::DataSetService::NewStub(grpc_channel_);
}

retcode GrpcDatasetMetaService::PutMeta(const DatasetMeta& meta) {
  VLOG(5) << "GrpcDatasetMetaService::PutMeta: " << meta.toJSON();
  rpc::NewDatasetRequest request;
  request.set_op_type(rpc::NewDatasetRequest::REGISTER);
  auto meta_info = request.mutable_meta_info();
  MetaToPbMetaInfo(meta, meta_info);
  int retry_time{0};
  do {
    rpc::NewDatasetResponse reply;
    grpc::ClientContext context;
    grpc::Status status = stub_->NewDataset(&context, request, &reply);
    if (status.ok()) {
      if (reply.ret_code() != rpc::NewDatasetResponse::SUCCESS) {
        LOG(ERROR) << "error msg: " << reply.ret_msg();
        return retcode::FAIL;
      }
      VLOG(5) << "PutMeta to node: ["
              <<  meta_service_.to_string() << "] rpc succeeded.";
    } else {
      LOG(WARNING) << "PutMeta to Node ["
                <<  meta_service_.to_string() << "] rpc failed. "
                << status.error_code() << ": " << status.error_message();
      if (retry_time < retry_max_times_) {
        retry_time++;
        continue;
      } else {
        LOG(WARNING) << "PutMeta to Node ["
            << meta_service_.to_string() << "] rpc failed. reaches max retry times: "
            << this->retry_max_times_ << " abort this operation";
        return retcode::FAIL;
      }
    }
    break;
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcDatasetMetaService::GetMeta(const DatasetId& dataset_id,
                                        FoundMetaHandler handler) {
  rpc::GetDatasetRequest request;
  request.add_id(dataset_id);
  for (const auto& id : request.id()) {
    VLOG(5) << "dataset id: " << id;
  }
  int retry_time{0};
  do {
    grpc::ClientContext context;
    rpc::GetDatasetResponse reply;
    grpc::Status status = stub_->GetDataset(&context, request, &reply);
    if (status.ok()) {
      if (reply.ret_code() != rpc::GetDatasetResponse::SUCCESS) {
        LOG(ERROR) << "get dataset meta info failed: " << reply.ret_msg();
        return retcode::FAIL;
      }
      VLOG(5) << "GetDataset from node: ["
              <<  meta_service_.to_string() << "] rpc succeeded.";
    } else {
      LOG(WARNING) << "GetDataset from Node ["
                <<  meta_service_.to_string() << "] rpc failed. "
                << status.error_code() << ": " << status.error_message();
      if (retry_time < retry_max_times_) {
        retry_time++;
        continue;
      } else {
        LOG(ERROR) << "GetDataset from: ["
                  << meta_service_.to_string() << "] rpc failed reaches max retry times: "
                  << retry_max_times_ << ", abort this operation";
        return retcode::FAIL;
      }
    }
    auto dataset_list = reply.data_set();
    VLOG(5) << "dataset_list: " << dataset_list.size();
    auto meta = std::make_shared<DatasetMeta>();
    for (const auto& dataset : dataset_list) {
      auto& meta_info = dataset.meta_info();
      if (dataset.available() == rpc::DatasetData::Available) {
        VLOG(5) << "data id: " << meta_info.id() << " "
          << "access_info: " << meta_info.access_info();
        PbMetaInfoToMeta(meta_info, meta.get());
        auto ret = handler(meta);
        if (ret != retcode::SUCCESS) {
          LOG(ERROR) << "handle meta info encountes error";
          return retcode::FAIL;
        }
        break;
      } else {
        LOG(WARNING) << "dataset id: " << meta_info.id() << " is not available";
        return retcode::FAIL;
      }
    }
    break;
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcDatasetMetaService::FindPeerListFromDatasets(
    const std::vector<DatasetWithParamTag>& datasets_with_tag,
    FoundMetaListHandler handler) {
  if (datasets_with_tag.empty()) {
    LOG(ERROR) << "no dataset need to get meta info";
    return retcode::FAIL;
  }
  std::vector<DatasetMetaWithParamTag> meta_list;
  rpc::GetDatasetRequest request;
  std::map<std::string, std::string> id2tag;
  for (const auto& pair : datasets_with_tag) {
    auto& dataset_id = std::get<0>(pair);
    auto& dataset_tag = std::get<1>(pair);
    VLOG(5) << "dataset_id: " << dataset_id << " "
        << "dataset_tag: " << dataset_tag;
    request.add_id(dataset_id);
    id2tag[dataset_id] = dataset_tag;
  }
  if (VLOG_IS_ON(5)) {
    for (const auto& id : request.id()) {
      VLOG(5) << "dataset id: " << id;
    }
  }
  int retry_time{0};
  do {
    grpc::ClientContext context;
    rpc::GetDatasetResponse reply;
    grpc::Status status = stub_->GetDataset(&context, request, &reply);
    if (status.ok()) {
      if (reply.ret_code() != rpc::GetDatasetResponse::SUCCESS) {
        LOG(ERROR) << "get dataset meta info failed: " << reply.ret_msg();
        return retcode::FAIL;
      }
      VLOG(5) << "GetDataset from node: ["
              <<  meta_service_.to_string() << "] rpc succeeded.";
    } else {
      LOG(WARNING) << "GetDataset from Node ["
                <<  meta_service_.to_string() << "] rpc failed. "
                << status.error_code() << ": " << status.error_message();
      if (retry_time < retry_max_times_) {
        retry_time++;
        continue;
      } else {
        LOG(ERROR) << "GetDataset from: ["
                  << meta_service_.to_string() << "] rpc failed reaches max retry times: "
                  << retry_max_times_ << ", abort this operation";
        return retcode::FAIL;
      }
    }
    auto dataset_list = reply.data_set();
    VLOG(5) << "dataset_list: " << dataset_list.size();
    for (const auto& dataset : dataset_list) {
      auto& meta_info = dataset.meta_info();
      if (dataset.available() == rpc::DatasetData::Available) {
        VLOG(5) << "data id: " << meta_info.id() << " "
          << "access_info: " << meta_info.access_info() << " "
          << "address: " << meta_info.address();
        auto meta = std::make_shared<DatasetMeta>();
        PbMetaInfoToMeta(meta_info, meta.get());
        auto it = id2tag.find(meta_info.id());
        if (it != id2tag.end()) {
          auto& dataset_tag = it->second;
          meta_list.emplace_back(std::move(meta), dataset_tag);
        } else {
          LOG(WARNING) << "no tag found for dataset id: " << meta_info.id();
        }
      } else {
        LOG(WARNING) << "dataset id: " << meta_info.id() << " is not available";
      }
    }
    break;
  } while (true);

  if (datasets_with_tag.size() != meta_list.size()) {
    std::stringstream ss;
    ss << "Failed to get all dataset's meta, no handler triggered. "
       << "expected dataset size: " << datasets_with_tag.size() << ", "
       << "but get: " << meta_list.size() << " ";
    for (auto& meta : meta_list) {
      ss << "item: " << std::get<1>(meta) << " ";
    }
    RaiseException(ss.str());
  } else {
    handler(meta_list);
  }
  return retcode::SUCCESS;
}

retcode GrpcDatasetMetaService::GetAllMetas(std::vector<DatasetMeta>* metas_ptr) {
  rpc::GetDatasetRequest request;
  int retry_time{0};
  do {
    grpc::ClientContext context;
    rpc::GetDatasetResponse reply;
    grpc::Status status =
        stub_->GetDataset(&context, request, &reply);
    if (status.ok()) {
      if (reply.ret_code() != rpc::GetDatasetResponse::SUCCESS) {
        LOG(ERROR) << "get dataset meta info failed: " << reply.ret_msg();
        return retcode::FAIL;
      }
      VLOG(5) << "GetDataset from: ["
              <<  meta_service_.to_string() << "] rpc succeeded.";
    } else {
      LOG(WARNING) << "GetDataset from: ["
                << meta_service_.to_string() << "] rpc failed. "
                << status.error_code() << ": " << status.error_message()
                << "  rerty...";
      if (retry_time < retry_max_times_) {
        retry_time++;
        continue;
      } else {
        LOG(ERROR) << "GetDataset from: ["
                  << meta_service_.to_string() << "] rpc failed reaches max retry times: "
                  << retry_max_times_ << ", abort this operation";
        return retcode::FAIL;
      }
    }
    auto& metas = *metas_ptr;
    metas.clear();
    auto dataset_list = reply.data_set();
    for (const auto& dataset : dataset_list) {
      auto& meta_info = dataset.meta_info();
      if (dataset.available() == rpc::DatasetData::Available) {
        VLOG(5) << "data id: " << meta_info.id() << " "
          << "access_info: " << meta_info.access_info();
        DatasetMeta meta;
        PbMetaInfoToMeta(meta_info, &meta);
        metas.push_back(std::move(meta));
      } else {
        LOG(WARNING) << "dataset id: " << meta_info.id() << " is not available";
      }
    }
    break;
  } while (true);
  return retcode::SUCCESS;
}

retcode GrpcDatasetMetaService::MetaToPbMetaInfo(const DatasetMeta& meta,
                                                rpc::MetaInfo* pb_meta_info) {
  pb_meta_info->set_id(meta.id);
  pb_meta_info->set_driver(meta.getDriverType());
  pb_meta_info->set_access_info(meta.getAccessInfo());
  pb_meta_info->set_address(meta.getServerInfo());
  if (meta.Visibility() == DatasetVisbility::PUBLIC) {
    pb_meta_info->set_visibility(rpc::MetaInfo::PUBLIC);
  } else if (meta.Visibility() == DatasetVisbility::PRIVATE) {
    pb_meta_info->set_visibility(rpc::MetaInfo::PRIVATE);
  }

  auto schema = meta.getSchema();
  auto table_schema = dynamic_cast<TableSchema*>(schema.get());
  auto& arrow_scheam = table_schema->ArrowSchema();
  for (const auto& field : arrow_scheam->fields()) {
    auto item = pb_meta_info->add_data_type();
    item->set_name(field->name());
    int type = field->type()->id();
    item->set_type(static_cast<rpc::DataTypeInfo::PlainDataType>(type));
  }
  return retcode::SUCCESS;
}

retcode GrpcDatasetMetaService::PbMetaInfoToMeta(
    const rpc::MetaInfo& pb_meta_info,
    DatasetMeta* meta) {
  auto& meta_info = *meta;
  meta_info.setServerInfo(pb_meta_info.address());
  meta_info.SetAccessInfo(pb_meta_info.access_info());
  meta_info.SetVisibility(pb_meta_info.visibility());
  meta_info.SetDatasetId(pb_meta_info.id());
  meta_info.SetDriverType(pb_meta_info.driver());
  std::vector<std::tuple<std::string, int>> data_types;
  const auto& data_field_list = pb_meta_info.data_type();
  for (const auto& field : data_field_list) {
    std::string name = field.name();
    int type = field.type();
    VLOG(5) << "name: " << name << " type: " << field.type();
    data_types.emplace_back(std::make_tuple(name, type));
  }
  meta_info.SetDataSchema(data_types);
  return retcode::SUCCESS;
}

}  // namespace primihub::service
