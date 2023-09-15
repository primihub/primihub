// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_ALGORITHM_LINEAR_MODEL_GEN_H_
#define SRC_PRIMIHUB_ALGORITHM_LINEAR_MODEL_GEN_H_

#include <fstream>
#include <random>
#include <vector>
#include <iostream>

#include "Eigen/Dense"
#include "cryptoTools/Common/Defines.h"
#include "aby3/sh3/Sh3Types.h"

namespace primihub {

class LinearModelGen {
 public:
  aby3::eMatrix<double> mModel;
  double mNoise, mSd;
  void setModel(aby3::eMatrix<double>& model, double noise = 1, double sd = 1);
};

class LogisticModelGen {
 public:
  aby3::eMatrix<double> mModel;
  double mNoise, mSd;
  void setModel(aby3::eMatrix<double>& model, double noise = 1, double sd = 1);
};
aby3::eMatrix<double> LoadDataLocalLogistic(const std::string& full_path);

}  // namespace primihub

#endif  // SRC_PRIMIHUB_ALGORITHM_LINEAR_MODEL_GEN_H_
