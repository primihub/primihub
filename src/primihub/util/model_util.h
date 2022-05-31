// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_MODEL_UTIL_H_
#define SRC_primihub_UTIL_MODEL_UTIL_H_

#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <array>

#include "Eigen/Dense"
#include "src/primihub/common/type/type.h"
#include "src/primihub/common/defines.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/eigen_util.h"

namespace primihub {

void Linear_sample(eMatrix<double>& X, eMatrix<double>& Y, eMatrix<double>& mModel, double mNoise = 1,
    double mSd = 1, bool print = false);

void Logistic_sample(eMatrix<double>& X, eMatrix<double>& Y, eMatrix<double>& mModel, double mNoise = 1,
    double mSd = 1);
}

#endif
