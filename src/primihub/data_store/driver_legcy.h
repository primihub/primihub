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

#ifndef SRC_PRIMIHUB_DATASTORE_DRIVER_LEGCY_H_
#define SRC_PRIMIHUB_DATASTORE_DRIVER_LEGCY_H_


#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#include <memory>

// #include "src/primihub/common/clp.h"
#include "src/primihub/common/type/type.h"

namespace primihub
{
    eMatrix<double> load_data_local_logistic(const std::string &fullpath);

} // namespace primihub

#endif // SRC_PRIMIHUB_DATASTORE_DRIVER_LEGCY_H_
