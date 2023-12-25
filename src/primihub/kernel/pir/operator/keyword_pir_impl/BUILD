package(default_visibility = ["//visibility:public"])

C_OPTS = [
  "-w",
  "-D_ASPI",
]

DEP_OPTS = [
  ":common",
  "//src/primihub/kernel/pir/operator:base_pir_operator",
  "//src/primihub/util:endian_util",
  "//src/primihub/util:util_lib",
  "//src/primihub/protos:worker_proto",
  "@mircrosoft_apsi//:APSI",
]

cc_library(
  name = "keyword_pir_server_impl",
  hdrs = ["keyword_pir_client.h"],
  srcs = ["keyword_pir_client.cc"],
  copts = C_OPTS,
  deps = DEP_OPTS,
)

cc_library(
  name = "keyword_pir_client_impl",
  hdrs = ["keyword_pir_server.h"],
  srcs = ["keyword_pir_server.cc"],
  copts = C_OPTS,
  deps = DEP_OPTS,
)

cc_library(
  name = "common",
  hdrs = ["keyword_pir_common.h"],
)
