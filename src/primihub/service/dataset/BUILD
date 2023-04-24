package(default_visibility = ["//visibility:public",],)

cc_library(
    name = "dataset_service_impl",
    hdrs = [
        "service.h",
        "model.h",
    ],
    srcs = [
        "service.cc",
        "model.cc",
    ],
    deps = [
        ":dataset_util",
        "//src/primihub/service/dataset/localkv:leveldb_kv_impl",
        "//src/primihub/service/dataset/localkv:default_kv_impl",
        "//src/primihub/data_store:data_store_lib",
        "//src/primihub/p2p:p2p_lib",
        "//src/primihub/util:redis_helper",
        "//src/primihub/node:server_config",
        "//src/primihub/util:arrow_wrapper_util",
        "@arrow",
        "@nlohmann_json",
    ],
)

cc_library(
    name = "dataset_util",
    hdrs = [
        "util.hpp"
    ],
    deps = [
        "//src/primihub/common:common_defination",
        "//src/primihub/util:util_lib",
        "@libp2p//:p2p",
    ],
)