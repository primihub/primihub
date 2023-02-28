package(default_visibility = ["//visibility:public",],)
load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")
# proto
# grpc_proto_library(
#     name = "route_guide",
#     srcs = ["src/primihub/protos/route_guide.proto"],
# )

grpc_proto_library(
    name = "worker_proto",
    srcs = [
        "worker.proto",
    ],
    deps = [
        ":common_proto",
        ":grpc_psi_proto",
        ":grpc_pir_proto",
    ],
)

grpc_proto_library(
    name = "common_proto",
    srcs = [
        "common.proto",
    ],
)

grpc_proto_library(
    name = "grpc_psi_proto",
    srcs = [
        "psi.proto"
    ],
)

grpc_proto_library(
    name = "grpc_pir_proto",
    srcs = [
        "pir.proto",
    ],
)

grpc_proto_library(
    name = "service_proto",
    srcs = [
        "service.proto",
    ],
)
