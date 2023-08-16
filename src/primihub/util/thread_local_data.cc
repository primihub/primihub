// "Copyright [2023] <PrimiHub>"
#include "src/primihub/util/thread_local_data.h"
namespace primihub {
std::string& ThreadLocalErrorMsg() {
  thread_local std::string err_msg;
  return err_msg;
}

void SetThreadLocalErrorMsg(const std::string& msg_info) {
  auto& err_msg = ThreadLocalErrorMsg();
  err_msg = msg_info;
}

void ResetThreadLocalErrorMsg() {
  auto& err_msg = ThreadLocalErrorMsg();
  err_msg.clear();
}

}  // namespace primihub
