package(default_visibility = ["//visibility:public"])

cc_library(
  name = "node_impl",
  hdrs = [
    "node_interface.h",
    "node_impl.h"
  ],
  srcs = [
    "main.cc",
    "node_interface.cc",
    "node_impl.cc",
  ],
  deps = [
    ":data_register_service",
    ":nodelet_lib",
    "//src/primihub/common:common_defination",
    "//src/primihub/node/worker:worker_lib_impl",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/common/config:config_lib",
    "//src/primihub/util:util_lib",
    "//src/primihub/task/language:language_parser_factory",
    "//src/primihub/task/semantic:task_semantic_parser",
    "//src/primihub/service/dataset/meta_service:meta_service_factory",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@arrow",
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


cc_library(
  name = "data_register_service",
  hdrs = ["ds.h"],
  srcs = ["ds.cc"],
  deps = [
    "//src/primihub/protos:service_proto",
    "//src/primihub/data_store:data_store_lib",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util:thread_local_data",
  ],
)

cc_library(
  name = "nodelet_lib",
  hdrs = ["nodelet.h"],
  srcs = ["nodelet.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/service:dataset_service",
  ],
)
