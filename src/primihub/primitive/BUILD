package(default_visibility = ["//visibility:public"])
cc_library(
    name = "primitive_lib",
    srcs = glob([
        "ot/share_ot.cc",
        "circuit/garble.cc",
        "circuit/circuit_library.cc",
        "circuit/beta_circuit.cc",
        "circuit/beta_library.cc",
        "ppa/kogge_stone.cpp",
    ]),
    hdrs = glob([
        "ot/share_ot.h",
        "circuit/garble.h",
        "circuit/gate.h",
        "circuit/circuit_library.h",
        "circuit/beta_circuit.h",
        "circuit/beta_library.h",
        "ppa/kogge_stone.h",
    ]),
    defines = ["ENABLE_CIRCUITS"],
    linkstatic = False,
    deps = [
        "//src/primihub/common:common_lib",
        "//src/primihub/util/crypto:crypto_lib",
        "//src/primihub/util/crypto/aes:aes_lib",
        "//src/primihub/util/crypto:prng_lib",
        "//src/primihub/util:log_util",
        "//src/primihub/util/crypto:bit_vector",
        "//src/primihub/util/network:network_lib",
        "@nlohmann_json",
        "//src/primihub/util/network:mpc_commpkg",
    ],
)
#  not found miracl (TODO)
# cc_library(
#     name = "lib_paillier",
#     srcs = glob([
#         "paillier/*.cpp",
#     ]),
#     hdrs = glob([
#         "paillier/*.h",
#     ]),
#     copts = ["-mdynamic-no-pic"],
#     linkstatic = False,
#     deps = [
#         ":lib_miracl",
#     ],
#     includes = ["thirdparty/miracl/mr_include"],
#     visibility = ["//visibility:public"],
# )
cc_library(
    name = "lib_opt_paillier",
    deps = [
        "//src/primihub/primitive/opt_paillier:lib_opt_paillier_impl",
    ],
    visibility = ["//visibility:public"],
)