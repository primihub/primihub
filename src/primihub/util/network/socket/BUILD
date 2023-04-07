package(default_visibility = ["//visibility:public",],)
C_OPT = [
    "-O0",
    "-g",
    "-Wall",
    "-ggdb",
    "-rdynamic",
    "-Wno-reserved-user-defined-literal",
]

LINK_OPTS = [
    "-pthread",
    "-ldl",
]

DEFINES = [
    "ENABLE_FULL_GSL",
    "ENABLE_BOOST",
    "ENABLE_RELIC",
    "ENABLE_CIRCUITS",
    "ENABLE_NASM",
    "OC_ENABLE_PORTABLE_AES",
    "USE_JSON",
    "NDEBUG",
    "TEST_LIBP2P",
]


cc_library(
    name = "network_lib",
    srcs = glob([
        "commpkg.cc",
        "acceptor.cc",
        "channel_base.cc",
        "channel.cc",
        "chl_operation.cc",
        "iobuffer.cc",
        "ioservice.cc",
        "session_base.cc",
        "session.cc",
        "tls.cc",
    ]),
    hdrs = glob([
        "commpkg.h",
        "acceptor.h",
        "channel_base.h",
        "channel.h",
        "chl_operation.h",
        "iobuffer.h",
        "ioservice.h",
        "session_base.h",
        "session.h",
        "socketadapter.h",
        "tls.h",
    ]),
    copts = C_OPT,
    defines = DEFINES,
    linkopts = LINK_OPTS,
    deps = [
        "@boost//:fiber",
        "@boost//:multiprecision",
        "@boost//:asio",
        "@boost//:system",
        "@boost//:circular_buffer",
        "@boost//:variant",
        "//src/primihub/common:common_lib",
        "//src/primihub/util:util_lib",
        "//src/primihub/util/crypto:crypto_lib",
        "//src/primihub/util/crypto:prng_lib",
    ],
    visibility = ["//visibility:public"],
)
