#include "src/primihub/util/eigen_util.h"

namespace primihub {

template <typename Derived>
void writeToCSVfile(std::string name, const Eigen::MatrixBase<Derived>& matrix) {
  std::ofstream file(name.c_str());
  file << matrix.format(CSVFormat);
  file.close();
}

Eigen::MatrixXd openData(std::string fileToOpen) {
  std::vector<double> matrixEntries;
  std::ifstream matrixDataFile(fileToOpen);
  std::string matrixRowString;
  std::string matrixEntry;
  int matrixRowNumber;

  while (getline(matrixDataFile, matrixRowString)){
    std::stringstream matrixRowStringStream(matrixRowString);

    while(getline(matrixRowStringStream, matrixEntry, ',')){
      matrixEntries.push_back(stod(matrixEntry));
    }
    matrixRowNumber++;
  }

  return Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> (
        matrixEntries.data(), matrixRowNumber, matrixEntries.size()/matrixRowNumber);
}

}
