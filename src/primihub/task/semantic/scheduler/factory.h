// Copyright [2023] <PRimihub>
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FACTORY_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FACTORY_H_
#include <memory>
#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include "src/primihub/task/semantic/scheduler/aby3_scheduler.h"
#include "src/primihub/task/semantic/scheduler/fl_scheduler.h"
#include "src/primihub/task/semantic/scheduler/mpc_scheduler.h"
#include "src/primihub/task/semantic/scheduler/tee_scheduler.h"
#include "src/primihub/protos/common.pb.h"
#include <glog/logging.h>
namespace primihub::task {
class SchedulerFactory {
 public:
  static std::unique_ptr<VMScheduler> CreateScheduler(const rpc::Task& task_config) {
    std::unique_ptr<VMScheduler> scheduler{nullptr};
    auto task_type = task_config.type();
    switch (task_type) {
      case rpc::TaskType::ACTOR_TASK:
        if (task_config.code() == "maxpool") {
          scheduler = std::make_unique<CRYPTFLOW2Scheduler>();
        } else if (task_config.code() == "lenet") {
          scheduler = std::make_unique<FalconScheduler>();
        } else {
          LOG(ERROR) << "task_config.code() == aby3";
          scheduler = std::make_unique<ABY3Scheduler>();
        }
        break;
      case rpc::TaskType::PSI_TASK:
      case rpc::TaskType::PIR_TASK:
        scheduler = std::make_unique<VMScheduler>();
        break;
      case rpc::TaskType::TEE_TASK:
        scheduler = std::make_unique<TEEScheduler>();
        break;
      default:
        LOG(ERROR) << "unknown task type: " << static_cast<int>(task_type);
        break;
    }
    return scheduler;
  }
};
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_SCHEDULER_FACTORY_H_
