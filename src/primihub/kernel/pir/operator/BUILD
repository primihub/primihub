package(default_visibility = ["//visibility:public"])
cc_library(
  name = "factory",
  hdrs = ["factory.h"],
  deps = [
    "//src/primihub/kernel/pir:common_def",
    ":base_pir_operator",
    ":keyword_pir_operator",
  ]
)

cc_library(
  name = "base_pir_operator",
  hdrs = ["base_pir.h"],
  srcs = ["base_pir.cc"],
  deps = [
    "//src/primihub/kernel/pir:common_def",
    "//src/primihub/util:endian_util",
    "//src/primihub/util:util_lib",
    "//src/primihub/common:common_defination",
    "//src/primihub/util/network:communication_lib",
  ],
)

cc_library(
  name = "keyword_pir_operator",
  deps = [
    "//src/primihub/kernel/pir/operator/keyword_pir_impl:keyword_pir_server_impl",
    "//src/primihub/kernel/pir/operator/keyword_pir_impl:keyword_pir_client_impl",
  ]
)
