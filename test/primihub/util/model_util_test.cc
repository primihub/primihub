#include "gtest/gtest.h"

#include "src/primihub/util/model_util.h"

#include "Eigen/Dense"

using namespace std;
using namespace Eigen;

using namespace primihub;

TEST(Sample, Linear_sample) {
  
  int N = 1000, dim = 10, testN = 100;
  PRNG prng(toBlock(1));

  std::cout << "dim: " << dim << ", testN:" << testN << std::endl;

  eMatrix<double> model(dim, 1);
  for (u64 i = 0; i < std::min(dim, 10); ++i) {
    model(i, 0) = prng.get<int>() % 10;
    std::cout << "model: " << model(i, 0) << std::endl;
  }
  eMatrix<double> val_train_data(N, dim), val_train_label(N, 1), val_W2(dim, 1);
  std::cout << "before Logistic_sample: " << std::endl;
  // eMatrix<double> val_test_data(testN, dim), val_test_label(testN, 1);
  Logistic_sample(val_train_data, val_train_label, model);
  // Logistic_sample(val_test_data, val_test_label, model);

  // EXPECT_EQ(val_train_data.cols(), 100);
}
