package(default_visibility = ["//visibility:public"])
cc_library(
  name = "worker_lib_impl",
  hdrs = ["worker.h"],
  srcs = ["worker.cc"],
  deps = [
    "//src/primihub/node:nodelet_lib",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/common:common_defination",
    "//src/primihub/task:task_factory",
    "//src/primihub/util:log_util",
    "//src/primihub/util:pb_log_helper",
    "@poco//:poco",
    "@com_github_base64_cpp//:base64_lib",
  ],
)