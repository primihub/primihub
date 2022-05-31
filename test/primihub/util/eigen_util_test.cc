#include "gtest/gtest.h"

#include "src/primihub/util/eigen_util.h"

using namespace std;
using namespace Eigen;

using namespace primihub;

TEST(Eigen_Test, writeToCSVfile) {

    Eigen::MatrixXd vals = Eigen::MatrixXd::Random(10, 3);
    writeToCSVfile("test.csv", vals);
}

TEST(Eigen_Test, openData) {
  MatrixXd matrix_test(4,4);
  matrix_test<< 1.2, 1.4, 1.6, 1.8,
                1.5, 1.7, 1.9, 1.10,
                0.8, 1.2, -0.1, -0.2,
                1.3, 99, 100, 112;

  writeToCSVfile("matrix.csv", matrix_test);
  MatrixXd matrix_test2;
  matrix_test2 = openData("matrix.csv");
  std::cout << matrix_test2<< endl;
}
