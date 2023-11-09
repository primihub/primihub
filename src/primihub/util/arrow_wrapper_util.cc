// Copyright [2023] <primihub.com>
#include "src/primihub/util/arrow_wrapper_util.h"
#include <glog/logging.h>
#include <algorithm>
#include <sstream>

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
  std::string lower_str = sql_type;
  // to lower
  std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
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

std::shared_ptr<arrow::ArrayBuilder> CreateBuilder(int type) {
  std::shared_ptr<arrow::ArrayBuilder> builder;
  switch (type) {
  case arrow::Type::type::INT32:
    builder = std::make_shared<arrow::Int32Builder>();
    break;
  case arrow::Type::type::INT64:
    builder = std::make_shared<arrow::Int64Builder>();
    break;
  case arrow::Type::type::STRING:
    builder = std::make_shared<arrow::StringBuilder>();
    break;
  case arrow::Type::type::FLOAT:
    builder = std::make_shared<arrow::FloatBuilder>();
    break;
  case arrow::Type::type::BOOL:
    builder = std::make_shared<arrow::BooleanBuilder>();
    break;
  case arrow::Type::type::DOUBLE:
    builder = std::make_shared<arrow::DoubleBuilder>();
    break;
  default:
    LOG(WARNING) << "unknown type: " << static_cast<int>(type)
                 << " using string instead";
    builder = std::make_shared<arrow::StringBuilder>();
    break;
  }
  return builder;
}

retcode AddIntValue(int64_t value,
                    int expected_type,
                    std::shared_ptr<arrow::ArrayBuilder> builder) {
  switch (expected_type) {
  case arrow::Type::type::INT32: {
    auto ptr = std::dynamic_pointer_cast<arrow::Int32Builder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::BOOL: {
    auto ptr = std::dynamic_pointer_cast<arrow::BooleanBuilder>(builder);
    ptr->Append(value == 0);
    break;
  }
  case arrow::Type::type::UINT32: {
    auto ptr = std::dynamic_pointer_cast<arrow::UInt32Builder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::INT64: {
    auto ptr = std::dynamic_pointer_cast<arrow::Int64Builder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::UINT64: {
    auto ptr = std::dynamic_pointer_cast<arrow::UInt64Builder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::STRING: {
    auto ptr = std::dynamic_pointer_cast<arrow::StringBuilder>(builder);
    ptr->Append(std::to_string(value));
    break;
  }
  case arrow::Type::type::FLOAT: {
    auto ptr = std::dynamic_pointer_cast<arrow::FloatBuilder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::DOUBLE: {
    auto ptr = std::dynamic_pointer_cast<arrow::DoubleBuilder>(builder);
    ptr->Append(value);
    break;
  }
  default:
    LOG(ERROR) << "unable to convert from int to " << expected_type;
  }
  return retcode::SUCCESS;
}

retcode AddBoolValue(bool value,
                     int expected_type,
                     std::shared_ptr<arrow::ArrayBuilder> builder) {
  if (expected_type == arrow::Type::type::BOOL) {
    auto ptr = std::dynamic_pointer_cast<arrow::BooleanBuilder>(builder);
    ptr->Append(value);
  } else {
    int64_t int_value = static_cast<int>(value);
    return AddIntValue(int_value, expected_type, builder);
  }
  return retcode::SUCCESS;
}

retcode AddDoubleValue(double value,
                       int expected_type,
                       std::shared_ptr<arrow::ArrayBuilder> builder) {
  switch (expected_type) {
  case arrow::Type::type::FLOAT: {
    auto ptr = std::dynamic_pointer_cast<arrow::FloatBuilder>(builder);
    ptr->Append(value);
    break;
  }
  case arrow::Type::type::DOUBLE: {
    auto ptr = std::dynamic_pointer_cast<arrow::DoubleBuilder>(builder);
    ptr->Append(value);
    break;
  }
  default: {
    std::ostringstream out;
    out << value;
    std::string str_value = out.str();
    return AddStringValue(str_value, expected_type, builder);
  }
  }
  return retcode::SUCCESS;
}

retcode AddStringValue(const std::string& value,
                       int expected_type,
                       std::shared_ptr<arrow::ArrayBuilder> builder) {
  switch (expected_type) {
  case arrow::Type::type::STRING: {
    auto ptr = std::dynamic_pointer_cast<arrow::StringBuilder>(builder);
    ptr->Append(value);
    break;
  }
  default: {
    try {
      double d_value = std::stod(value);
      return AddDoubleValue(d_value, expected_type, builder);
    } catch (std::exception& e) {
      LOG(ERROR) << "convert [" << value << "]to double failed. " << e.what();
      return retcode::FAIL;
    }
    break;
  }
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::arrow_wrapper::util
