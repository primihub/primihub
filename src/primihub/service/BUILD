package(default_visibility = ["//visibility:public"])

SERVICE_ROOT_PATH = "src/primihub/service"

cc_library(
    name = "service_common",
    hdrs = [
        "error.hpp",
        "outcome.hpp",
        "outcome_reg.hpp",
    ],
    srcs = [
        "error.cpp",
    ],
    deps = [
        "@boost//:outcome",
    ],
)

cc_library(
    name = "dataset_service",
    deps = [
        "//%s/dataset:dataset_service_impl" % SERVICE_ROOT_PATH,
    ],
)

cc_library(
    name = "notify_serivce",
    deps = [
        "//%s/notify:notify_service_impl" % SERVICE_ROOT_PATH,
    ],
)