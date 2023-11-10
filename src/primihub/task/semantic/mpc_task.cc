/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "src/primihub/task/semantic/mpc_task.h"
#include "src/primihub/algorithm/arithmetic.h"
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/algorithm/missing_val_processing.h"
#include "src/primihub/algorithm/mpc_statistics.h"
#include "src/primihub/common/value_check_util.h"

// #if defined(__linux__) && defined(__x86_64__)
// #include "src/primihub/algorithm/cryptflow2_maxpool.h"
// #include "src/primihub/algorithm/falcon_lenet.h"
// #endif

namespace primihub::task {
MPCTask::MPCTask(const std::string &node_id, const std::string &function_name,
                 const TaskParam *task_param,
                 std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service) {
  PartyConfig config(node_id, task_param_);
  if (function_name == "logistic_regression") {
    using LRExecutor = primihub::LogisticRegressionExecutor;
    algorithm_ = std::make_shared<LRExecutor>(config, dataset_service);
  } else if (function_name == "linear_regression") {
    // TODO(XXX): implement linear regression
  } else if (function_name == "mpc_statistics") {
    using StatisticsExecutor = primihub::MPCStatisticsExecutor;
    algorithm_ = std::make_shared<StatisticsExecutor>(config, dataset_service);
  } else if (function_name == "xgboost") {
    // TODO(XXX): implement xgboost
  } else if (function_name == "lightgbm") {
    // TODO(XXX): implement lightgbm
  } else if (function_name == "catboost") {
    // TODO(XXX): implement catboost
  } else if (function_name == "dnn") {
    // TODO(XXX): implement dnn
  } else if (function_name == "cnn") {
    // TODO(XXX): implement cnn
  } else if (function_name == "rnn") {
    // TODO(XXX): implement rnn
  } else if (function_name == "lstm") {
    // TODO(XXX): implement lstm
  } else if (function_name == "AbnormalProcessTask") {
    using MissingProcess = primihub::MissingProcess;
    algorithm_ = std::make_shared<MissingProcess>(config, dataset_service);
  } else if (function_name == "arithmetic") {
    std::string accuracy;
    const auto& param_map = task_param_.params().param_map();
    auto it = param_map.find("Accuracy");
    if (it != param_map.end()) {
      accuracy = it->second.value_string();
    }
    if (accuracy == "D32") {
      using D32Executor = primihub::ArithmeticExecutor<D32>;
      algorithm_ = std::make_shared<D32Executor>(config, dataset_service);
    } else {
      using D16Executor = primihub::ArithmeticExecutor<D16>;
      algorithm_ = std::make_shared<D16Executor>(config, dataset_service);
    }
  } else {
    LOG(ERROR) << "Unsupported algorithm: " << function_name;
  }

  if (algorithm_ != nullptr) {
    algorithm_->InitLinkContext(getTaskContext().getLinkContext().get());
  }
}

MPCTask::MPCTask(const std::string& function_name,
                 const TaskParam *task_param) :
                 TaskBase(task_param, nullptr) {   // do not read dataset,
                                                   // cause dataset service
                                                   // set to nullptr
  PartyConfig config("no use", *task_param);
  if (function_name == "mpc_statistics") {
    algorithm_ =
        std::make_shared<primihub::MPCStatisticsExecutor>(config, nullptr);
  } else if (function_name == "AbnormalProcessTask") {
    try {
      algorithm_ =
          std::make_shared<primihub::MissingProcess>(config, nullptr);
    } catch (const std::runtime_error &error) {
      LOG(ERROR) << error.what();
      algorithm_ = nullptr;
    }
  } else if (function_name == "arithmetic") {
    try {
      std::string accuracy;
      const auto& param_map = task_param_.params().param_map();
      auto it = param_map.find("Accuracy");
      if (it != param_map.end()) {
        accuracy = it->second.value_string();
      }
      if (accuracy == "D32") {
        using D32Executor = primihub::ArithmeticExecutor<D32>;
        algorithm_ = std::make_shared<D32Executor>(config, nullptr);
      } else {
        using D16Executor = primihub::ArithmeticExecutor<D16>;
        algorithm_ = std::make_shared<D16Executor>(config, nullptr);
      }
    } catch (const std::runtime_error &error) {
      LOG(ERROR) << error.what();
      algorithm_ = nullptr;
    }
  } else {
    LOG(ERROR) << "Unsupported algorithm: " << function_name;
  }
  if (algorithm_ != nullptr) {
    algorithm_->InitLinkContext(getTaskContext().getLinkContext().get());
  }
}

int MPCTask::execute() {
  if (algorithm_ == nullptr) {
    LOG(ERROR) << "Algorithm is not initialized";
    return -1;
  }
  if (RoleValidation::IsAuxiliaryCompute(this->party_name())) {
    int retcode{0};
    std::vector<int64_t> shape;
    RecvShapeFromLauncher(&shape);
    std::vector<double> input;
    std::vector<int64_t> col_rows;
    MakeAuxiliaryComputeData(shape, &input, &col_rows);
    std::vector<double> result;
    auto ret = ExecuteTask(input, col_rows, &result);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "run task as auxiliarty server failed";
      retcode = -1;
    }
    return retcode;
  } else {
    auto ret = ExecuteImpl();
    if (ret != retcode::SUCCESS) {
      return -1;
    }
    return 0;
  }
}

