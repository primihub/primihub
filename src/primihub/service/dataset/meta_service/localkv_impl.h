/**
 * Copyright [2023] <PrimiHub>
*/

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_LOCALKV_IMPL_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_LOCALKV_IMPL_H_
#include <vector>
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/meta_service/interface.h"
#include "src/primihub/service/dataset/meta_service/localkv/storage_backend.h"

namespace primihub::service {
class LocalDatasetMetaService : public DatasetMetaService {
 public:
  LocalDatasetMetaService() = default;
  retcode PutMeta(const DatasetMeta& meta) override;
  retcode GetMeta(const DatasetId &id, FoundMetaHandler handler) override;
  retcode FindPeerListFromDatasets(
      const std::vector<DatasetWithParamTag>& datasets_with_tag,
      FoundMetaListHandler handler) override;
  retcode GetAllMetas(std::vector<DatasetMeta>* metas) override;

 private:
  std::unique_ptr<StorageBackend> storage_{nullptr};
};
}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_LOCALKV_IMPL_H_
