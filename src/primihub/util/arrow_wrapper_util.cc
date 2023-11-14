// Copyright [2023] <primihub.com>
#include "src/primihub/util/arrow_wrapper_util.h"
#include <glog/logging.h>
#include <algorithm>
#include "src/primihub/util/util.h"

namespace primihub::arrow_wrapper::util {
static std::unordered_map<std::string, int> sql_tyep_2_arrow_type_map {
  {"bigint",  arrow::Type::type::INT64},
  {"integer", arrow::Type::type::INT64},
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

std::shared_ptr<arrow::DataType> MakeArrowDataType(int type) {
  switch (type) {
  case arrow::Type::type::BOOL:
    return arrow::boolean();
  case arrow::Type::type::UINT8:
    return arrow::uint8();
  case arrow::Type::type::INT8:
    return arrow::int8();
  case arrow::Type::type::UINT16:
    return arrow::uint16();
  case arrow::Type::type::INT16:
    return arrow::int16();
  case arrow::Type::type::UINT32:
    return arrow::uint32();
  case arrow::Type::type::INT32:
    return arrow::int32();
  case arrow::Type::type::UINT64:
    return arrow::uint64();
  case arrow::Type::type::INT64:
    return arrow::int64();
  case arrow::Type::type::HALF_FLOAT:
    return arrow::float16();
  case arrow::Type::type::FLOAT:
    return arrow::float32();
  case arrow::Type::type::DOUBLE:
    return arrow::float64();
  case arrow::Type::type::STRING:
    return arrow::utf8();
  case arrow::Type::type::BINARY:
    return arrow::binary();
  case arrow::Type::type::DATE32:
    return arrow::date32();
  case arrow::Type::type::DATE64:
    return arrow::date64();
  case arrow::Type::type::TIMESTAMP:
    return arrow::timestamp(arrow::TimeUnit::type::MILLI);
  case arrow::Type::type::TIME32:
    return arrow::time32(arrow::TimeUnit::type::MILLI);
  case arrow::Type::type::TIME64:
    return arrow::time64(arrow::TimeUnit::type::MICRO);
  // case arrow::Type::type::DECIMAL128:
  case arrow::Type::type::DECIMAL:
    return arrow::decimal(10, 0);
  case arrow::Type::type::DECIMAL256:
    return arrow::decimal(10, 0);
  default:
    return arrow::utf8();
  }
  return arrow::utf8();
}

retcode SqlType2ArrowType(const std::string& sql_type, int* arrow_type) {
  // to lower
  std::string lower_str = primihub::strToLower(sql_type);
  auto it = sql_tyep_2_arrow_type_map.find(lower_str);
  if (it != sql_tyep_2_arrow_type_map.end()) {
    *arrow_type = it->second;
  } else {
    LOG(WARNING) << "using deflault type: STRING ";
    *arrow_type = sql_tyep_2_arrow_type_map["DEFAULT_TYPE"];
  }
  return retcode::SUCCESS;
}

std::shared_ptr<arrow::Array> MakeArrowArray(int field_type,
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
    LOG(ERROR) << "unknown data type: " << field_type
        << " consider as string type";
    array = StringArrowArrayBuilder(arr);
    break;
  }
  return array;
}

std::shared_ptr<arrow::Array> Int32ArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<int32_t> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    int32_t value{0};
    try {
      value = stoi(item);
    } catch(...) {

    }
    tmp_arr.push_back(value);
  }
  arrow::Int32Builder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array> Int64ArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<int64_t> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    int64_t value{0};
    try {
      value = stoi(item);
    } catch(...) {

    }
    tmp_arr.push_back(value);
  }
  arrow::Int64Builder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array> FloatArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<float> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    float value = 0.0f;
    try {
      value = std::stof(item);
    } catch (...) {

    }
    tmp_arr.push_back(value);
  }
  arrow::FloatBuilder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array> DoubleArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  std::vector<double> tmp_arr;
  tmp_arr.reserve(arr.size());
  for (const auto& item : arr) {
    double value = 0.0f;
    try {
      value = std::stof(item);
    } catch (...) {

    }
    tmp_arr.push_back(value);
  }
  arrow::DoubleBuilder builder;
  builder.AppendValues(tmp_arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array> StringArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  arrow::StringBuilder builder;
  builder.AppendValues(arr);
  builder.Finish(&array);
  return array;
}

std::shared_ptr<arrow::Array> BinaryArrowArrayBuilder(
    const std::vector<std::string>& arr) {
  std::shared_ptr<arrow::Array> array;
  arrow::BinaryBuilder builder;
  builder.AppendValues(arr);
  builder.Finish(&array);
  return array;
}

}  // namespace primihub::arrow_wrapper::util
