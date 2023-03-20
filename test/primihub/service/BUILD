
SERVICE_DEFAULT_DEPS = [
    "@com_google_googletest//:gtest_main",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
    "//src/primihub/protos:service_proto",
]
cc_test(
    name = "dataset_service_test",
    srcs = [
        "dataset/dataset_service_test.cc",
    ],
    deps = SERVICE_DEFAULT_DEPS + [
        "//src/primihub/service:dataset_service",
        "//src/primihub/p2p:p2p_lib"
    ],
)

cc_test(
    name = "local_kv_test",
    srcs = [
        "dataset/local_kv_test.cc",
    ],
    deps = SERVICE_DEFAULT_DEPS + [
        "//src/primihub/service:dataset_service",
        "//src/primihub/p2p:p2p_lib"
      ],
)


cc_binary(
    name = "notify_test_client",
    srcs = [
        "notify/test_client.cc",
    ],
    deps = SERVICE_DEFAULT_DEPS,
)

