package(default_visibility = ["//visibility:public"])
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

# link context
pybind_extension(
    name = "linkcontext",
    srcs = [
        "network/link_context_wrapper.cc",
    ],
    deps = [
        "//:python3_lib",
        "//src/primihub/util:util_lib",
        "//src/primihub/util/network:communication_lib",
    ],
)

py_library(
    name = "linkcontext",
    data = ["linkcontext.so"],
)

# opt paillier
pybind_extension(
    name = "opt_paillier_c2py",  # This name is not actually created!
    srcs = [
        "algorithm/opt_paillier_c2py.cc",
        "algorithm/opt_paillier_c2py.hpp",
    ],
    deps = [
        "//:python3_lib",
        "//src/primihub/algorithm:lib_opt_paillier",
    ],
)

py_library(
    name = "opt_paillier_c2py",
    data = ["opt_paillier_c2py.so"],
)
