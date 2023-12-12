package(default_visibility = ["//visibility:public",],)
C_OPT = []
LINK_OPTS = []

cc_library(
    name = "cli_impl",
    srcs = [
        "cli.cc",
        "cli.h",
    ],
    copts = C_OPT,
    linkstatic = True,
    linkopts = LINK_OPTS,
    deps = [
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/memory",
        "@com_github_glog_glog//:glog",
        "//src/primihub/protos:worker_proto",
        "//src/primihub/protos:service_proto",
        "//src/primihub/common:common_defination",
        "//src/primihub/util:util_lib",
        "@openssl",
        "@com_github_stduuid//:stduuid_lib",
        "//src/primihub/util/network:communication_lib",
        "@nlohmann_json",
    ],
)

cc_binary(
  name = "reg_cli",
  srcs = [
    "reg_cli.cc",
    "reg_cli.h",
  ],
  copts = C_OPT,
  includes = [
  ],
  linkstatic = True,
  linkopts = LINK_OPTS,
  deps = [
    "//src/primihub/common:common_defination",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/protos:service_proto",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_google_absl//absl/memory",
    "@com_github_glog_glog//:glog",
    "@openssl",
    "@com_github_stduuid//:stduuid_lib",
    "@nlohmann_json",
  ],
)