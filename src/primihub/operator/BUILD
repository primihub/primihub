package(default_visibility = ["//visibility:public"])
cc_library(
    name = "aby3_operator",
    hdrs = ["aby3_operator.h"],
    srcs = ["aby3_operator.cc"],
    deps = [
        "//src/primihub/util/network:network_lib",
        "//src/primihub/util:log_util",
        "//src/primihub/util:eigen_util",
        "//src/primihub/util/crypto:prng_lib",
        "//src/primihub/common:common_lib",
        "//src/primihub/protocol:protocol_aby3_lib",
        "@com_github_glog_glog//:glog",
    ],
)