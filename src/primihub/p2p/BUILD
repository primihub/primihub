LINK_OPTS = []
COPTS = [
    "-Wall",
    "-ggdb",
    "-rdynamic",
    "-Wno-reserved-user-defined-literal",
]

cc_library(
    name = "p2p_lib",
    srcs = [
        "node_stub.cc",
    ],
    hdrs = [
        "node_stub.h",
    ],
    copts = COPTS,
    linkopts = LINK_OPTS,
    deps = [
        "@libp2p//:p2p",
        "@boost//:thread",
    ],
    linkstatic = False,
    visibility = ["//visibility:public"],
)