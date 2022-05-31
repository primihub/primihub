// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_EIGEN_UTIL_H_
#define SRC_primihub_UTIL_EIGEN_UTIL_H_

#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <vector>

using namespace Eigen;
using namespace std;

namespace primihub {

const static Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ",", "\n", "[", "]");

const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");

template <typename Derived>
void writeToCSVfile(std::string name, const Eigen::MatrixBase<Derived>& matrix);

Eigen::MatrixXd openData(std::string fileToOpen);

}

#endif // SRC_primihub_UTIL_EIGEN_UTIL_H_
