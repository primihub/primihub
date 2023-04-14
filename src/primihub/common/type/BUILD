package(default_visibility = ["//visibility:public",],)
C_OPT = [
    # "-O0",
    # "-g",
    # "-Wall",
    # "-ggdb",
    # "-rdynamic",
    # "-Wno-reserved-user-defined-literal",
]

LINK_OPTS = [

]

cc_library(
    name = "data_type_defination",
    hdrs = [
        "type.h",
        "fixed_point.h",
        "matrix.h",
        "matrix_view.h",
    ],
    srcs = [
        "type.cc",
        "fixed_point.cc",
    ],
    copts = C_OPT + [
        "--std=c++17",
    ],
    linkstatic = False,
    deps = [
        "@eigen//:eigen",
        "@boost//:multiprecision",
        "//src/primihub/common/gsl:microsoft_gsl",
        "//src/primihub/common:common_util",
    ],
    visibility = ["//visibility:public"],
)