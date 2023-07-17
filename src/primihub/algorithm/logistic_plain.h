// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_ALGORITHM_LOGISTIC_PLAIN_H_
#define SRC_PRIMIHUB_ALGORITHM_LOGISTIC_PLAIN_H_
#include <time.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <exception>



// #include "src/primihub/common/common.h"
// #include "src/primihub/common/clp.h"
// #include "src/primihub/common/type/type.h"
// // #include "src/primihub/data_service/dataload.h"
// #include "src/primihub/algorithm/regression.h"
// #include "src/primihub/algorithm/linear_model_gen.h"
// #include "src/primihub/algorithm/aby3ML.h"
// #include "src/primihub/algorithm/plainML.h"
// #include "src/primihub/util/network/socket/ioservice.h"
// #include "src/primihub/util/network/socket/channel.h"
// #include "src/primihub/data_store/driver_legcy.h"

namespace primihub {
  int logistic_plain_main();
  int logistic_2plain_main(std::string &filename);
}

#endif  // SRC_PRIMIHUB_ALGORITHM_LOGISTIC_PLAIN_H_
