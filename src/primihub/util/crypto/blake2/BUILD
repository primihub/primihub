package(default_visibility = ["//visibility:public",],)
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
    name = "blake2_lib_impl",
    defines = select({
        ":x86_64": ["ENABLE_BLAKE2_SSE"],
        ":darwin_x86_64": ["ENABLE_BLAKE2_SSE"],
        "//conditions:default": [],
    }),
    copts = select({
        ":x86_64": ["-msse"],
        ":darwin_x86_64": ["-msse"],
        "//conditions:default": [],
    }),
    deps = select({
        ":x86_64": [
            "//src/primihub/util/crypto/blake2/sse:blake2_lib_sse_impl",
        ],
        ":darwin_x86_64": [
            "//src/primihub/util/crypto/blake2/sse:blake2_lib_sse_impl",
        ],
        "//conditions:default": [
            "//src/primihub/util/crypto/blake2/c:blake2_lib_c_impl",
        ],
    })
)