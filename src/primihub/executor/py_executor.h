// Copyright [2023] <Primihub>

#ifndef SRC_PRIMIHUB_EXECUTOR_PY_EXECUTOR_H_
#define SRC_PRIMIHUB_EXECUTOR_PY_EXECUTOR_H_
#include <memory>
#include <map>

#include "pybind11/embed.h"
#include "src/primihub/common/common.h"
#include "src/primihub/protos/worker.pb.h"

namespace py = pybind11;
namespace primihub::task::python {
struct NodeContext {
    std::string role;
    std::string protocol;
    std::string next_peer;
    std::string task_type;
    std::vector<std::string> datasets;
    std::string dumps_func;
    std::map<std::string, std::string> dataset_port_map;
};

class PyExecutor {
 public:
    PyExecutor() = default;
    retcode init(const std::string& server_id, const std::string& server_config_file,
            const std::string& request);
    retcode execute();

 protected:
    retcode parseParams();
    retcode run_task_code();
    std::string node_id() const {
        return this->node_id_;
    }

 private:
    std::unique_ptr<primihub::rpc::PushTaskRequest> task_request_{nullptr};
    std::string node_id_;
    std::string server_config_file_;
    py::object ph_exec_m_, ph_context_m_;
    NodeContext node_context_;
    std::map<std::string, std::string> dataset_meta_map_;

    // Key is the combine of node's nodeid and role,
    // and value is 'ip:port'.
    std::map<std::string, std::string> node_addr_map_;

    // Save all params.
    std::map<std::string, std::string> params_map_;
    std::string taskid_;
    std::string jobid_;
    std::string request_id_;
};

}  // namespace primihub::task::python

#endif  // SRC_PRIMIHUB_EXECUTOR_PY_EXECUTOR_H_
