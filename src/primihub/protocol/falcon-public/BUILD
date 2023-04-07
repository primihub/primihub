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
    name = "falcon_lib_impl",
    srcs = glob([
        "*.cpp",
        "util/*.cpp"
    ]),
    hdrs = glob([
        "*.h",
        "util/*.h",
    ]),
    defines = select({
        ":x86_64": ["ENABLE_SSE"],
        ":darwin_x86_64": ["ENABLE_SSE"],
        ":aarch64": [],
        "//conditions:default": [],
    }),
    copts = [
        # TODO: Consider to remove -I flag.
        "-I src/primihub/protocol/falcon-public/",
        "-I src/primihub/protocol/falcon-public/util/",
        "-fpic",
        "-Wno-narrowing",
    ] + select({
        ":x86_64": ["-maes", "-mpclmul"],
        ":darwin_x86_64": ["-maes", "-mpclmul"],
        ":aarch64": [],
        "//conditions:default": [],
    }),
    linkstatic = False,
    deps = [
        "//src/primihub/common:common_util",
        "@eigen//:eigen",
        "@openssl",
    ]
)