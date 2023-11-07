package(default_visibility = ["//visibility:public"])
config_setting(
  name = "enable_sgx",
  values = {"define" : "enable_sgx=true"},
)
cc_library(
  name = "node_executor_impl",
  srcs = ["main.cc"],
  deps = [
    ":node_impl",
    "//src/primihub/service/datastream:stream_data",
  ],
)

cc_library(
  name = "node_impl",
  hdrs = [
    "node_interface.h",
    "node_impl.h"
  ],
  srcs = [
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
    "//src/primihub/util:log_util",
    "//src/primihub/util:pb_log_helper",
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
  name = "data_register_service",
  hdrs = ["ds.h"],
  srcs = ["ds.cc"],
  deps = [
    "//src/primihub/protos:service_proto",
    "//src/primihub/data_store:data_store_lib",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util:thread_local_data",
    "//src/primihub/common/config:server_config",
    "//src/primihub/util:pb_log_helper",
    "//src/primihub/util:file_util",
  ],
)

cc_library(
  name = "nodelet_lib",
  hdrs = ["nodelet.h"],
  srcs = ["nodelet.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/service:dataset_service",
  ] + select({
    "enable_sgx": [
      "@tee_engine//sgx/engine:engine_lib",
      "@tee_engine//sgx/ra:ra_service"
    ],
	  "//conditions:default": [],
  }),
)
