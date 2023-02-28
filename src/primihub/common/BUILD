
package(default_visibility = ["//visibility:public",],)
C_OPT = [
    "-O0",
    "-g",
    "-Wall",
    "-ggdb",
    "-rdynamic",
    "-Wno-reserved-user-defined-literal",
]

LINK_OPTS = [

]

CURRENT_PATH="src/primihub/common"

cc_library(
    name = "common_lib",
    copts = C_OPT + [
        "--std=c++17",
    ],
    linkopts = LINK_OPTS,
    linkstatic = False,
    deps = [
        ":common_defination",
        ":config_lib",
        ":common_util",
        ":clp_lib",
        ":eventbus_lib",
        ":finally_tool",
        ":data_type_defination",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_glog_glog//:glog",
    ],
)

cc_library(
    name = "common_defination",
    hdrs = [
        "common.h",
        "value_check_util.h"
    ],
)

cc_library(
    name = "clp_lib",
    hdrs = [
        "clp.h",
    ],
    srcs = [
        "clp.cc",
    ],
    deps = [
        ":common_defination",
        ":microsoft_gsl",
    ],
)

cc_library (
    name = "finally_tool",
    hdrs = [
        "finally.h",
    ],
)

cc_library(
    name = "common_util",
    hdrs = [
        "defines.h",
    ],
    srcs = [
        "defines.cc",
    ],
    defines = ["ENABLE_FULL_GSL"],
    linkopts = LINK_OPTS,
    linkstatic = False,
    deps = [
        ":microsoft_gsl",
        ":common_defination",
    ],
)

cc_import(
    name = "microsoft_gsl",
    deps = [
        "//%s/gsl:microsoft_gsl" % CURRENT_PATH,
    ],
)

cc_import(
    name = "data_type_defination",
    deps = [
        "//%s/type:data_type_defination" % CURRENT_PATH,
    ],
)

cc_import(
    name = "config_lib",
    deps = [
        "//%s/config:config_lib" % CURRENT_PATH,
    ],
)

cc_import(
    name = "eventbus_lib",
    deps = [
        "//%s/eventbus:eventbus_lib" % CURRENT_PATH,
    ],
)
