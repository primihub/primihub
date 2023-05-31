package(default_visibility = ["//visibility:public"])

cc_library(
  name = "meta_service_factory",
  hdrs = ["factory.h"],
  deps = [
    ":meta_service_interface",
    ":meta_service_grpc_impl",
    ":meta_service_memory_impl",
    ":meta_service_localkv_impl",
    "//src/primihub/common:common_defination",
  ],
)

cc_library(
  name = "meta_service_interface",
  hdrs = ["interface.h"],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/service/dataset:model_lib",
  ],
)

cc_library(
  name = "meta_service_grpc_impl",
  hdrs = ["grpc_impl.h"],
  srcs = ["grpc_impl.cc"],
  deps = [
    ":meta_service_interface",
    "//src/primihub/protos:service_proto",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
  ],
)

cc_library(
  name = "meta_service_memory_impl",
  hdrs = ["memory_impl.h"],
  srcs = ["memory_impl.cc"],
  deps = [
    ":meta_service_interface",
    "//src/primihub/service/dataset/meta_service/localkv:default_kv_impl",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "meta_service_localkv_impl",
  hdrs = ["localkv_impl.h"],
  srcs = ["localkv_impl.cc"],
  deps = [
    ":meta_service_interface",
    "//src/primihub/service/dataset/meta_service/localkv:leveldb_kv_impl",
    "@com_github_glog_glog//:glog",
  ],
)