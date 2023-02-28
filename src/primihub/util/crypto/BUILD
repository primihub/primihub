package(default_visibility = ["//visibility:public",],)
C_OPT = []
DEFINES = []
LINK_OPTS = []

cc_library(
    name = "crypto_lib",
    hdrs = [
        "Blake2.h",
        "hashable.h",
        "random_oracle.h",
    ],
    srcs = [
        "Blake2.cc",
    ],
    deps = [
        "//src/primihub/util/crypto/blake2:blake2_lib_impl",
        "//src/primihub/common:common_lib",
    ],
)

cc_library(
    name = "bit_iterator",
    hdrs = [
        "bit_iterator.h",
    ],
)
cc_library(
    name = "bit_vector",
    hdrs = ["bit_vector.h"],
    srcs = ["bit_vector.cc"],
    deps = [
        ":bit_iterator",
        ":prng_lib",
        "//src/primihub/common:common_lib",
    ]
)

cc_library(
    name = "block_lib",
    hdrs = [
        "block.h",
    ],
    srcs = ["block.cc"],
    deps = [
        "//src/primihub/common:common_lib",
        ":bit_iterator",
    ],
)

cc_library(
    name = "prng_lib",
    srcs = glob([
        "prng.cc",
    ]),
    hdrs = glob([
        "prng.h",
    ]),
    deps = [
        "//src/primihub/util/crypto/aes:aes_lib",
        "//src/primihub/util:util_lib",
    ],
    linkstatic = False,
)
