package(default_visibility = ["//visibility:public",],)
C_OPT = []
LINK_OPTS = []

cc_library(
  name = "client_sdk",
  hdrs = ["cli.h"],
  srcs = ["cli.cc"],
  copts = C_OPT,
  linkstatic = True,
  linkopts = LINK_OPTS,
  deps = [
    ":common",
    "@com_github_glog_glog//:glog",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/protos:service_proto",
    "//src/primihub/common:common_defination",
    "//src/primihub/util:util_lib",
    "@openssl",
    "//src/primihub/util/network:communication_lib",
    "@nlohmann_json",
  ],
)

cc_library(
  name = "common",
  hdrs = ["common.h"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/protos:service_proto",
    "@com_github_glog_glog//:glog",
    "@com_github_stduuid//:stduuid_lib",
  ],
)

cc_library(
  name = "run_task_lib",
  hdrs = ["task_config_parser.h"],
  srcs = ["task_config_parser.cc"],
  deps = [
    ":client_sdk",
    "//src/primihub/common:common_defination",
    "@nlohmann_json",
  ],
)

cc_library(
  name = "dataset_reg_lib",
  hdrs = ["dataset_config_parser.h"],
  srcs = ["dataset_config_parser.cc"],
  deps = [
    ":client_sdk",
    "//src/primihub/common:common_defination",
    "@nlohmann_json",
  ],
)

cc_library(
  name = "cli_impl",
  srcs = ["main.cc"],
  deps = [
    ":common",
    ":run_task_lib",
    ":dataset_reg_lib",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_google_absl//absl/memory",
  ],
)