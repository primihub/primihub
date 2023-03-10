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
    std::string pb_request_str = base64_decode(request);
    task_request_ = std::make_unique<rpc::PushTaskRequest>();
    bool status = task_request_->ParseFromString(pb_request_str);
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
    ret = run_task_code();
    return ret;
}

retcode PyExecutor::parseParams() {
    const auto& pb_task = task_request_->task();
    // Convert TaskParam to NodeContext
    const auto& param_map = pb_task.params().param_map();

    const auto& node_map_ref = pb_task.node_map();
    std::string server_ip_str;
    for (auto iter = node_map_ref.begin(); iter != node_map_ref.end(); iter++) {
        if (!server_ip_str.empty()) {
            break;
        }
        auto& vm_list = iter->second.vm();
        for (auto& vm : vm_list) {
            if (vm.next().link_type() == primihub::rpc::LinkType::SERVER) {
                server_ip_str = iter->second.ip();
                break;
            }
            VLOG(1) << "link type: " << vm.next().link_type() << "\n";
        }
        VLOG(1) << " --- iter first node ip: " << iter->second.ip()
                  << ", port: " << iter->second.port() << " \n";
    }
    std::string key = "role";
    auto it = param_map.find(key);
    if (it != param_map.end()) {
        this->node_context_.role = it->second.value_string();
    } else {
        LOG(ERROR) << "Failed to load params: " << key;
        return retcode::FAIL;
    }
    key = "protocol";
    it = param_map.find(key);
    if (it != param_map.end()) {
        this->node_context_.protocol = it->second.value_string();
    } else {
        LOG(ERROR) << "Failed to load params: " << key;
        return retcode::FAIL;
    }
    this->node_context_.dumps_func = pb_task.code();
    auto input_datasets = pb_task.input_datasets();
    for (auto& input_dataset : input_datasets) {
        VLOG(1) << "input_dataset: " << input_dataset << "\n";
        this->node_context_.datasets.push_back(input_dataset);
    }

    // get next peer address
    auto node_it = pb_task.node_map().find(node_id());
    if (node_it == pb_task.node_map().end()) {
        LOG(ERROR) << "Failed to find node: " << node_id();
        return retcode::FAIL;
    }

    auto vm_list = node_it->second.vm();
    if (vm_list.empty()) {
        LOG(ERROR) << "Failed to find vm list for node: " << node_id();
        return retcode::FAIL;
    }

    for (auto& vm : vm_list) {
        std::string full_addr =
            vm.next().ip() + ":" + std::to_string(vm.next().port());

        // Key is the combine of node's nodeid and role,
        // and value is 'ip:port'.
        node_addr_map_[vm.next().name()] = full_addr;
    }

    {
        uint32_t count = 0;
        auto iter = node_addr_map_.begin();

        VLOG(1)
            << "Dump node id with role and it's address used by FL algorithm.";

        for (iter; iter != node_addr_map_.end(); iter++) {
            VLOG(1) << "Node " << iter->first << ": [" << iter->second << "].";
            count++;
        }

        VLOG(1) << "Dump finish, dump count " << count << ".";
    }

    // Set datasets meta list in context
    for (auto& input_dataset : input_datasets) {
        // Get dataset path from task params map
        auto data_meta = param_map.find(input_dataset);
        if (data_meta == param_map.end()) {
            LOG(ERROR) << "Failed to find dataset: " << input_dataset;
            return retcode::FAIL;;
        }
        std::string data_path = data_meta->second.value_string();
        this->dataset_meta_map_.insert(
            std::make_pair(input_dataset, data_path));
    }


    for (auto &pair : param_map)
        this->params_map_[pair.first] = pair.second.value_string();

    std::string node_key = node_id() + "_dataset";
    auto iter = param_map.find(node_key);
    if (iter == param_map.end()) {
        LOG(ERROR) << "Can't find dataset name of node " << node_id() << ".";
        return retcode::FAIL;
    }

    this->params_map_["local_dataset"] = iter->second.value_string();
    jobid_ = pb_task.task_info().job_id();
    taskid_ = pb_task.task_info().task_id();
    request_id_ = pb_task.task_info().request_id();
    VLOG(1) << "end of parseParams, params_map_ size: " << this->params_map_.size();
    return retcode::SUCCESS;
}

retcode PyExecutor::run_task_code() {
    py::gil_scoped_acquire acquire;
    try {
        ph_exec_m_ = py::module::import("primihub.executor").attr("Executor");
        ph_context_m_ = py::module::import("primihub.context");
        // Run set_node_context method.
        py::object set_node_context;
        set_node_context = ph_context_m_.attr("set_node_context");

        set_node_context(node_context_.role, node_context_.protocol,
                          py::cast(node_context_.datasets));

        set_node_context.release();
        // Run set_task_context_dataset_map method.
        py::object set_task_context_dataset_map;
        set_task_context_dataset_map =
            ph_context_m_.attr("set_task_context_dataset_map");

        for (auto& dataset_meta : this->dataset_meta_map_) {
            VLOG(1) << "Insert DATASET : " << dataset_meta.first
                      << ", " << dataset_meta.second;
            set_task_context_dataset_map(dataset_meta.first,
                                         dataset_meta.second);
        }

        set_task_context_dataset_map.release();

        // Run set_task_context_params_map.
        py::object set_task_context_params_map;
        set_task_context_params_map =
            ph_context_m_.attr("set_task_context_params_map");

        for (auto &pair : this->params_map_) {
            set_task_context_params_map(pair.first, pair.second);
        }

        {
            // std::string nodelet_addr =
            // this->dataset_service_->getNodeletAddr();
            // auto pos = nodelet_addr.find(":");

            // set_task_context_params_map("DatasetServiceAddr",
            //     nodelet_addr.substr(pos + 1, nodelet_addr.length()));
            const auto& node_map = task_request_->task().node_map();
            for (const auto& node_info : node_map) {
              if (node_info.first == this->node_id_) {
                  auto& node_access = node_info.second;
                  std::string node_access_info = node_access.ip();
                  node_access_info.append(":").append(std::to_string(node_access.port()))
                  .append(":").append(node_access.use_tls() ? "1" : "0");
                  set_task_context_params_map("DatasetServiceAddr", node_access_info);
              }
            }
            // Setup task id and job id.
            set_task_context_params_map("jobid", jobid_);
            set_task_context_params_map("taskid", taskid_);
            set_task_context_params_map("request_id", request_id_);
            set_task_context_params_map("config_file_path", server_config_file_);
        }

        set_task_context_params_map.release();
        // Run set_task_context_node_addr_map.
        py::object set_node_addr_map;
        set_node_addr_map = ph_context_m_.attr("set_task_context_node_addr_map");
        for (auto &pair : this->node_addr_map_) {
            set_node_addr_map(pair.first, pair.second);
        }

        set_node_addr_map.release();
    } catch (std::exception& e) {
        LOG(ERROR) << "Failed: " << e.what();
        return retcode::FAIL;
    }

    auto taskId = this->task_request_->task().task_info().task_id();
    auto job_id = this->task_request_->task().task_info().task_id();
    try {
        VLOG(1) << "<<<<<<<<< Start executing Python code <<<<<<<<<" << std::endl;
        // Execute python code.
        ph_exec_m_.attr("execute_py")(py::bytes(node_context_.dumps_func));
        VLOG(1) << "<<<<<<<<< Execute Python Code End <<<<<<<<<" << std::endl;

    } catch (std::exception &e) {
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