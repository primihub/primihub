// Copyright [2021] <primihub.com>
#include "src/primihub/util/model_util.h"

#include <iostream>
#include <random>

#include "cryptoTools/Common/Defines.h"

#include "src/primihub/util/eigen_util.h"

namespace primihub {

void Linear_sample(eMatrix<double>& X, eMatrix<double>& Y,
                   eMatrix<double>& mModel, double mNoise = 1, double mSd = 1) {
  if (X.rows() != Y.rows()) {
    throw std::runtime_error(LOCATION);
  }
  if (1 != Y.cols()) {
    throw std::runtime_error(LOCATION);
  }
  if (X.cols() != mModel.rows()) {
    throw std::runtime_error(LOCATION);
  }

  std::default_random_engine generator;
  std::normal_distribution<double> distribution(mNoise, mSd);

  eMatrix<double> noise(X.rows(), 1);

  for (int i = 0; i < X.rows(); ++i) {
      for (int j = 0; j < X.cols(); ++j) {
          X(i, j) = distribution(generator);
      }

      noise(i, 0) = distribution(generator);
  }

  Y = X * mModel + noise;
}

void Logistic_sample(eMatrix<double>& X, eMatrix<double>& Y,
                     eMatrix<double>& mModel, double mNoise = 1,
                     double mSd = 1, bool print = false) {
  if (X.rows() != Y.rows()) {
      throw std::runtime_error(LOCATION);
  }
  if (1 != Y.cols()) {
      throw std::runtime_error(LOCATION);
  }
  if (X.cols() != mModel.rows()) {
      throw std::runtime_error(LOCATION);
  }
  std::default_random_engine generator(time(NULL));
  std::normal_distribution<double> distribution(mNoise, mSd);

  eMatrix<double> noise(X.rows(), 1);
  for (int i = 0; i < X.rows(); ++i) {
      for (int j = 0; j < X.cols(); ++j) {
          X(i, j) = distribution(generator);
      }

    noise(i, 0) = distribution(generator);
  }

  Y = X * mModel + noise;

  for (int i = 0; i < Y.size(); ++i) {
      if (print) {
          std::cout << X.row(i).format(CSVFormat);
          std::cout << " -> " << (Y(i) > 0) << Y.row(i).format(CSVFormat) << std::endl;
      }
      Y(i) = Y(i) > 0;
  }
}

}
