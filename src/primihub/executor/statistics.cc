#include "src/primihub/executor/statistics.h"

#include <glog/logging.h>

namespace primihub {
#ifndef MPC_SOCKET_CHANNEL
retcode MPCSumOrAvg::PlainTextDataCompute(
    std::shared_ptr<primihub::Dataset>& dataset,
    const std::vector<std::string>& columns,
    const std::map<std::string, ColumnDtype>& all_type,
    eMatrix<double>* col_sum,
    eMatrix<double>* col_count) {
  auto table = std::get<std::shared_ptr<arrow::Table>>(dataset->data);

  LOG(INFO) << "Schema of table is:" << table->schema()->ToString(true);
  col_sum->resize(columns.size(), 1);
  col_count->resize(columns.size(), 1);

  size_t col_index = 0;
  for (const auto &column : columns) {
    double sum = 0, count = 0;
    auto chunked_array = table->GetColumnByName(column);
    if (chunked_array.get() == nullptr) {
      LOG(ERROR) << "Can't get column value by column name " << column
                 << " from table.";
      LOG(ERROR) << "Schema of table is:\n" << table->schema()->ToString();
      return retcode::FAIL;
    }

    LOG(INFO) << "Get sum and count of column " << column << ".";

    for (int i = 0; i < chunked_array->num_chunks(); i++) {
      // Calculate sum of the column and value count of it.
      auto iter = all_type.find(column);
      const ColumnDtype &col_type = iter->second;
      if (col_type == ColumnDtype::INTEGER || col_type == ColumnDtype::LONG) {
        auto detected_type = table->schema()->GetFieldByName(column)->type();
        if ((detected_type->id() == arrow::Type::UINT8) ||
            (detected_type->id() == arrow::Type::INT8)) {
          auto i8_array = std::static_pointer_cast<arrow::Int8Array>(
              chunked_array->chunk(i));
          for (int64_t j = 0; j < i8_array->length(); j++)
            sum += i8_array->Value(j);
          count += i8_array->length();
        } else if ((detected_type->id() == arrow::Type::UINT16) ||
                   (detected_type->id() == arrow::Type::INT16)) {
          auto i16_array = std::static_pointer_cast<arrow::Int16Array>(
              chunked_array->chunk(i));
          for (int64_t j = 0; j < i16_array->length(); j++)
            sum += i16_array->Value(j);
          count += i16_array->length();
        } else if ((detected_type->id() == arrow::Type::UINT32) ||
                   (detected_type->id() == arrow::Type::INT32)) {
          auto i32_array = std::static_pointer_cast<arrow::Int32Array>(
              chunked_array->chunk(i));
          for (int64_t j = 0; j < i32_array->length(); j++)
            sum += i32_array->Value(j);
          count += i32_array->length();
        } else {
          auto i64_array = std::static_pointer_cast<arrow::Int64Array>(
              chunked_array->chunk(i));
          for (int64_t j = 0; j < i64_array->length(); j++)
            sum += i64_array->Value(j);
          count += i64_array->length();
        }
      } else if (col_type == ColumnDtype::DOUBLE) {
        auto fp64_array = std::static_pointer_cast<arrow::DoubleArray>(
            chunked_array->chunk(i));
        for (int64_t j = 0; j < fp64_array->length(); j++)
          sum += fp64_array->Value(j);
        count += fp64_array->length();
      } else {
        LOG(ERROR)
            << "Only support column that dtype of which is integer or double.";
        return retcode::FAIL;
      }
    }

    // Save sum and count of value in the column.
    (*col_sum)(col_index, 0) = sum;
    (*col_count)(col_index, 0) = count;
    col_index++;

    VLOG(3) << "Calculate sum and count of column " << column
            << " finish, count " << count << ", sum " << sum << ".";
  }

  LOG(INFO) << "Build matrix that saves sum and count of value in all columns "
               "finish, shape of matrix is "
            << "(" << col_sum->rows() << ", " << col_sum->cols() << ").";
  return retcode::SUCCESS;
}

retcode MPCSumOrAvg::CipherTextDataCompute(const eMatrix<double>& col_sum,
    const std::vector<std::string>& col_name,
    const eMatrix<double>& col_count) {
  sf64Matrix<D16> sh_sums[3];
  for (uint8_t i = 0; i < 3; i++)
    sh_sums[i].resize(col_sum.rows(), col_sum.cols());

  for (uint16_t i = 0; i < 3; i++) {
    if (i == party_id_)
      mpc_op_->createShares(col_sum, sh_sums[i]);
    else
      mpc_op_->createShares(sh_sums[i]);
  }

  // Secure share count of every column in local.
  LOG(INFO) << "Secure share count of all column, local party id " << party_id_
            << ".";

  eMatrix<double> fp64_count(col_count.rows(), col_count.cols());
  for (uint64_t j = 0; j < col_count.rows(); j++)
    fp64_count(j, 0) = col_count(j, 0);

  sf64Matrix<D16> sh_count[3];
  for (uint8_t i = 0; i < 3; i++)
    sh_count[i].resize(col_count.rows(), col_count.cols());

  for (uint16_t i = 0; i < 3; i++) {
    if (i == party_id_)
      mpc_op_->createShares(fp64_count, sh_count[i]);
    else
      mpc_op_->createShares(sh_count[i]);
  }

  // Value sum of every column in all party.
  sf64Matrix<D16> sh_all_sum;
  sh_all_sum.resize(sh_sums[0].rows(), sh_sums[0].cols());
  sh_all_sum = sh_sums[0] + sh_sums[1] + sh_sums[2];

  // Count sum of every column in all party.
  sf64Matrix<D16> sh_all_count;
  sh_all_count.resize(sh_count[0].rows(), sh_count[0].cols());
  sh_all_count = sh_count[0] + sh_count[1] + sh_count[2];

  if (avg_result_) {
    // Get average value.
    if (use_mpc_div_) {
      LOG(INFO) << "Run MPC Div after MPC Sum.";
      sf64Matrix<D16> sh_avg = mpc_op_->MPC_Div(sh_all_sum, sh_all_count);
      result.resize(col_sum.rows(), col_sum.cols());
      for (uint16_t i = 0; i < 3; i++)
        if (i == party_id_)
          result = mpc_op_->reveal(sh_avg);
        else
          mpc_op_->reveal(sh_avg, i);
    } else {
      eMatrix<double> plaintext_sum;
      for (uint16_t i = 0; i < 3; i++)
        if (i == party_id_)
          plaintext_sum = mpc_op_->reveal(sh_all_sum);
        else
          mpc_op_->reveal(sh_all_sum, i);

      eMatrix<double> plaintext_count;
      for (uint16_t i = 0; i < 3; i++)
        if (i == party_id_)
          plaintext_count = mpc_op_->reveal(sh_all_count);
        else
          mpc_op_->reveal(sh_all_count, i);

      result.resize(sh_all_sum.rows(), sh_all_sum.cols());
      for (int i = 0; i < plaintext_sum.rows(); i++)
        result(i, 0) = plaintext_sum(i, 0) / plaintext_count(i, 0);
    }
  } else {
    // Get sum value.
    eMatrix<double> plaintext_sum(sh_all_sum.rows(), sh_all_sum.rows());
    for (uint16_t i = 0; i < 3; i++)
      if (i == party_id_)
        plaintext_sum = mpc_op_->reveal(sh_all_sum);
      else
        mpc_op_->reveal(sh_all_sum, i);

    result.resize(sh_all_sum.rows(), sh_all_sum.cols());
    for (int i = 0; i < plaintext_sum.rows(); i++)
      result(i, 0) = plaintext_sum(i, 0);
  }
  LOG(INFO) << "Finish CipherText Data Compute ";
  return retcode::SUCCESS;
}

retcode MPCSumOrAvg::run(std::shared_ptr<primihub::Dataset> &dataset,
                         const std::vector<std::string> &columns,
                         const std::map<std::string, ColumnDtype> &all_type) {
  eMatrix<double> col_sum;
  eMatrix<double> rows_per_column;
  // run local plain data compute to get sum and total rows for each columns
  auto ret = PlainTextDataCompute(dataset, columns, all_type,
                                  &col_sum, &rows_per_column);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "PlainTextDataCompute failed";
    return retcode::FAIL;
  }
  // Secure share value sum of every column in local.
  LOG(INFO) << "Secure share sum of all column, local party id " << party_id_
            << ".";
  ret = CipherTextDataCompute(col_sum, columns, rows_per_column);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "CipherTextDataCompute failed";
    return retcode::FAIL;
  }
  LOG(INFO) << "Finish.";
  return retcode::SUCCESS;
}

