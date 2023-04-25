// Copyright [2023] <Primihub>
#include "src/primihub/executor/py_executor.h"
#include <glog/logging.h>

#include <iostream>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"

#include "src/primihub/protos/worker.pb.h"
#include "base64.h"
#include <pybind11/stl.h>
#include "Python.h"

ABSL_FLAG(std::string, node_id, "node0", "unique node_id");
ABSL_FLAG(std::string, config_file, "data/node.yaml", "server config file");
ABSL_FLAG(std::string, request, "", "task request, serialized by rpc::Task");

namespace primihub::task::python {
retcode PyExecutor::init(const std::string& node_id, const std::string& server_config_file,
        const std::string& request) {
    this->node_id_ = node_id;
    this->server_config_file_ = server_config_file;
    pb_task_request_ = base64_decode(request);
    task_request_ = std::make_unique<rpc::PushTaskRequest>();
    bool status = task_request_->ParseFromString(pb_task_request_);
    if (!status) {
        LOG(ERROR) << "parse task request error";
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
}

retcode PyExecutor::execute() {
    auto ret = parseParams();
    if (ret != retcode::SUCCESS) {
        return retcode::FAIL;
    }
    return ExecuteTaskCode();
}

retcode PyExecutor::parseParams() {
  return retcode::SUCCESS;
}

retcode PyExecutor::ExecuteTaskCode() {
  py::gil_scoped_acquire acquire;
  try {
    ph_exec_m_ = py::module::import("primihub.executor").attr("Executor");
    ph_context_m_ = py::module::import("primihub.context");
    py::object set_message;
    set_message = ph_context_m_.attr("set_message");
    set_message(py::bytes(pb_task_request_));
    set_message.release();
    VLOG(1) << "<<<<<<<<< Start executing Python code <<<<<<<<<" << std::endl;
    // Execute python code.
    ph_exec_m_.attr("execute_py")();
    VLOG(1) << "<<<<<<<<< Execute Python Code End <<<<<<<<<" << std::endl;
  } catch (std::exception& e) {
    LOG(ERROR) << "Failed to execute python: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::task::python

int main(int argc, char **argv) {
    py::scoped_interpreter python;
    py::gil_scoped_release release;
    google::InitGoogleLogging(argv[0]);
    FLAGS_colorlogtostderr = false;
    FLAGS_alsologtostderr = false;
    VLOG(1) << "";
    VLOG(1) << "start py main";
    absl::ParseCommandLine(argc, argv);
    std::string node_id = absl::GetFlag(FLAGS_node_id);
    std::string config_file = absl::GetFlag(FLAGS_config_file);
    std::string task_request_str = absl::GetFlag(FLAGS_request);
    if (task_request_str.empty()) {
        LOG(ERROR) << "task request empty is not allowed";
        return -1;
    }
    auto py_executor = std::make_unique<primihub::task::python::PyExecutor>();
    auto ret = py_executor->init(node_id, config_file, task_request_str);
    if (ret != primihub::retcode::SUCCESS) {
        LOG(ERROR) << "init py executor failed";
        return -1;
    }
    ret = py_executor->execute();
    if (ret != primihub::retcode::SUCCESS) {
        LOG(ERROR) << "py executor encoutes error when executing task";
        return -1;
    }
    return 0;
}