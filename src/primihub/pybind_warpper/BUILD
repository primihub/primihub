package(default_visibility = ["//visibility:public"])
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

LINK_PYTHON_OPTS = PLACEHOLDER-PYTHON3.X-CONFIG
INCLUDES_PYTHON_COPTS = PLACEHOLDER-PYTHON3.X-INCLUDES

# link context
pybind_extension(
    name = "linkcontext",
    srcs = [
        "network/link_context_wrapper.cc",
    ],
    copts = INCLUDES_PYTHON_COPTS,
    linkopts = LINK_PYTHON_OPTS,
    deps = [
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
        "primitive/opt_paillier_c2py.cc",
        "primitive/opt_paillier_c2py.hpp",
    ],
    copts = INCLUDES_PYTHON_COPTS,
    linkopts = LINK_PYTHON_OPTS,
    deps = [
        "//src/primihub/primitive:lib_opt_paillier",
    ],
)

py_library(
    name = "opt_paillier_c2py",
    data = ["opt_paillier_c2py.so"],
)
