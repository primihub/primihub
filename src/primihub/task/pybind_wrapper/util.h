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
#ifndef SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_UTIL_H_
#define SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_UTIL_H_
#include <string>
#include "src/primihub/common/common.h"
namespace primihub::task::wrapper {
retcode GenerateSubtaskId(std::string* sub_task_id);
}
#endif  // SRC_PRIMIHUB_TASK_PYBIND_WRAPPER_UTIL_H_
