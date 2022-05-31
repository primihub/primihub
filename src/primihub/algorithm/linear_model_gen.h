// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_ALGORITHM_LINEAR_MODEL_GEN_H_
#define SRC_primihub_ALGORITHM_LINEAR_MODEL_GEN_H_

#include <fstream>
#include <random>
#include <vector>
#include <iostream>

#include "Eigen/Dense"

#include "src/primihub/common/type/type.h"
#include "src/primihub/common/defines.h"

namespace primihub {
class LinearModelGen {
 public:
  eMatrix<double> mModel;
  double mNoise, mSd;

  void setModel(eMatrix<double>& model, double noise = 1,
    double sd = 1);
};

class LogisticModelGen {
 public:
  eMatrix<double> mModel;
  double mNoise, mSd;

  void setModel(eMatrix<double>& model, double noise = 1,
    double sd = 1);

};

}  // namespace primihub

#endif  // SRC_primihub_ALGORITHM_LINEAR_MODEL_GEN_H_
