//
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_COMMON_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_COMMON_H_
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/protos/common.pb.h"
using primihub::rpc::Task;
using primihub::rpc::Language;
using primihub::service::DatasetService;
using primihub::service::DatasetWithParamTag;
using primihub::service::DatasetMetaWithParamTag;

namespace primihub::task {
struct NodeContext {
    std::string role;
    std::string protocol;
    std::string next_peer;
    std::string task_type;
    std::vector<std::string> datasets;
    std::string dumps_func;
    std::map<std::string, std::string> dataset_port_map;
};
using PeerDatasetMap = std::map<std::string, std::vector<DatasetWithParamTag>>;
using NodeWithRoleTag = std::pair<rpc::Node, std::string>;
using PeerContextMap = std::map<std::string, NodeContext>; // key: role name
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_COMMON_H_
