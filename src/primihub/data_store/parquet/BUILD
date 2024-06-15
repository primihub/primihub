package(default_visibility = ["//visibility:public",],)
cc_library(
    name = "parquet_driver",
    hdrs = ["parquet_driver.h"],
    srcs = ["parquet_driver.cc"],
    deps = [
        "//src/primihub/data_store:base_driver",
        "//src/primihub/util:util_lib",
        "//src/primihub/util:thread_local_data",
        "@arrow",
        "@nlohmann_json",
    ],
)