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

namespace primihub {
  int logistic_plain_main();
  int logistic_2plain_main(std::string &filename);
}

#endif  // SRC_PRIMIHUB_ALGORITHM_LOGISTIC_PLAIN_H_
