// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_ENDIAN_UTIL_H_
#define SRC_PRIMIHUB_UTIL_ENDIAN_UTIL_H_
#ifdef __linux__
#include <endian.h>
#endif
namespace primihub {
#ifdef __linux__
#define ntohll(x)     be64toh(x)
#define htonll(x)     htobe64(x)
#endif
}
#endif  // SRC_PRIMIHUB_UTIL_ENDIAN_UTIL_H_
