#ifndef SGX_COMMON_LOG_UTIL_H_
#define SGX_COMMON_LOG_UTIL_H_

#include <map>
#include <string>
#include <vector>


#define COMMON_INFO(...) COMMON_LOG_LEVEL(2, __VA_ARGS__)
#define COMMON_ERROR(...) COMMON_LOG_LEVEL(-1, __VA_ARGS__)

#define RETURN_ERROR_STR(ret, str, info)                                                                       \
  if (!(ret)) {                                                                                                \
    COMMON_ERROR("%s%s", std::string(str).c_str(), std::string(info).c_str());                                 \
    return COMMON_FAIL;                                                                                        \
  }

#define RETURN_ERROR_MSG(ret, info)                                                                            \
  if (!(ret)) {                                                                                                \
    COMMON_ERROR(MULTIPLE_PARAS info);                                                                         \
    return COMMON_FAIL;                                                                                        \
  }

#endif  // SGX_COMMON_LOG_UTIL_H_
