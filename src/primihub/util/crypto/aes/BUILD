package(default_visibility = ["//visibility:public",],)
cc_library(
    name = "aes_lib",
    hdrs = [
        "aes.h",
        "multikey_aes.h",
        "aes_dec.h",
    ],
    srcs = [
        "aes.cc",
        "aes_dec.cc",
    ],
    #TODO fix -maes
    defines = ["OC_ENABLE_PORTABLE_AES"],
    deps = [
        "//src/primihub/common:common_util",
        "//src/primihub/util/crypto:block_lib",
    ],
)