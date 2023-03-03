package(default_visibility = ["//visibility:public"])
config_setting(
    name = "aarch64",
    values = {"define": "cpu_arch=aarch64"},
)

config_setting(
    name = "x86_64",
    values = {"define": "cpu_arch=x86_64"},
)
config_setting(
    name = "darwin_x86_64",
    values = {"define": "cpu_arch=darwin_x86_64"},
)

cc_library(
    name = "cryptflow2_ot_lib_impl",
    srcs = glob([
        "*.cpp",
        "BuildingBlocks/*.cpp",
        "FloatingPoint/*.cpp",
        "GC/*.cpp",
        "LinearHE/*.cpp",
        "LinearOT/*.cpp",
        "Math/*.cpp",
        "Millionaire/*.cpp",
        "NonLinear/*.cpp",
        "OT/*.cpp",
    ],
    exclude = [
        "globals_float.cpp",
        "library_float.cpp",
    ]),

    hdrs = glob([
        "*.h",
        "utils/*.h",
        "BuildingBlocks/*.h",
        "FloatingPoint/*.h",
        "GC/*.h",
        "LinearHE/*.h",
        "LinearOT/*.h",
        "Math/*.h",
        "Millionaire/*.h",
        "NonLinear/*.h",
        "OT/*.h",
        "utils/*.hpp",
        "GC/integer.hpp",
        "GC/bit.hpp",
        "utils/ArgMapping/*",
    ],
    exclude = ["globals_float.h"]),
    linkstatic = False,
    defines = ["SCI_OT"] + select({
        ":x86_64": ["ENABLE_SSE"],
        ":aarch64": [],
        "//conditions:default": [],
    }),
    copts = [
        "-I src/primihub/protocol/cryptflow2/",
        "-I src/primihub/protocol/cryptflow2/BuildingBlocks/",
        "-I src/primihub/protocol/cryptflow2/FloatingPoint/",
        "-I src/primihub/protocol/cryptflow2/GC/",
        "-I src/primihub/protocol/cryptflow2/LinearHE/",
        "-I src/primihub/protocol/cryptflow2/LinearOT/",
        "-I src/primihub/protocol/cryptflow2/Math/",
        "-I src/primihub/protocol/cryptflow2/Millionaire/",
        "-I src/primihub/protocol/cryptflow2/NonLinear/",
        "-I src/primihub/protocol/cryptflow2/OT/",
        "-I src/primihub/protocol/cryptflow2/utils/",
        "-D SCI_OT",
        "-pthread",
    ] + select({
        ":x86_64": ["-maes", "-msse4.1", "-mavx", "-mavx2", "-mrdseed"],
        ":aarch64": [],
        "//conditions:default": [],
    }),
    linkopts = ["-fopenmp"],
    deps = [
        "@com_github_primihub_seal_40//:seal",
        "@com_github_gmp//:gmp",
        "@openssl",
        "@eigen//:eigen",
    ]
)