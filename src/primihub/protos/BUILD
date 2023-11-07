package(default_visibility = ["//visibility:public",],)
load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_grpc_library", "py_proto_library")
# proto
# grpc_proto_library(
#     name = "route_guide",
#     srcs = ["src/primihub/protos/route_guide.proto"],
# )

grpc_proto_library(
  name = "dataset_proto",
  srcs = [
    "metadatastream.proto",
  ],
)

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

proto_library(
  name = "common_pb",
  srcs = ["common.proto"],
)

proto_library(
  name = "psi_pb",
  srcs = ["psi.proto"],
)

proto_library(
  name = "pir_pb",
  srcs = ["pir.proto"],
)

proto_library(
  name = "worker_pb",
  srcs = ["worker.proto"],
  deps = [
    ":common_pb",
    ":psi_pb",
    ":pir_pb",
  ],
)

proto_library(
  name = "service_pb",
  srcs = ["service.proto"],
)

py_proto_library(
  name = "service_py_pb2",
  deps = [":service_pb"],
)

py_proto_library(
  name = "worker_py_pb2",
  deps = [":worker_pb"],
)

py_grpc_library(
  name = "service_py_pb2_grpc",
  srcs = [":service_pb"],
  deps = [":service_py_pb2"],
)

py_grpc_library(
  name = "worker_py_pb2_grpc",
  srcs = [":worker_pb"],
  deps = [":worker_py_pb2"],
)
