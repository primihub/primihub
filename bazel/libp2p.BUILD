# Description:
#   cpp-libp2p library

package(default_visibility = ["//visibility:public"])



load("@rules_cc//cc:defs.bzl", "cc_proto_library") 
load("@rules_proto//proto:defs.bzl", "proto_library") 


proto_library(
    name = "libp2p_protos",
    srcs = [
        "src/protocol/kademlia/protobuf/kademlia.proto",
        "src/crypto/protobuf/keys.proto",    
        "src/security/plaintext/protobuf/plaintext.proto",
        "src/security/secio/protobuf/secio.proto",
        "src/security/noise/protobuf/noise.proto",
        "src/protocol/identify/protobuf/identify.proto",
        "src/protocol/gossip/protobuf/rpc.proto",
    ]
)

cc_proto_library( 
    name = "libp2p_cc_pb", 
    deps = [":libp2p_protos"], 
) 


cc_library(
    name = "p2p",
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.hpp",
        ],
        exclude = [
            "src/storage/sqlite.cpp"
        ],
    ),
    hdrs = glob([
        "include/libp2p/**/*.hpp",
        "include/generated/*.h",
        ],
        exclude = [
            "include/libp2p/sqlite.hpp"
        ],
    ),
    textual_hdrs = [
              "src/common/gsl/span",
              "src/common/gsl/gsl_assert",
              "src/common/gsl/gsl_util",
              "src/common/gsl/gsl_byte",
              "src/common/gsl/gsl_algorithm",
              "src/common/gsl/gsl",
              "src/common/gsl/pointers",
              "src/common/gsl/multi_span",
              "src/common/gsl/string_span",
    ],
    copts = [
        "--std=c++17",
        "-O3",
        "-g",
        "-Wall",
        "-ggdb",
        "-Wno-reserved-user-defined-literal",
        "-Wno-narrowing"
    ],
    includes = [
        "include/",
        "src/common/",
    ],
    include_prefix = "src/common/",
    deps = [
        # "@boost//:asio",
        "@boost//:asio_ssl",
        "@boost//:random",
        "@openssl//:openssl",
        "@openssl//:crypto",
        "@boost//:beast",
        "@boost//:multiprecision",
        "@boost//:outcome",
        "@boost//:filesystem",
        "@boost//:program_options",
        "@boost//:signals2",
        "@boost//:format",
        "@com_github_cares_cares//:ares",
        "@fmt//:fmt",
        "@com_google_absl//absl/base",
        "@com_openmpc_soralog//:soralog",
        "@com_github_masterjedy_hat_hrie//:hat-trie",
        # "@com_github_soramitsu_sqlite//:sqlite-cpp",
        "@com_github_masterjedy_di//:boost_di",
        ":libp2p_cc_pb",
    ],
)
