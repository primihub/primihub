/*
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef SRC_PRIMIHUB_UTIL_LOG_H_
#define SRC_PRIMIHUB_UTIL_LOG_H_
#include "glog/logging.h"
#include <string>

namespace primihub {
enum class LogType {
  kScheduler,
  kTask,
  kDataService
};

static std::string LogTypeToString(LogType type) {
  switch (type) {
    case LogType::kScheduler:
      return "SCHEDULER";
    case LogType::kTask:
      return "TASK";
    case LogType::kDataService:
      return "DATA_SERVICE";
    default:
      return "";
  }
}

#define PH_LOG(log_level, log_type) \
    LOG(log_level)
    // LOG(log_level) << LogTypeToString(log_type) << " "

#define PH_VLOG(log_level, log_type) \
    VLOG(log_level)

    // VLOG(log_level) << LogTypeToString(log_type) << " "

#define PH_LOG_EVERY_N(log_level, n, log_type) \
    LOG_EVERY_N(log_level, n) << LogTypeToString(log_type) << " "

}  // namespace primihub

#endif  // SRC_PRIMIHUB_UTIL_LOG_H_
