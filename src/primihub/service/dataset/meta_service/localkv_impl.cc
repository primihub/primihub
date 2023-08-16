/**
 * Copyright [2023] <PrimiHub>
*/
#include "src/primihub/service/dataset/meta_service/localkv_impl.h"

namespace primihub::service {
retcode LocalDatasetMetaService::PutMeta(const DatasetMeta& meta) {
  return retcode::SUCCESS;
}

retcode LocalDatasetMetaService::GetMeta(const DatasetId &id, FoundMetaHandler handler) {
  return retcode::SUCCESS;
}

retcode LocalDatasetMetaService::FindPeerListFromDatasets(
    const std::vector<DatasetWithParamTag>& datasets_with_tag,
    FoundMetaListHandler handler) {
//
  return retcode::SUCCESS;
}

retcode LocalDatasetMetaService::GetAllMetas(std::vector<DatasetMeta>* metas) {
  return retcode::SUCCESS;
}

}  // namespace primihub::service