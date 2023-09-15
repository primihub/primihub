// Copyright [2023] <PrimiHub>
#ifndef SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_INTERFACE_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_INTERFACE_H_
#include <vector>

#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/model.h"

namespace primihub::service {
class DatasetMetaService {
 public:
  DatasetMetaService() = default;
  explicit DatasetMetaService(size_t timeout) : meta_search_timeout_(timeout) {}
  virtual ~DatasetMetaService() = default;
  /**
   * save meta data
  */
  virtual retcode PutMeta(const DatasetMeta& meta) = 0;
  /**
   * get meta data by DatasetId
  */
  virtual retcode GetMeta(const DatasetId &id, FoundMetaHandler handler) = 0;
  virtual retcode FindPeerListFromDatasets(
      const std::vector<DatasetWithParamTag>& datasets_with_tag,
      FoundMetaListHandler handler) = 0;
  /**
   * get all dataset from meta server
  */
  virtual retcode GetAllMetas(std::vector<DatasetMeta>* metas) = 0;
  void SetMetaSearchTimeout(size_t timeout_seconds) {
    meta_search_timeout_ = timeout_seconds;
  }

 private:
  size_t meta_search_timeout_{60};  // seconds;
};
}  // namespace primihub::service
#endif  // SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_INTERFACE_H_
