
DEFAULT_ALGORITHM_LINK_DEPS = [
    "@com_google_googletest//:gtest_main",
    "@com_google_absl//absl/base",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "//src/primihub/service/dataset/meta_service:meta_service_factory",
]

cc_test(
    name = "logistic_test",
    srcs = [
        "logistic_test.cc"
    ],
    deps = DEFAULT_ALGORITHM_LINK_DEPS + [
        "//src/primihub/algorithm:algorithm_lib",
        "//src/primihub/util/network:memory_channel",
    ],
)

cc_library(
  name = "mpc_statistics_util_lib",
  hdrs = ["statistics_util.h"],
  srcs = ["statistics_util.cc"],
  deps = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/algorithm:algorithm_lib",
  ],
)

cc_test(
  name = "mpc_avg_test",
  srcs = [
    "statistics_avg_test.cc"
  ],
  deps = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/algorithm:algorithm_lib",
    ":mpc_statistics_util_lib",
  ],
)
cc_test(
  name = "mpc_sum_test",
  srcs = [
    "statistics_sum_test.cc"
  ],
  deps = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/algorithm:algorithm_lib",
    ":mpc_statistics_util_lib",
  ],
)
cc_test(
  name = "mpc_min_test",
  srcs = [
    "statistics_min_test.cc"
  ],
  deps = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/algorithm:algorithm_lib",
    ":mpc_statistics_util_lib",
  ],
)
cc_test(
  name = "mpc_max_test",
  srcs = [
    "statistics_max_test.cc"
  ],
  deps = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/algorithm:algorithm_lib",
    ":mpc_statistics_util_lib",
  ],
)

cc_test(
    name = "maxpool_test",
    srcs = [
        "maxpool_test.cc"
    ],
    copts = [
      "-maes",
    ],
    deps = DEFAULT_ALGORITHM_LINK_DEPS + [
        #"//src/primihub/algorithm:cryptflow2_algorithm_lib",
    ]
)

cc_test(
    name = "falcon_lenet_test",
    srcs = [
        "falcon_lenet_test.cc",
    ],
    deps = DEFAULT_ALGORITHM_LINK_DEPS + [
        "//src/primihub/algorithm:algorithm_lib",
    ],
)

ABY3_DEPS = DEFAULT_ALGORITHM_LINK_DEPS + [
    "//src/primihub/protocol:protocol_aby3_lib",
    "//src/primihub/util/network:network_lib",
    "//src/primihub/operator:aby3_operator",
    "@com_github_grpc_grpc//:grpc++",
]
cc_test(
    name = "aby3_div_test",
    srcs = [
        "aby3_div_test.cc",
    ],
    deps = ABY3_DEPS,
)


cc_test(
    name = "aby3_A2B_test",
    srcs = [
        # "test/primihub/protocol/aby3/evaluator/evaluator_test.cc",
        "aby3_A2B_test.cc",
        # "test/primihub/protocol/aby3/evaluator/piecewise_test.cc",
        # "test/primihub/protocol/aby3/encryptor_test.cc",
        # "test/primihub/protocol/aby3/runtime_test.cc",
        # "test/primihub/protocol/aby3/sh3_gen_test.cc",
    ],
    deps = ABY3_DEPS,
)


cc_test(
    name = "aby3_add_test",
    srcs = [
        "aby3_add_test.cc",
    ],
    deps = ABY3_DEPS,
)

cc_test(
    name = "aby3_mul_test",
    srcs = [
        "aby3_mul_test.cc",
    ],
    deps = ABY3_DEPS,
)

cc_test(
    name = "aby3_MSB_test",
    srcs = [
        "aby3_MSB_test.cc",
    ],
    deps = ABY3_DEPS,
)