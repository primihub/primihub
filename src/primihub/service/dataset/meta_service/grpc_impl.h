// Copyright [2023] <PrimiHub>

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_GRPC_IMPL_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_GRPC_IMPL_H_
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <vector>
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/meta_service/interface.h"
#include "src/primihub/protos/service.grpc.pb.h"

namespace primihub::service {
class GrpcDatasetMetaService : public DatasetMetaService {
 public:
  GrpcDatasetMetaService() = default;
  explicit GrpcDatasetMetaService(const Node& meta_service_info);
  retcode PutMeta(const DatasetMeta& meta) override;
  retcode GetMeta(const DatasetId& id, FoundMetaHandler handler) override;
  retcode FindPeerListFromDatasets(
      const std::vector<DatasetWithParamTag>& datasets_with_tag,
      FoundMetaListHandler handler) override;
  retcode GetAllMetas(std::vector<DatasetMeta>* metas) override;

 protected:
  retcode MetaToPbMetaInfo(const DatasetMeta& meta, rpc::MetaInfo* pb_meta_info);
  retcode PbMetaInfoToMeta(const rpc::MetaInfo& pb_meta_info, DatasetMeta* meta);

 private:
  std::unique_ptr<rpc::DataSetService::Stub> stub_{nullptr};
  std::shared_ptr<grpc::Channel> grpc_channel_{nullptr};
  Node meta_service_;
  int retry_max_times_{GRPC_RETRY_MAX_TIMES};
};
}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_META_SERVICE_GRPC_IMPL_H_
