package(default_visibility = ["//visibility:public",],)
C_OPT = []
LINK_OPTS = []

cc_library(
  name = "communication_lib",
  srcs = [
    "link_context.cc",
    "grpc_link_context.cc",
  ],
  hdrs = [
    "link_factory.h",
    "link_context.h",
    "grpc_link_context.h",
  ],
  copts = C_OPT,
  linkopts = LINK_OPTS,
  linkstatic = False,
  deps = [
    "//src/primihub/util:threadsafe_queue",
    "//src/primihub/common:config_lib",
    "//src/primihub/protos:worker_proto",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
    name = "network_lib",
    deps = [
        "//src/primihub/util/network/socket:network_lib",
    ]
)

cc_library(
  name = "message_exchange_interface",
  hdrs = ["message_interface.h"],
  srcs = ["message_interface.cc"],
  deps = [
    ":communication_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
    "//src/primihub/util:threadsafe_queue",
  ],
)