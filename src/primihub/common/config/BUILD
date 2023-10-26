package(default_visibility = ["//visibility:public"])

cc_library(
  name = "config_lib",
  hdrs = [
    "config.h",
  ],
  srcs = [
    "config.cc",
  ],
  deps = [
    "//src/primihub/common:common_defination",
    "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "server_config",
  hdrs = ["server_config.h"],
  srcs = ["server_config.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/common/config:config_lib",
  ],
)