// "Copyright [2023] <Primihub>"
#ifndef SRC_PRIMIHUB_UTIL_THREAD_LOCAL_DATA_H_
#define SRC_PRIMIHUB_UTIL_THREAD_LOCAL_DATA_H_
#include <string>
#include <thread>
namespace primihub {
std::string& ThreadLocalErrorMsg();
void SetThreadLocalErrorMsg(const std::string& msg_info);

}  // namespace primihub
#endif  // SRC_PRIMIHUB_UTIL_THREAD_LOCAL_DATA_H_
