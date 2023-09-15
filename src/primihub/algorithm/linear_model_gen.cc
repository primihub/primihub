// Copyright [2021] <primihub.com>
#include "src/primihub/algorithm/linear_model_gen.h"

namespace primihub {
void LinearModelGen::setModel(aby3::eMatrix<double>& model,
                              double noise, double sd) {
  mModel = model;
  mNoise = noise;
  mSd = sd;
}

void LogisticModelGen::setModel(aby3::eMatrix<double>& model,
                                double noise, double sd) {
  mModel = model;
  mNoise = noise;
  mSd = sd;
}

aby3::eMatrix<double> LoadDataLocalLogistic(const std::string& full_path) {
  std::cout << "go into function: load_data_local" << full_path << std::endl;
  std::string line, num;
  std::fstream fs;

  fs.open(full_path.c_str(), std::fstream::in);
  if (!fs) {
    std::cout << "File not exists: " << full_path << std::endl;
    return aby3::eMatrix<double>(0, 0);
  } else {
    std::cout << "Start to load data from a fixed full path: "
      << full_path << std::endl;
  }

  unsigned int rowInd = 0;
  unsigned int colInd;
  std::vector<std::vector<double>> tmpVec;
  std::vector<double> tmpRow;
  while (getline(fs, line)) {
    rowInd += 1;
    colInd = 0;
    std::istringstream readstr(line);
    tmpRow.clear();
    // while (readstr >> num) {
    while (getline(readstr, num, ',')) {
      colInd += 1;
      tmpRow.push_back(std::stold(num.c_str()));
    }
    tmpVec.push_back(tmpRow);
  }
  fs.close();

  aby3::eMatrix<double> res(rowInd, colInd);
  for (unsigned int i = 0; i < rowInd; ++i) {
    for (unsigned int j = 0; j < colInd; ++j) {
      res(i, j) = tmpVec[i][j];
    }
  }

  return res;
}
}  // namespace primihub
