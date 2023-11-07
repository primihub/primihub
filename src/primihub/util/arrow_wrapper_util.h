// Copyright [2023] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_ARROW_WRAPPER_UTIL_H_
#define SRC_PRIMIHUB_UTIL_ARROW_WRAPPER_UTIL_H_
#include <memory>
#include <string>
#include <vector>
#include "arrow/api.h"
#include "src/primihub/common/common.h"

namespace primihub::arrow_wrapper::util {
/**
 * convert sql type to arrow type, arrow type is defined by the fllowing link:
 * https://arrow.apache.org/docs/4.0/cpp/api/datatype.html?highlight=datatype#_CPPv4N5arrow4Type4typeE  //NOLINT
 *
 * input parameter:
 *  sql_type: sql type string
 *  arrow_type: sql type mapped arrow type
*/
retcode SqlType2ArrowType(const std::string& sql_type, int* arrow_type);

/**
 * construct arrow datatype from a type enum
 * if type in not in list, construct a default arrow::utf8()
*/
std::shared_ptr<arrow::DataType> MakeArrowDataType(int type);

/**
 * factory to make arrow array
 * input parameter:
 *  field_type: arrow data type
 *  arr: raw data need to convert to specify data type
*/
std::shared_ptr<arrow::Array>
MakeArrowArray(int field_type, const std::vector<std::string>& arr);

/**
 * method to convert string to int32 arrow array
*/
std::shared_ptr<arrow::Array>
Int32ArrowArrayBuilder(const std::vector<std::string>& arr);

/**
 * method to convert string to int64 arrow array
*/
std::shared_ptr<arrow::Array>
Int64ArrowArrayBuilder(const std::vector<std::string>& arr);

/**
 * method to convert string to float arrow array
*/
std::shared_ptr<arrow::Array>
FloatArrowArrayBuilder(const std::vector<std::string>& arr);

/**
 * method to convert string to double arrow array
*/
std::shared_ptr<arrow::Array>
DoubleArrowArrayBuilder(const std::vector<std::string>& arr);

/**
 * method to convert string to string arrow array
*/
std::shared_ptr<arrow::Array>
StringArrowArrayBuilder(const std::vector<std::string>& arr);

/**
 * method to convert string to binary arrow array
*/
std::shared_ptr<arrow::Array>
BinaryArrowArrayBuilder(const std::vector<std::string>& arr);

std::shared_ptr<arrow::ArrayBuilder> CreateBuilder(int type);
retcode AddIntValue(int64_t value, int expected_type,
                    std::shared_ptr<arrow::ArrayBuilder> builder);
retcode AddBoolValue(bool value, int expected_type,
                     std::shared_ptr<arrow::ArrayBuilder> builder);
retcode AddDoubleValue(double value, int expected_type,
                       std::shared_ptr<arrow::ArrayBuilder> builder);
retcode AddStringValue(const std::string& value, int expected_type,
                       std::shared_ptr<arrow::ArrayBuilder> builder);
}  // namespace primihub::arrow_wrapper::util
#endif  // SRC_PRIMIHUB_UTIL_ARROW_WRAPPER_UTIL_H_
