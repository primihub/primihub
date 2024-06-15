package(default_visibility = ["//visibility:public",],)
config_setting(
    name = "enable_mysql_driver",
    values = {"define": "enable_mysql_driver=true"},
)

cc_library(
    name = "data_store_lib",
    hdrs = [
        "factory.h",
    ],
    defines = select({
        "enable_mysql_driver": ["ENABLE_MYSQL_DRIVER"],
        "//conditions:default": []
    }),
    deps = [
        "//src/primihub/data_store/csv:csv_driver",
        "//src/primihub/data_store/sqlite:sqlite_driver",
        "//src/primihub/data_store/image:image_driver",
        "//src/primihub/data_store/parquet:parquet_driver",
    ] + select({
        "enable_mysql_driver": [
            "//src/primihub/data_store/mysql:mysql_driver",
        ],
        "//conditions:default": []
    }),
)

cc_library(
    name = "base_driver",
    hdrs = [
        "driver.h",
        "dataset.h"
    ],
    srcs = [
        "driver.cc",
    ],
    deps = [
        ":driver_constant",
        "//src/primihub/common:common_defination",
        "//src/primihub/util:arrow_wrapper_util",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_glog_glog//:glog",
        "@arrow",
        "@nlohmann_json",
    ],
)
cc_library(
    name = "data_store_util",
    hdrs = ["driver_legcy.h"],
    srcs = ["driver_legcy.cc"],
    deps = [
        "//src/primihub/common:data_type_defination",
    ],
)

cc_library(
  name = "driver_constant",
  hdrs = ["driver_constant.h"],
)