retcode MPCTask::ExecuteImpl() {
  int ret = -1;
  std::string error_msg;
  do {
    ret = algorithm_->loadParams(task_param_);
    BREAK_LOOP_BY_RETVAL(ret, "Load params failed.")

    ret = algorithm_->loadDataset();
    BREAK_LOOP_BY_RETVAL(ret, "Load dataset from file failed.")

    ret = algorithm_->initPartyComm();
    BREAK_LOOP_BY_RETVAL(ret, "Initialize party communicate failed.")

    auto retcode = algorithm_->InitEngine();
    BREAK_LOOP_BY_RETVAL(ret, "init engine failed")

    ret = algorithm_->execute();
    BREAK_LOOP_BY_RETVAL(ret, "Run task failed.")

    algorithm_->finishPartyComm();
    ret = algorithm_->saveModel();
    BREAK_LOOP_BY_RETVAL(ret, "saveModel failed.")
    ret = 0;
  } while (0);

  if (ret != 0) {
    return retcode::FAIL;
  } else {
    return retcode::SUCCESS;
  }
}

retcode MPCTask::ExecuteTask(const std::vector<double>& input_data,
                             const std::vector<int64_t>& rows_per_col,
                             std::vector<double>* result) {
  if (algorithm_ == nullptr) {
    LOG(ERROR) << "Algorithm is not initialized";
    return retcode::FAIL;
  }
  eMatrix<double> input_data_info;
  input_data_info.resize(input_data.size(), 2);
  for (size_t i = 0; i < input_data.size(); i++) {
    input_data_info(i, 0) = input_data[i];
    input_data_info(i, 1) = rows_per_col[i];
  }
  std::vector<std::string> col_names;
  for (size_t i = 0; i < input_data.size(); i++) {
    std::string col_name = "COL_" + std::to_string(i);
    col_names.push_back(col_name);
  }
  int ret;
  try {
    auto retcode = algorithm_->InitTaskConfig(task_param_);
    retcode = algorithm_->ExtractProxyNode(task_param_);
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "ExtractProxyNode from task config failed";
      return retcode::FAIL;
    }
    ret = algorithm_->initPartyComm();
    if (ret) {
      LOG(ERROR) << "Initialize party communicate failed.";
      return retcode::FAIL;
    }
    retcode = algorithm_->InitEngine();
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "init engine failed";
      return retcode::FAIL;
    }
    retcode = algorithm_->execute(input_data_info, col_names, result);
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "Run task failed.";
      return retcode::FAIL;
    }
    algorithm_->finishPartyComm();
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

std::shared_ptr<Dataset> MPCTask::MakeDataset(
    const std::vector<double>& input_data, int64_t colum_num) {
  std::vector<std::shared_ptr<arrow::Field>> field_vec_double;
  std::vector<std::shared_ptr<arrow::Array>> data_vec_double;
  for (int64_t i = 0; i < colum_num; i++) {
    std::string field_name{"COL_"};
    field_name.append(std::to_string(i));
    auto field_ptr = arrow::field(field_name, arrow::float64());
    field_vec_double.push_back(std::move(field_ptr));
  }
  for (int i = 0; i < input_data.size(); i++) {
    std::shared_ptr<arrow::Array> array;
    arrow::DoubleBuilder builder;
    builder.Append(input_data[i]);
    builder.Finish(&array);
    data_vec_double.push_back(std::move(array));
  }
  auto schema = std::make_shared<arrow::Schema>(field_vec_double);
  auto table = arrow::Table::Make(schema, data_vec_double);
  return std::make_shared<Dataset>(table, nullptr);
}

retcode MPCTask::RecvShapeFromLauncher(std::vector<int64_t>* shape) {
  auto& link_ctx = this->getTaskContext().getLinkContext();
  Node proxy_node;
  const auto& auxiliary_server = this->getTaskParam()->auxiliary_server();
  auto it = auxiliary_server.find(PROXY_NODE);
  if (it == auxiliary_server.end()) {
    LOG(ERROR) << "no proxy node found";
    return retcode::FAIL;
  }
  const auto& pb_proxy_node = it->second;
  pbNode2Node(pb_proxy_node, &proxy_node);
  std::string recv_buf;
  auto task_info = this->getTaskParam()->task_info();
  std::string shape_key = task_info.sub_task_id() + "_mpc_shape";
  link_ctx->Recv(shape_key, proxy_node, &recv_buf);
  rpc::ParamValue pb_shape;
  pb_shape.ParseFromString(recv_buf);
  auto& int64_arr = pb_shape.value_int64_array().value_int64_array();
  for (const auto item : int64_arr) {
    shape->push_back(item);
  }
  VLOG(7) << "end of RecvShapeFromLauncher";
  return retcode::SUCCESS;
}

retcode MPCTask::MakeAuxiliaryComputeData(const std::vector<int64_t>& shape,
                                          std::vector<double>* input,
                                          std::vector<int64_t>* col_rows) {
  if (shape.size() != 2) {
    LOG(ERROR) << "shape dim is not supported, 2 is expected,"
               << " but get: " << shape.size();
    return retcode::FAIL;
  }
  int64_t col_size = shape[1];
  col_rows->reserve(col_size);
  col_rows->assign(col_size, 0);

  input->reserve(col_size);
  auto task_config = this->getTaskParam();
  auto algorithm = task_config->algorithm();
  switch (algorithm.statistics_op_type()) {
    case rpc::Algorithm::MAX:
      input->assign(col_size, std::numeric_limits<double>::min());
      break;
    case rpc::Algorithm::MIN:
      input->assign(col_size, std::numeric_limits<double>::max());
      break;
    case rpc::Algorithm::AVG:
    case rpc::Algorithm::SUM:
      input->assign(col_size, 0);
      break;
    default:
      LOG(WARNING) << "unknown op type for statistics: "
                   << algorithm.statistics_op_type();
      return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::task
