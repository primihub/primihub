UTIL_DEFAULT_DEPS = [
    "@com_google_googletest//:gtest_main",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_github_glog_glog//:glog",
    "//src/primihub/common:common_lib",
    "@com_github_grpc_grpc//:grpc++",
]
cc_test(
    name = "network_test",
    srcs = [
        "network/socket/channel_test.cc",
        "util_test.h",
    ],
    deps = UTIL_DEFAULT_DEPS + [
        "//src/primihub/primitive:primitive_lib",
        "//src/primihub/util/network:network_lib",
        "//src/primihub/protocol:protocol_aby3_lib",
    ],
)

cc_test(
    name = "wolfssl_test",
    srcs = [
        "network/socket/wolfssl_test.cc",
        "util_test.h",
    ],
    deps = UTIL_DEFAULT_DEPS + [
        "//src/primihub/primitive:primitive_lib",
        "//src/primihub/util/network:network_lib",
        "//src/primihub/protocol:protocol_aby3_lib",
    ],
)

cc_test(
    name = "prng_test",
    srcs = [
        "crypto/aes_test.cc",
        "crypto/prng_test.cc",
    ],
    deps = UTIL_DEFAULT_DEPS + [
        "//src/primihub/util/crypto:prng_lib",
    ],
)
