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
    "//src/primihub/protos:service_proto",
    "//src/primihub/util:util_lib",
    "//src/primihub/util:log_util",
    "//src/primihub/util:pb_log_helper",
    "//src/primihub/protos:dataset_proto",
    "//src/primihub/util:arrow_wrapper_util",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
    "@arrow",
  ],
  visibility = ["//visibility:public"],
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

cc_library(
  name = "mpc_channel",
  hdrs = ["mpc_channel.h"],
  srcs = ["mpc_channel.cc"],
  deps = [
    ":communication_lib",
    "//src/primihub/util:threadsafe_queue",
    "//src/primihub/common:common_lib",
    "//src/primihub/util:util_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
    "@ph_communication//network:channel_interface",
  ],
)

cc_library(
  name = "memory_channel",
  hdrs = ["mem_channel.h"],
  srcs = ["mem_channel.cc"],
  deps = [
    "//src/primihub/util:threadsafe_queue",
    "//src/primihub/common:common_lib",
    "//src/primihub/util:util_lib",
    "@ph_communication//network:channel_interface",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "mpc_commpkg",
  hdrs = ["mpc_commpkg.h"],
  deps = [
    ":mpc_channel",
  ],
)
