#ifndef SRC_PRIMIHUB_UTIL_LOG_WRAPPER_H_H_
#define SRC_PRIMIHUB_UTIL_LOG_WRAPPER_H_H_
#include <glog/logging.h>
#include <map>

namespace primihub {
enum class PlatFormType {
   FL = 0,
   MPC,
   PSI,
   PIR,
   TEE,
   WORKER_NODE,
};

static std::map<PlatFormType, std::string> type_to_name {
  {PlatFormType::FL, std::string("FL")},
  {PlatFormType::MPC, std::string("MPC")},
  {PlatFormType::PSI, std::string("PSI")},
  {PlatFormType::PIR, std::string("PIR")},
  {PlatFormType::WORKER_NODE, std::string("WORKER")},
};
static std::string typeToName(PlatFormType type_) {
  auto it = type_to_name.find(type_);
  if (it != type_to_name.end()) {
    return it->second;
  } else {
    return std::string("UNKNOWN");
  }
}

#define VLOG_WRAPPER(level, platform, job_id, task_id) \
  VLOG(level) << "INFO" << platform << " " << job_id << " " << task_id << " "

#define LOG_INFO_WRAPPER(platform, job_id, task_id) \
  LOG(INFO) << "INFO" << platform << " " << job_id << " " << task_id << " "

#define LOG_WARNING_WRAPPER(platform, job_id, task_id) \
  LOG(WARNING) << "WARN" << platform << " " << job_id << " " << task_id << " "

#define LOG_ERROR_WRAPPER(platform, job_id, task_id) \
  LOG(ERROR) << "ERROR" << platform << " " << job_id << " " << task_id << " "

#define V_VLOG(level, PLATFORM, JOB_ID, TASK_ID) \
    VLOG_WRAPPER(level, PLATFORM, JOB_ID, TASK_ID)

#define LOG_INFO(PLATFORM, JOB_ID, TASK_ID) \
    LOG_INFO_WRAPPER(PLATFORM, JOB_ID, TASK_ID)

#define LOG_WRANING(PLATFORM, JOB_ID, TASK_ID) \
    LOG_WARNING_WRAPPER(PLATFORM, JOB_ID, TASK_ID)

#define LOG_ERROR(PLATFORM, JOB_ID, TASK_ID) \
    LOG_ERROR_WRAPPER(PLATFORM, JOB_ID, TASK_ID)

}  // namespace primihub::log
#endif  // SRC_PRIMIHUB_UTIL_LOG_WRAPPER_H_H_
