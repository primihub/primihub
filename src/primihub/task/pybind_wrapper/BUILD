package(default_visibility = ["//visibility:public"])
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

# link context
pybind_extension(
  name = "ph_secure_lib",
  srcs = [
    "task_wrapper.cc",
  ],
  deps = [
    ":mpc_task_wrapper",
    ":psi_task_wrapper",
    "//:python3_lib",
    "//src/primihub/util:util_lib",
  ],
)

py_library(
  name = "ph_secure_lib",
  data = ["ph_secure_lib.so"],
)

cc_library(
  name = "mpc_task_wrapper",
  srcs = ["mpc_task_wrapper.cc"],
  hdrs = ["mpc_task_wrapper.h"],
  deps = [
    "//src/primihub/task/semantic:mpc_task",
    "@com_github_stduuid//:stduuid_lib",
  ],
)

cc_library(
  name = "psi_task_wrapper",
  srcs = ["psi_wrapper.cc"],
  hdrs = ["psi_wrapper.h"],
  deps = [
    "//src/primihub/task/semantic:psi_task",
    "@com_github_stduuid//:stduuid_lib",
  ],
)

cc_library(
  name = "util_lib",
  hdrs = ["util.h"],
  srcs = ["util.cc"],
)

