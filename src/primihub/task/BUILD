package(default_visibility = ["//visibility:public"])
cc_library(
    name = "task_common",
    hdrs = ["common.h"],
    deps = [
        "//src/primihub/service:dataset_service",
        "//src/primihub/protos:common_proto",
    ],
)
cc_library(
    name = "task_factory",
    deps = [
        "//src/primihub/task/semantic:task_factory"
    ],
)

cc_library(
    name = "task_scheduler",
    deps = [
        "//src/primihub/task/semantic/scheduler:scheduler_lib",
    ],
)

