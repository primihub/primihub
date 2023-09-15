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
    try {
      algorithm_ = std::make_shared<primihub::LogisticRegressionExecutor>(config, dataset_service);
    } catch (std::exception &e) {
      LOG(ERROR) << e.what();
      algorithm_ = nullptr;
    }
  } else if (function_name == "linear_regression") {
    // TODO(XXX): implement linear regression
  } else if (function_name == "mpc_statistics") {
    algorithm_ =
        std::make_shared<primihub::MPCStatisticsExecutor>(config, dataset_service);
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
  } else if (function_name == "arithmetic") {
    try {
      auto param_map = task_param_.params().param_map();
      std::string accuracy = param_map["Accuracy"].value_string();

      if (accuracy == "D32") {
        std::shared_ptr<ArithmeticExecutor<D32>> executor;
        algorithm_ = std::make_shared<ArithmeticExecutor<D32>>(config, dataset_service);
      } else {
        std::shared_ptr<ArithmeticExecutor<D16>> executor;
        algorithm_ = std::make_shared<ArithmeticExecutor<D16>>(config, dataset_service);
      }
    } catch (const std::runtime_error &error) {
      LOG(ERROR) << error.what();
      algorithm_ = nullptr;
    }
  } else if (function_name == "AbnormalProcessTask") {
    try {
      algorithm_ =
          std::make_shared<primihub::MissingProcess>(config, dataset_service);
    } catch (const std::runtime_error &error) {
      LOG(ERROR) << error.what();
      algorithm_ = nullptr;
    }
  } else if (function_name == "arithmetic") {
    try {
      auto param_map = task_param_.params().param_map();
      std::string accuracy = param_map["Accuracy"].value_string();
      if (accuracy == "D32") {
        algorithm_ = std::dynamic_pointer_cast<AlgorithmBase>(
            std::make_shared<primihub::ArithmeticExecutor<D32>>(config,
                                                      dataset_service));
      } else {
        algorithm_ = std::dynamic_pointer_cast<AlgorithmBase>(
            std::make_shared<primihub::ArithmeticExecutor<D16>>(config,
                                                      dataset_service));
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

  int ret = 0;
  try {
    do
    {
      ret = algorithm_->loadParams(task_param_);
      if (ret) {
        LOG(ERROR) << "Load params failed.";
        break;
      }

      ret = algorithm_->loadDataset();
      if (ret) {
        LOG(ERROR) << "Load dataset from file failed.";
        break;
      }

      ret = algorithm_->initPartyComm();
      if (ret) {
        LOG(ERROR) << "Initialize party communicate failed.";
        break;
      }
      auto retcode = algorithm_->InitEngine();
      if (retcode != retcode::SUCCESS) {
        LOG(ERROR) << "init engine failed";
        ret = -1;
        break;
      }

      ret = algorithm_->execute();
      if (ret) {
        LOG(ERROR) << "Run task failed.";
        break;
      }

      algorithm_->finishPartyComm();
      ret = algorithm_->saveModel();
      if (ret != 0) {
        LOG(ERROR) << "saveModel failed.";
        break;
      }
    } while (0);
  } catch (std::exception &e) {
    LOG(ERROR) << e.what();
    return -2;
  }

  return ret;
}

}  // namespace primihub::task
