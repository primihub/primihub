// Copyright 2023 <primihub.com>
#ifndef SRC_PRIMIHUB_COMMON_VALUE_CHECK_UTIL_H_
#define SRC_PRIMIHUB_COMMON_VALUE_CHECK_UTIL_H_
namespace primihub {
#define CHECK_RETCODE_WITH_RETVALUE(ret_code, retvalue)  \
    do {                                                \
        if (ret_code != retcode::SUCCESS) {             \
            return retvalue;                            \
        }                                               \
    } while (0);

#define CHECK_RETCODE(ret_code)                 \
    do {                                        \
        if (ret_code != retcode::SUCCESS) {     \
            return retcode::FAIL;               \
        }                                       \
    } while (0);

#define CHECK_RETCODE_WITH_ERROR_MSG(ret_code, error_msg)   \
    do {                                                    \
        if (ret_code != retcode::SUCCESS) {                 \
            LOG(ERROR) << error_msg;                        \
            return retcode::FAIL;                           \
        }                                                   \
    } while (0);

#define CHECK_NULLPOINTER(ptr, ret_code)          \
  do {                                            \
    if (ptr == nullptr) {                         \
      return ret_code;                            \
    }                                             \
  } while (0);

#define CHECK_NULLPOINTER_WITH_ERROR_MSG(ptr, error_msg)    \
  do {                                                      \
    if (ptr == nullptr) {                                   \
      LOG(ERROR) << error_msg;                              \
      return retcode::FAIL;                                 \
    }                                                       \
  } while (0);

// task worker stop check
#define CHECK_TASK_STOPPED(ret_data)                        \
    do {                                                    \
        if (this->has_stopped()) {                          \
            LOG(ERROR) << "task has been set stopped";      \
            return ret_data;                                \
        }                                                   \
    } while (0);

#define BREAK_LOOP_BY_RETCODE(ret_code, msg)                    \
  if (ret != retcode::SUCCESS) {                                \
    error_msg = msg;                                            \
    LOG(ERROR) << error_msg;                                    \
    break;                                                      \
  }

#define BREAK_LOOP_BY_RETVAL(retval, msg)                 \
  if (ret != 0) {                                         \
    error_msg = msg;                                      \
    LOG(ERROR) << error_msg;                              \
    break;                                                \
  }

#define RaiseException(err_msg)                          \
  do {                                                   \
    LOG(ERROR) << err_msg;                               \
    throw std::runtime_error(err_msg);                   \
  } while (0);

}  // namespace primihub
#endif  // SRC_PRIMIHUB_COMMON_VALUE_CHECK_UTIL_H_
