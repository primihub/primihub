// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_EIGEN_UTIL_H_
#define SRC_PRIMIHUB_UTIL_EIGEN_UTIL_H_
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "Eigen/Dense"

namespace primihub {

static Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ",", ",", "[", "]");

static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", ",");

template <typename Derived>
void writeToCSVfile(std::string name, const Eigen::MatrixBase<Derived>& matrix);

Eigen::MatrixXd openData(std::string fileToOpen);

}  // namespace primihub

#endif  // SRC_PRIMIHUB_UTIL_EIGEN_UTIL_H_
