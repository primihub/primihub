package(default_visibility = ["//visibility:public"])

cc_library(
  name = "mpc_express_executor",
  hdrs = [
    "express.h",
    "statistics.h",
  ],
  srcs = [
    "express.cc",
    "statistics.cc",
  ],
  deps = [
    "//src/primihub/common:type",
    "//src/primihub/operator:aby3_operator",
    "//src/primihub/service:dataset_service",
    "@com_github_glog_glog//:glog"
  ],
)
