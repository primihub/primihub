package(default_visibility = ["//visibility:public"])
cc_library(
    name = "lib_opt_paillier_impl",
    srcs = glob([
        "src/*.cc",
    ]),
    hdrs = glob([
        "include/*.h",
    ]),
    deps = [
        "@com_github_gmp//:gmp",
    ],
)