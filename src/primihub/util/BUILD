package(default_visibility = ["//visibility:public",],)
C_OPT = []
LINK_OPTS = []
cc_library(
  name = "endian_util",
  hdrs = [
    "endian_util.h",
  ],
)

cc_library(
  name = "thread_local_data",
  hdrs = ["thread_local_data.h"],
  srcs = ["thread_local_data.cc"],
  linkopts = [
    "-lpthread",
  ],
)

cc_library(
  name = "instruction_check_util",
  hdrs = [
    "cpu_check.h",
  ],
)

cc_library(
  name = "model_util_lib",
  srcs = glob([
    "model_util.cc",
  ]),
  hdrs = glob([
    "model_util.h",
  ]),
  copts = C_OPT,
  linkopts = LINK_OPTS,
  linkstatic = False,
  deps = [
    ":eigen_util",
    "//src/primihub/common:common_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
  ],
)

cc_library(
  name = "file_util",
  hdrs = ["file_util.h"],
  srcs = ["file_util.cc"],
  deps = [
    "//src/primihub/common/config:server_config",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "log_util",
  hdrs = ["log.h"],
  srcs = ["log.cc"],
  deps = [
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "time_util",
  hdrs = ["timer.h"],
  srcs = ["timer.cc"],
  deps = [
    ":log_util",
  ],
)

cc_library(
  name = "hash_lib",
  hdrs = ["hash.h"],
  srcs = ["hash.cc"],
  deps = [
    "@openssl",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "util_lib",
  srcs = glob([
    "util.cc",
  ]),
  hdrs = glob([
    "util.h",
  ]),
  copts = C_OPT,
  linkopts = LINK_OPTS,
  linkstatic = False,
  deps = [
    ":file_util",
    ":log_util",
    ":time_util",
    ":threadsafe_queue",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/common:common_defination",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "eigen_util",
  hdrs = [
    "eigen_util.h",
  ],
  srcs = [
    "eigen_util.cc",
  ],
  deps = [
    "@eigen//:eigen",
  ],
)

cc_library(
  name = "threadsafe_queue",
  hdrs = [
    "threadsafe_queue.h",
  ],
)

cc_library(
  name = "redis_helper",
  hdrs = [
    "redis_helper.h"
  ],
  srcs = [
    "redis_helper.cc"
  ],
  deps = [
    "@com_github_redis_hiredis//:hiredis",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "arrow_wrapper_util",
  hdrs = ["arrow_wrapper_util.h"],
  srcs = ["arrow_wrapper_util.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    ":util_lib",
    "@com_github_glog_glog//:glog",
    "@arrow",
  ],
)

cc_library(
  name = "pb_log_helper",
  hdrs = ["proto_log_helper.h"],
  srcs = ["proto_log_helper.cc"],
  deps = [
    "//src/primihub/protos:worker_proto",
    "@com_github_glog_glog//:glog",
  ],
)