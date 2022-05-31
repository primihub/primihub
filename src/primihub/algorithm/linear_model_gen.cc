// Copyright [2021] <primihub.com>
#include "src/primihub/algorithm/linear_model_gen.h"

namespace primihub {
void LinearModelGen::setModel(eMatrix<double>& model,
  double noise, double sd) {
  mModel = model;
  mNoise = noise;
  mSd = sd;
}

void LogisticModelGen::setModel(eMatrix<double>& model,
  double noise, double sd) {
  mModel = model;
  mNoise = noise;
  mSd = sd;
}

}  // namespace primihub
