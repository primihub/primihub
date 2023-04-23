#include "src/primihub/data_store/common/sql_driver_util.h"
#include <glog/logging.h>

namespace primihub {

static std::unordered_map<std::string, int> sql_tyep_2_arrow_type_map {
  {"bigint",  arrow::Type::type::INT64},
  {"INTEGER", arrow::Type::type::INT64},
  {"tinyint", arrow::Type::type::UINT8},
  {"int", arrow::Type::type::INT32},
  {"mediumint", arrow::Type::type::INT32},
  {"smallint", arrow::Type::type::INT32},
  {"float", arrow::Type::type::FLOAT},
  {"double", arrow::Type::type::DOUBLE},
  {"varchar", arrow::Type::type::STRING},
  {"char", arrow::Type::type::STRING},
  {"text", arrow::Type::type::STRING},
  {"binary", arrow::Type::type::BINARY},
  {"varbinary", arrow::Type::type::BINARY},
  {"blob", arrow::Type::type::BINARY},
  {"datetime", arrow::Type::type::DATE64},
  {"time", arrow::Type::type::TIME64},
  {"year", arrow::Type::type::DATE32},
  {"timestamp", arrow::Type::type::STRING},
  {"DEFAULT_TYPE", arrow::Type::type::STRING},
};

retcode SqlType2ArrowType(const std::string& sql_type, int* arrow_type) {
  auto it = sql_tyep_2_arrow_type_map.find(sql_type);
  if (it != sql_tyep_2_arrow_type_map.end()) {
    *arrow_type = it->second;
  } else {
    LOG(WARNING) << "using deflault type: STRING ";
    *arrow_type = sql_tyep_2_arrow_type_map["DEFAULT_TYPE"];
  }
  return retcode::SUCCESS;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::makeArrowArray(int field_type,
                                  const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  switch (field_type) {
  case arrow::Type::type::INT64:
  case arrow::Type::type::UINT64:
    array = Int64ArrowArrayBuilder(arr);
    break;
  case arrow::Type::type::INT32:
  case arrow::Type::type::INT16:
  case arrow::Type::type::INT8:
  case arrow::Type::type::UINT32:
  case arrow::Type::type::UINT16:
  case arrow::Type::type::UINT8:
    array = Int32ArrowArrayBuilder(arr);
    break;
  case arrow::Type::type::FLOAT:
    array = FloatArrowArrayBuilder(arr);
    break;
  case arrow::Type::type::DOUBLE:
    array = DoubleArrowArrayBuilder(arr);
    break;
  case arrow::Type::type::STRING:
    array = StringArrowArrayBuilder(arr);
    break;
  default:
    LOG(ERROR) << "unkonw data type: " << field_type
        << " consider as string type";
    array = StringArrowArrayBuilder(arr);
    break;
  }
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::Int32ArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<int32_t> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    tmp_arr.push_back(stoi(item));
  }
  arrow::Int32Builder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::Int64ArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<int64_t> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    tmp_arr.push_back(stoi(item));
  }
  arrow::Int64Builder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::FloatArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<float> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    tmp_arr.push_back(std::stof(item));
  }
  arrow::FloatBuilder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::DoubleArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<double> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    tmp_arr.push_back(std::stof(item));
  }
  arrow::DoubleBuilder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::StringArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  arrow::StringBuilder builder;
  builder.AppendValues(arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array>
SqlCommonOperation::BinaryArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  arrow::BinaryBuilder builder;
  builder.AppendValues(arr);
  builder.Finish(&array);
  return array;
}
}  // namespace primihub