retcode MPCSumOrAvg::getResult(eMatrix<double> &col_avg) {
  if ((col_avg.rows() != result.rows()) || (col_avg.cols() != result.cols())) {
    col_avg.resize(result.rows(), result.cols());
    LOG(WARNING) << "Wrong shape of output matrix, reshape it.";
  }

  for (int i = 0; i < result.rows(); i++) {
    col_avg(i, 0) = result(i, 0);
  }

  return retcode::SUCCESS;
}

double MPCMinOrMax::minValueOfAllParty(double local_min) {
  // Compare value from party 0 and party 1.
  f64Matrix<D16> m(1, 1);
  m(0, 0) = local_min;

  sbMatrix sh_result;
  if (party_id_ == 0)
    mpc_op_->MPC_Compare(m, sh_result);
  else if (party_id_ == 1)
    mpc_op_->MPC_Compare(m, sh_result);
  else
    mpc_op_->MPC_Compare(sh_result);

  i64Matrix cmp_result;
  cmp_result.resize(1, 1);
  cmp_result = mpc_op_->revealAll(sh_result);

  uint8_t smaller_party = UINT8_MAX;
  if (cmp_result(0, 0))
    smaller_party = 0;
  else
    smaller_party = 1;

  LOG(INFO) << "The min value of party 0 and party 1 is at party "
            << std::to_string(smaller_party) << ".";

  uint8_t smallest_party = UINT8_MAX;
  if (smaller_party == 0) {
    // Compare value from party 0 and party 2.
    sbMatrix sh_result;
    if (party_id_ == 0)
      mpc_op_->MPC_Compare(m, sh_result);
    else if (party_id_ == 2)
      mpc_op_->MPC_Compare(m, sh_result);
    else
      mpc_op_->MPC_Compare(sh_result);

    cmp_result(0, 0) = UINT8_MAX;
    cmp_result = mpc_op_->revealAll(sh_result);

  } else {
    // Compare value from party 1 and party 2.
    sbMatrix sh_result;
    if (party_id_ == 1)
      mpc_op_->MPC_Compare(m, sh_result);
    else if (party_id_ == 2)
      mpc_op_->MPC_Compare(m, sh_result);
    else
      mpc_op_->MPC_Compare(sh_result);

    cmp_result(0, 0) = UINT8_MAX;
    cmp_result = mpc_op_->revealAll(sh_result);
  }

  if (smaller_party == 0) {
    if (cmp_result(0, 0))
      smallest_party = 0;
    else
      smallest_party = 2;
  } else {
    if (cmp_result(0, 0))
      smallest_party = 1;
    else
      smallest_party = 2;
  }

  LOG(INFO) << "The min value of three party is at party "
            << std::to_string(smallest_party) << ".";

  sf64Matrix<D16> sh_m(1, 1);

  if (party_id_ == smallest_party) {
    mpc_op_->createShares(m, sh_m);
  } else
    mpc_op_->createShares(sh_m);

  eMatrix<double> min_val;
  min_val = mpc_op_->revealAll(sh_m);
  return min_val(0, 0);
}
double MPCMinOrMax::maxValueOfAllParty(double local_max) {
  // Compare value from party 0 and party 1.
  // P0:2 P1:3 P2:4  P0:-2 P1:-3 P2:-4
  f64Matrix<D16> m(1, 1);  // to out
  m(0, 0) = local_max;
  f64Matrix<D16> m_negative(1, 1);
  m_negative(0, 0) = (-1) * (local_max);  // to compare
  sbMatrix sh_result;

  // the same as min
  if (party_id_ == 0)
    mpc_op_->MPC_Compare(m_negative, sh_result);  // P0:2  -2
  else if (party_id_ == 1)
    mpc_op_->MPC_Compare(m_negative, sh_result);  // P1:3   -3
  else
    mpc_op_->MPC_Compare(sh_result);
  // because origin is smaller，keep -3

  i64Matrix cmp_result;
  cmp_result.resize(1, 1);
  cmp_result = mpc_op_->revealAll(sh_result);

  uint8_t larger_party = UINT8_MAX;
  if (cmp_result(0, 0))
    larger_party = 0;  // notice:this larger is "larger"
  else                 // to let P1(3)(-3) win
    larger_party = 1;

  LOG(INFO) << "The max value of party 0 and party 1 is at party "
            << std::to_string(larger_party) << ".";

  uint8_t largest_party = UINT8_MAX;
  if (larger_party == 0) {
    // Compare value from party 0 and party 2.
    // P0:2 P1:3 P2:4  P0:-2 P1:-3 P2:-4
    sbMatrix sh_result;
    if (party_id_ == 0)  // P0:2  -2
      mpc_op_->MPC_Compare(m_negative, sh_result);
    else if (party_id_ == 2)  // P2:4   -4
      mpc_op_->MPC_Compare(m_negative, sh_result);
    else
      mpc_op_->MPC_Compare(sh_result);
    // because origin's alg is smaller，keep -4

    cmp_result(0, 0) = UINT8_MAX;
    cmp_result = mpc_op_->revealAll(sh_result);
  } else {
    // Compare value from party 1 and party 2.
    sbMatrix sh_result;
    if (party_id_ == 1)                             //  similar to the above
      mpc_op_->MPC_Compare(m_negative, sh_result);  // P0:2 P1:3 P2:4
    else if (party_id_ == 2)
      mpc_op_->MPC_Compare(m_negative, sh_result);  //-3 -4
    else
      mpc_op_->MPC_Compare(sh_result);  // keep -4

    cmp_result(0, 0) = UINT8_MAX;
    cmp_result = mpc_op_->revealAll(sh_result);
  }
  // Because the algorithm at the beginning keeps the minimum value, it finally
  // keeps -4
  if (larger_party == 0) {
    if (cmp_result(0, 0))
      largest_party = 0;
    else
      largest_party = 2;
  } else {
    if (cmp_result(0, 0))
      largest_party = 1;
    else
      largest_party = 2;  // notice:this largest is "largest"
  }

  LOG(INFO) << "The max value of three party is at party "
            << std::to_string(largest_party) << ".";

  sf64Matrix<D16> sh_m(1, 1);

  if (party_id_ == largest_party)
    mpc_op_->createShares(m, sh_m);
  else
    mpc_op_->createShares(sh_m);

  eMatrix<double> max_val;
  max_val = mpc_op_->revealAll(sh_m);
  return max_val(0, 0);
}
retcode MPCMinOrMax::PlainTextDataCompute(
    std::shared_ptr<primihub::Dataset>& dataset,
    const std::vector<std::string>& columns,
    const std::map<std::string, ColumnDtype>& col_dtype,
    eMatrix<double>* result_data,
    eMatrix<double>* row_records) {
//
  std::shared_ptr<arrow::Table> table =
      std::get<std::shared_ptr<arrow::Table>>(dataset->data);
  LOG(INFO) << "Schema of table is:\n" << table->schema()->ToString(true);
  result_data->resize(columns.size(), 1);
  row_records->resize(columns.size(), 1);

  for (size_t col_index = 0; col_index < columns.size(); col_index++) {
    auto& col_name = columns[col_index];
    auto chunked_array = table->GetColumnByName(col_name);
    if (chunked_array.get() == nullptr) {
      LOG(ERROR) << "Can't get column value by column name " << col_name
                 << " from table.";
      LOG(ERROR) << "Schema of table is:\n" << table->schema()->ToString();
      return retcode::FAIL;
    }
    double col_result = 0;
    fillInitValue(col_result);
    // Get max or min of a column.
    for (int i = 0; i < chunked_array->num_chunks(); i++) {
      auto iter = col_dtype.find(col_name);
      const ColumnDtype &col_type = iter->second;
      if (col_type == ColumnDtype::INTEGER || col_type == ColumnDtype::LONG) {
        auto detected_type = table->schema()->GetFieldByName(col_name)->type();
        if (detected_type->id() == arrow::Type::INT32) {
          auto array = std::static_pointer_cast<arrow::Int32Array>(
              chunked_array->chunk(i));

          int32_t val = 0;
          fillInitValue(val);
          minOrMax(array, val);
          fillChunkResult(col_result, val);
        } else if (detected_type->id() == arrow::Type::INT64) {
          auto array = std::static_pointer_cast<arrow::Int64Array>(
              chunked_array->chunk(i));
          int64_t val = 0;
          fillInitValue(val);
          minOrMax(array, val);
          fillChunkResult(col_result, val);
        }
      } else if (col_type == ColumnDtype::DOUBLE) {
        auto array = std::static_pointer_cast<arrow::DoubleArray>(
            chunked_array->chunk(i));
        double val = 0;
        fillInitValue(val);
        minOrMax(array, val);
        fillChunkResult(col_result, val);
      } else {
        LOG(ERROR)
            << "Only support column that dtype of which is integer or double.";
        return retcode::FAIL;
      }
    }
    if (VLOG_IS_ON(3)) {
      if (type_ == MPCStatisticsType::MAX) {
        VLOG(3) << "Max value of column " << col_name
                << " is " << col_result << ".";
      } else {
        VLOG(3) << "Min value of column " << col_name
                << " is " << col_result << ".";
      }
    }
    (*result_data)(col_index, 0) = col_result;
  }
  return retcode::SUCCESS;
}

