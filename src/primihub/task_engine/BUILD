package(default_visibility = ["//visibility:public"])

cc_library(
  name = "task_engine_impl",
  srcs = ["main.cc"],
  copts = [
    "-fvisibility=hidden"
  ],
  deps = [
    ":task_engine",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_google_absl//absl/memory",
    "@com_github_glog_glog//:glog",
    "@pybind11",
  ],
)

cc_library(
  name = "task_engine",
  hdrs = ["task_executor.h"],
  srcs = ["task_executor.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/util:util_lib",
    "//src/primihub/node:server_config",
    "//src/primihub/task/semantic:task_factory",
    "//src/primihub/service/dataset/meta_service:meta_service_factory",
    "//src/primihub/service:dataset_service",
    "@com_github_base64_cpp//:base64_lib",
  ],
)

cc_library(
  name = "py_executor",
  hdrs = ["py_executor.h"],
  srcs = ["py_executor.cc"],
  copts = [
    "-fvisibility=hidden"
  ],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/util:util_lib",
    "@com_github_base64_cpp//:base64_lib",
    "@pybind11",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_google_absl//absl/memory",
    "@com_github_glog_glog//:glog",
  ],
)
