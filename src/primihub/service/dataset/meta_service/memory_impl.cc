/**
 * Copyright [2023] <PrimiHub>
*/
#include "src/primihub/service/dataset/meta_service/memory_impl.h"
#include "src/primihub/service/dataset/meta_service/localkv/storage_default.h"

namespace primihub::service {
MemoryDatasetMetaService::MemoryDatasetMetaService() {
  storage_ = std::make_unique<StorageBackendDefault>();
}

retcode MemoryDatasetMetaService::PutMeta(const DatasetMeta& meta) {
  std::string dataset_id = meta.id;
  std::string meta_str = meta.toJSON();
  // LOG(ERROR) << "MemoryDatasetMetaService::PutMeta: " << meta_str;
  this->storage_->PutValue(dataset_id, meta_str);
  return retcode::SUCCESS;
}

retcode MemoryDatasetMetaService::GetMeta(const DatasetId &id, FoundMetaHandler handler) {
  auto meta = std::make_shared<DatasetMeta>();
  std::string meta_str;
  this->storage_->GetValue(id, &meta_str);
  meta->fromJSON(meta_str);
  auto ret = handler(meta);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "handle meta info encountes error";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode MemoryDatasetMetaService::FindPeerListFromDatasets(
    const std::vector<DatasetWithParamTag>& datasets_with_tag,
    FoundMetaListHandler handler) {
//
  return retcode::SUCCESS;
}

retcode MemoryDatasetMetaService::GetAllMetas(std::vector<DatasetMeta>* metas) {
  return retcode::SUCCESS;
}

}  // namespace primihub::service