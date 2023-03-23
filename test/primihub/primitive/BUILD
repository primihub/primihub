cc_test(
    name = "primitive_test",
    srcs = [
        "ot/share_ot_test.cc",
        "circuit/circuit_test.cc"
    ],
    deps = [
        "//src/primihub/primitive:primitive_lib",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_binary(
    name = "opt_paillier_test",
    srcs = [
        "opt_paillier_test.cc",
    ],
    deps = [
        "//src/primihub/primitive:lib_opt_paillier",
    ],
)

cc_binary(
    name = "opt_paillier_pack_test",
    srcs = [
        "opt_paillier_pack_test.cc",
    ],
    deps = [
        "//src/primihub/primitive:lib_opt_paillier",
    ],
)