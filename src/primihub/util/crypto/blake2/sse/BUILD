package(default_visibility = ["//visibility:public",],)
cc_library(
    name = "blake2_lib_sse_impl",
    hdrs = glob([
        "blake2.h",
        "blake2b-load-sse2.h",
        "blake2b-load-sse41.h",
        "blake2b-round.h",
        "blake2-config.h",
        "blake2-impl.h",
    ]),
    srcs = glob([
        "blake2b.c",
        "blake2bp.c",
        "blake2xb.c"
    ]),
    defines = ["ENABLE_BLAKE2_SSE"],
)