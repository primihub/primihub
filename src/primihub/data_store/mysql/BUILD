package(default_visibility = ["//visibility:public",],)
cc_library(
    name = "mysql_driver",
    hdrs = ["mysql_driver.h"],
    srcs = ["mysql_driver.cc"],
    linkopts = [
        "-L/usr/lib64/mysql",
        "-lmysqlclient",
    ],
    deps = [
        "//src/primihub/data_store:base_driver",
        "//src/primihub/util:arrow_wrapper_util",
        "//src/primihub/util:util_lib",
        "//src/primihub/util:thread_local_data",
        "@arrow",
        "@nlohmann_json",
    ],
)
