package(default_visibility = ["//visibility:public"])

cc_library(
    name = "language_parser_factory",
    hdrs = [
        "factory.h",
    ],
    deps = [
        ":proto_parser",
        ":python_parser",
    ],
)

cc_library(
    name = "parser_interface",
    hdrs = [
      "parser.h",
    ],
    srcs = [
      "parser.cc",
    ],
    deps = [
      "//src/primihub/protos:worker_proto",
      "//src/primihub/service:dataset_service",
       "//src/primihub/util:log_util",
      "//src/primihub/util:pb_log_helper",
    ],
)

cc_library(
    name = "proto_parser",
    hdrs = ["proto_parser.h"],
    srcs = ["proto_parser.cc"],
    deps = [
        ":parser_interface",
        "//src/primihub/util:util_lib",
        "//src/primihub/common:common_defination",
        "//src/primihub/util:log_util",
        "//src/primihub/util:pb_log_helper",
    ],
)

cc_library(
    name = "python_parser",
    hdrs = ["py_parser.h"],
    srcs = ["py_parser.cc"],
    copts = [
        "-fvisibility=hidden"
    ],
    deps = [
        ":parser_interface",
        "//src/primihub/task:task_common",
        "@pybind11",
        "@com_github_glog_glog//:glog",
        "//src/primihub/util:log_util",
        "//src/primihub/util:pb_log_helper",
    ],
)
