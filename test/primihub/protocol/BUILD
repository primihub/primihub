cc_test(
    name = "share_test",
    srcs = [
        "aby3/share_test.cc",
    ],
    deps = [
        "//test/primihub:test_util_test",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_github_glog_glog//:glog",
        "//src/primihub/common:common_lib",
        "//src/primihub/util/network:network_lib",
        "//src/primihub/protocol:share_lib",
        "@com_github_grpc_grpc//:grpc++",
    ],
)