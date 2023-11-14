// Copyright [2023] <PrimiHub>
#ifndef SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_FACTORY_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_FACTORY_H_
#include <glog/logging.h>
#include <string>
#include <algorithm>
#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/meta_service/interface.h"
#include "src/primihub/service/dataset/meta_service/grpc_impl.h"
#include "src/primihub/service/dataset/meta_service/memory_impl.h"
#include "src/primihub/service/dataset/meta_service/localkv_impl.h"
#include "src/primihub/util/util.h"

namespace primihub::service {
struct MetaServiceMode {
  static constexpr const char* MODE_GRPC = "GRPC";
  static constexpr const char* MODE_MEMORY = "MEMORY";
  static constexpr const char* MODE_LOCAL_KV = "LOCAL_KV";
};

class MetaServiceFactory {
 public:
  template<typename... Args>
  static std::unique_ptr<DatasetMetaService>
  Create(const std::string& mode, Args&&... args) {
    std::unique_ptr<DatasetMetaService> meta_service{nullptr};
    // to upper
    std::string upper_mode = primihub::strToUpper(mode);
    if (upper_mode == MetaServiceMode::MODE_GRPC) {
      meta_service = CreateGRPCMetaService(std::forward<Args>(args)...);
    } else if (upper_mode == MetaServiceMode::MODE_MEMORY) {
      meta_service = CreateMemoryetaService();
    } else if (upper_mode == MetaServiceMode::MODE_LOCAL_KV) {
      meta_service = CreateLocalKVMetaService();
    } else {
      LOG(ERROR) << "unsupported mode: " << upper_mode;
    }
    return meta_service;
  }

  static std::unique_ptr<DatasetMetaService>
  CreateGRPCMetaService(const Node& meta_service_cfg) {
    return std::make_unique<GrpcDatasetMetaService>(meta_service_cfg);
  }

  static std::unique_ptr<DatasetMetaService> CreateMemoryetaService() {
    return std::make_unique<MemoryDatasetMetaService>();
  }

  static std::unique_ptr<DatasetMetaService> CreateLocalKVMetaService() {
    return std::make_unique<LocalDatasetMetaService>();
  }

};
}  // namespace primihub::service
#endif  // SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_FACTORY_H_