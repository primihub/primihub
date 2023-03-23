package(default_visibility = ["//visibility:public",],)
cc_library(
    name = "blake2_lib_c_impl",
    hdrs = glob([
        "blake2.h",
        "blake2-impl.h",
    ]),
    srcs = glob([
        "blake2bp-ref.c",
        "blake2b-ref.c",
        "blake2sp-ref.c",
        "blake2s-ref.c",
        "blake2xb-ref.c",
        "blake2xs-ref.c",
    ]),
)