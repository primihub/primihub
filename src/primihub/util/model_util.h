// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_MODEL_UTIL_H_
#define SRC_primihub_UTIL_MODEL_UTIL_H_

#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <array>

#include "src/primihub/common/type.h"

namespace primihub {

void Linear_sample(eMatrix<double>& X, eMatrix<double>& Y,
                   eMatrix<double>& mModel, double mNoise = 1,
                   double mSd = 1, bool print = false);

void Logistic_sample(eMatrix<double>& X, eMatrix<double>& Y,
                     eMatrix<double>& mModel, double mNoise = 1,
                     double mSd = 1);
}

#endif
