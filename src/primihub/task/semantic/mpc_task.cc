/*
 Copyright 2022 Primihub

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
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/util/network/socket/session.h"

#ifndef __APPLE__
#include "src/primihub/algorithm/falcon_lenet.h"
#include "src/primihub/algorithm/cryptflow2_maxpool.h"
#endif

namespace primihub::task
{
  MPCTask::MPCTask(const std::string &node_id, const std::string &function_name,
                   const TaskParam *task_param,
                   std::shared_ptr<DatasetService> dataset_service)
      : TaskBase(task_param, dataset_service)
  {
    if (function_name == "logistic_regression")
    {
      PartyConfig config(node_id, task_param_);
      algorithm_ = std::dynamic_pointer_cast<AlgorithmBase>(
          std::make_shared<primihub::LogisticRegressionExecutor>(
              config, dataset_service));
    }
    else if (function_name == "linear_regression")
    {
      // TODO: implement linear regression
    }
    else if (function_name == "maxpool")
    {
#ifndef __APPLE__
      PartyConfig config(node_id, task_param_);
      try
      {
        algorithm_ = std::dynamic_pointer_cast<AlgorithmBase>(
            std::make_shared<primihub::cryptflow2::MaxPoolExecutor>(
		    config, dataset_service));
      }
      catch (const std::runtime_error &error)
      {
        LOG(ERROR) << error.what();
        algorithm_ = nullptr;
      }
#else
      LOG(WARNING) << "Skip init maxpool algorithm instance due to lack support for apple platform.";
#endif
    }
    else if (function_name == "lenet")
    {
#ifndef __APPLE__
      PartyConfig config(node_id, task_param_);
      algorithm_ = std::dynamic_pointer_cast<AlgorithmBase>(
          std::make_shared<primihub::falcon::FalconLenetExecutor>(
		  config, dataset_service));
#else
      LOG(WARNING) << "Skip init lenet algorithm instance due to lack support for apple platform.";
#endif
    }
    else if (function_name == "decision_tree")
    {
      // TODO: implement decision tree
    }
    else if (function_name == "random_forest")
    {
      // TODO: implement random forest
    }
    else if (function_name == "xgboost")
    {
      // TODO: implement xgboost
    }
    else if (function_name == "lightgbm")
    {
      // TODO: implement lightgbm
    }
    else if (function_name == "catboost")
    {
      // TODO: implement catboost
    }
    else if (function_name == "dnn")
    {
      // TODO: implement dnn
    }
    else if (function_name == "cnn")
    {
      // TODO: implement cnn
    }
    else if (function_name == "rnn")
    {
      // TODO: implement rnn
    }
    else if (function_name == "lstm")
    {
      // TODO: implement lstm
    }
    else
    {
      LOG(ERROR) << "Unsupported algorithm: " << function_name;
    }
  }

  int MPCTask::execute()
  {
    if (algorithm_ == nullptr)
    {
      LOG(ERROR) << "Algorithm is not initialized";
      return -1;
    }

    algorithm_->loadParams(task_param_);
    int ret = 0;
    do
    {
      ret = algorithm_->loadDataset();
      if (ret)
      {
        LOG(ERROR) << "Load dataset from file failed.";
        break;
      }

      ret = algorithm_->initPartyComm();
      if (ret)
      {
        LOG(ERROR) << "Initialize party communicate failed.";
        break;
      }

      ret = algorithm_->execute();
      if (ret)
      {
        LOG(ERROR) << "Run train failed.";
        break;
      }

      algorithm_->finishPartyComm();
      algorithm_->saveModel();
    } while (0);

    return ret;
  }

} // namespace primihub::task
