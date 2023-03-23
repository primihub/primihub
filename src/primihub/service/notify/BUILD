package(default_visibility = ["//visibility:public",])
cc_library(
    name = "notify_service_impl",
    hdrs = [
        "service.h",
        "model.h",
    ],
    srcs = [
        "service.cc",
        "model.cc",
    ],
    deps = [
        "//src/primihub/protos:service_proto",
        "//src/primihub/common/eventbus:eventbus_lib",
        "@com_github_glog_glog//:glog",
        "@com_github_grpc_grpc//:grpc++",
        #"@boost//:outcome",
        #":common_lib",
        #":service_base",
    ],
)