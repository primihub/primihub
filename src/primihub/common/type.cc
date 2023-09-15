// "Copyright [2023] <PrimiHub>"
#include "src/primihub/common/type.h"
#include <string>
namespace primihub {
std::string columnDtypeToString(const ColumnDtype &type) {
  std::string type_str;
  switch (type) {
    case ColumnDtype::BOOLEAN:
      type_str = "BOOLEAN";
      break;
    case ColumnDtype::DOUBLE:
      type_str = "FP64";
      break;
    case ColumnDtype::INTEGER:
      type_str = "INT64";
      break;
    case ColumnDtype::LONG:
      type_str = "INT64";
      break;
    case ColumnDtype::ENUM:
      type_str = "ENUM";
      break;
    case ColumnDtype::STRING:
      type_str = "STRING";
      break;
    default:
      type_str = "UNKNOWN TYPE";
      break;
  }
  return type_str;
}

void columnDtypeFromInteger(int val, ColumnDtype &type) {
  switch (val) {
    case 0:
      type = ColumnDtype::STRING;
      break;
    case 1:
      type = ColumnDtype::INTEGER;
      break;
    case 2:
      type = ColumnDtype::DOUBLE;
      break;
    case 3:
      type = ColumnDtype::LONG;
      break;
    case 4:
      type = ColumnDtype::ENUM;
      break;
    case 5:
      type = ColumnDtype::BOOLEAN;
      break;
    default:
      type = ColumnDtype::UNKNOWN;
      break;
  }
}
}  // namespace primihub
