package(default_visibility = ["//visibility:public",],)

cc_library(
  name = "dataset_service_impl",
  hdrs = [
    "service.h",
  ],
  srcs = [
    "service.cc",
  ],
  deps = [
    ":dataset_util",
    ":model_lib",
    "//src/primihub/service/dataset/meta_service:meta_service_interface",
    "//src/primihub/service/dataset/meta_service:meta_service_grpc_impl",
    "//src/primihub/data_store:data_store_lib",
    "//src/primihub/util:redis_helper",
    "//src/primihub/common/config:server_config",
    "//src/primihub/util:arrow_wrapper_util",
    "@arrow",
    "@nlohmann_json",
  ],
)

cc_library(
  name = "model_lib",
  hdrs = ["model.h"],
  srcs = ["model.cc"],
  deps = [
    "//src/primihub/util:arrow_wrapper_util",
    "//src/primihub/data_store:base_driver",
  ],
)

cc_library(
  name = "dataset_util",
  hdrs = [
    "util.hpp"
  ],
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/util:util_lib",
  ],
)