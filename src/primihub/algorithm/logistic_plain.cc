// Copyright [2021] <primihub.com>
#include "src/primihub/algorithm/logistic_plain.h"
#include <random>

#include "src/primihub/util/eigen_util.h"
#include "aby3/sh3/Sh3Types.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Common/Defines.h"
#include "src/primihub/common/common.h"
#include "src/primihub/algorithm/linear_model_gen.h"
#include "src/primihub/algorithm/regression.h"
#include "src/primihub/algorithm/plainML.h"
// using namespace std;
// using namespace Eigen;

namespace primihub {
template<typename T>
using eMatrix = aby3::eMatrix<T>;

using PRNG = oc::PRNG;

template <typename Derived>
void writeToCSVfile(std::string name, const Eigen::MatrixBase<Derived>& matrix)
{
    std::ofstream file(name.c_str());
    file << matrix.format(CSVFormat);
    file.close();
}

// 明文的逻辑回归
void plain_Logistic_sample(eMatrix<double>& X, eMatrix<double>& Y,
                           eMatrix<double>& mModel, double mNoise = 1,
                           double mSd = 1, bool print = false) {
  if (X.rows() != Y.rows()) throw std::runtime_error(LOCATION);
  if (1 != Y.cols()) throw std::runtime_error(LOCATION);
  if (X.cols() != mModel.rows()) throw std::runtime_error(LOCATION);
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

  Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ", ", ";", "[", "]");
  for (int i = 0; i < Y.size(); ++i) {
    if (print) {
      std::cout << X.row(i).format(HeavyFmt);
      std::cout << " -> " << (Y(i) > 0) << Y.row(i).format(HeavyFmt) << std::endl;
    }
    Y(i) = Y(i) > 0;
  }
}

int logistic_plain_main() {
  int N = 1000, D = 100, B = 128, IT = 10, testN = 100;

  PRNG prng(oc::toBlock(1));
  eMatrix<double> model(D, 1);
  for (int i = 0; i < D; ++i) {
    model(i, 0) = prng.get<int>() % 10;
  }

  eMatrix<double> train_data(N, D), train_label(N, 1);
  eMatrix<double> test_data(testN, D), test_label(testN, 1);
  plain_Logistic_sample(train_data, train_label, model);

  Eigen::MatrixXd result;
  result.resize(train_data.rows(), train_data.cols() + 1);
  result << train_data, train_label;
  writeToCSVfile("matrix.csv", result);
  plain_Logistic_sample(test_data, test_label, model);

  std::cout << "training __" << std::endl;

  RegressionParam params;
  params.mBatchSize = B;
  params.mIterations = IT;
  params.mLearningRate = 1.0 / (1 << 3);
  PlainML engine;

  eMatrix<double> W2(D, 1);
  W2.setZero();
  engine.mPrint = true;

  SGD_Logistic(params, engine, train_data, train_label, W2,
    &test_data, &test_label);

  for (int i = 0; i < D; ++i) {
    std::cout << i << " " << model(i, 0) << " " << W2(i, 0) << std::endl;
  }

  return 0;
}

int logistic_2plain_main(std::string &filename) {
  int B = 128, IT = 10000, testN = 1000;

  PRNG prng(oc::toBlock(1));
  LogisticModelGen gen;

  eMatrix<double> bytematrix;
  u64 row, col;

  bytematrix = LoadDataLocalLogistic(filename);
  row = bytematrix.rows();
  col = bytematrix.cols();

  int N = row - testN, D = col - 1;

  eMatrix<double> train_data(N, D), train_label(N, 1);
  eMatrix<double> test_data(testN, D), test_label(testN, 1);

  for (auto i = 0; i < N; i++) {
    for (auto j = 0; j < D; j++) {
      train_data(i, j) = bytematrix(i, j + 1);
    }
  }
      srand(time(nullptr));
  for (auto i = 0; i < testN; i++) {
      u64 random_row = rand() % (i + N);
    for (auto j = 0; j < D; j++) {
      test_data(i, j) = bytematrix(random_row, j + 1);
    }
  }

  for (auto i = 0; i < N; i++) {
    train_label(i, 0) = bytematrix(i, 0);
  }

  for (auto i = 0; i < testN; i++) {
    test_label(i, 0) = bytematrix(i + N, 0);
  }

  std::cout << "training __" << std::endl;

  RegressionParam params;
  params.mBatchSize = B;
  params.mIterations = IT;
  params.mLearningRate = 0.001;
  PlainML engine;

  eMatrix<double> W2(D, 1);
  W2.setZero();
  engine.mPrint = true;

  SGD_Logistic(params, engine, train_data, train_label, W2,
    &test_data, &test_label);

  for (int i = 0; i < D; ++i) {
    std::cout << i << " " << W2(i, 0) << std::endl;
  }

  return 0;
}

}