retcode MPCMinOrMax::CipherTextDataCompute(const eMatrix<double>& col_data,
    const std::vector<std::string>& col_names,
    const eMatrix<double>& row_records) {
  mpc_result_.resize(col_data.rows(), 1);
  if (type_ == MPCStatisticsType::MIN) {
    for (uint64_t col_index = 0; col_index < col_data.rows(); col_index++) {
      auto& col_name = col_names[col_index];
      double col_result = col_data(col_index, 0);
      col_result = minValueOfAllParty(col_result);
      mpc_result_(col_index, 0) = col_result;
      VLOG(3) << "Global min value of column " << col_name << " is "
              << mpc_result_(col_index, 0) << ".";
    }
  } else {
    for (uint64_t col_index = 0; col_index < col_data.rows(); col_index++) {
      auto& col_name = col_names[col_index];
      double col_result = col_data(col_index, 0);
      col_result = maxValueOfAllParty(col_result);
      mpc_result_(col_index, 0) = col_result;
      VLOG(3) << "Global max value of column " << col_name << " is "
              << mpc_result_(col_index, 0) << ".";
    }
  }
  return retcode::SUCCESS;
}

retcode MPCMinOrMax::run(std::shared_ptr<primihub::Dataset> &dataset,
                         const std::vector<std::string> &columns,
                         const std::map<std::string, ColumnDtype> &col_dtype) {
  eMatrix<double> col_data;
  eMatrix<double> rows_per_column;
  auto ret = PlainTextDataCompute(dataset, columns, col_dtype,
                                  &col_data, &rows_per_column);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "PlainTextDataCompute failed";
    return retcode::FAIL;
  }
  ret = CipherTextDataCompute(col_data, columns, rows_per_column);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "CipherTextDataCompute failed";
    return retcode::FAIL;
  }
  LOG(INFO) << "run MPCMinOrMax Finished";
  return retcode::SUCCESS;
}

retcode MPCMinOrMax::getResult(eMatrix<double> &result) {
  if ((mpc_result_.rows() != result.rows()) ||
      (mpc_result_.cols() != result.cols())) {
    result.resize(mpc_result_.rows(), mpc_result_.cols());
    LOG(WARNING) << "Wrong shape of output matrix, reshape it.";
  }

  for (int i = 0; i < result.rows(); i++) {
    result(i, 0) = mpc_result_(i, 0);
  }

  return retcode::SUCCESS;
}

#endif  // MPC_SOCKET_CHANNEL
};  // namespace primihub
