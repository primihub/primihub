package(default_visibility = ["//visibility:public"])

config_setting(
    name = "aarch64",
    values = {"define": "cpu_arch=aarch64"},
)

config_setting(
    name = "x86_64",
    values = {"define": "cpu_arch=x86_64"},
)
config_setting(
    name = "darwin_x86_64",
    values = {"define": "cpu_arch=darwin_x86_64"},
)

DEFAULT_DEPS_OPT = [
  "//src/primihub/common:common_defination",
  "@eigen//:eigen",
  "@com_github_ladnir_aby3//aby3:aby3_lib",
  "@ladnir_cryptoTools//:libcryptoTools",
  "@com_github_glog_glog//:glog",
]
cc_library(
  name = "aby3_ml",
  srcs = [
    "aby3ML.cc",
  ],
  hdrs = [
    "aby3ML.h",
  ],
  deps = DEFAULT_DEPS_OPT,
)

cc_library(
  name = "generate_linear_model",
  srcs = [
    "linear_model_gen.cc",
  ],
  hdrs = [
    "linear_model_gen.h",
  ],
  deps = DEFAULT_DEPS_OPT,
)

cc_library(
  name = "regression",
  hdrs = ["regression.h"],
  srcs = ["regression.cc"],
  deps = DEFAULT_DEPS_OPT,
)

cc_library(
  name = "plain_ml",
  srcs = ["plainML.cc",],
  hdrs = ["plainML.h",],
  deps = DEFAULT_DEPS_OPT,
)

cc_library(
  name = "logistic_plain",
  srcs = ["logistic_plain.cc",],
  hdrs = ["logistic_plain.h",],
  deps = [
    "//src/primihub/util:eigen_util",
    ":regression",
    ":generate_linear_model",
    ":plain_ml",
  ],
)
cc_library(
  name = "algorithm_base",
  hdrs = ["base.h"],
  srcs = ["base.cc"],
  deps = [
    "//src/primihub/common:party_config",
    "//src/primihub/common:type",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/util/network:mpc_channel",
  ],
)

cc_library(
  name = "logistic",
  srcs = ["logistic.cc",],
  hdrs = ["logistic.h",],
  deps = [
    ":aby3_ml",
    ":algorithm_base",
    ":generate_linear_model",
    ":plain_ml",
    ":regression",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util/network:message_exchange_interface",
    "@ladnir_cryptoTools//:libcryptoTools",
    "@arrow",
    "@com_github_glog_glog//:glog",
    "@eigen//:eigen",
  ],
)

cc_library(
  name = "arithmetic",
  srcs = ["arithmetic.cc",],
  hdrs = ["arithmetic.h",],
  deps = [
    ":algorithm_base",
    "//src/primihub/executor:mpc_express_executor",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/common:common_defination",
    "//src/primihub/util/network:message_exchange_interface",
    "//src/primihub/util:util_lib",
    "@com_github_ladnir_aby3//aby3:aby3_lib",
    "@arrow",
  ],
)
cc_library(
  name = "missing_val_proc",
  srcs = ["missing_val_processing.cc",],
  hdrs = ["missing_val_processing.h",],
  deps = [
    ":algorithm_base",
    "//src/primihub/executor:mpc_express_executor",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/util/network:message_exchange_interface",
    "@arrow",
  ],
)
cc_library(
  name = "mpc_statistics",
  srcs = ["mpc_statistics.cc"],
  hdrs = ["mpc_statistics.h"],
  deps = [
    ":algorithm_base",
    "//src/primihub/executor:mpc_express_executor",
    "//src/primihub/service:dataset_service",
    "//src/primihub/util/network:communication_lib",
    "//src/primihub/util/network:message_exchange_interface",
    "@arrow",
  ],
)
cc_library(
  name = "algorithm_lib",
  deps = [
    ":algorithm_base",
    ":mpc_statistics",
    ":missing_val_proc",
    ":arithmetic",
    ":logistic",
    ":logistic_plain",
    ":plain_ml",
    ":regression",
    ":aby3_ml",
    ":generate_linear_model",

  ],
)

cc_library(
  name = "lib_opt_paillier",
  deps = [
      "//src/primihub/algorithm/opt_paillier:lib_opt_paillier_impl",
  ],
  visibility = ["//visibility:public"],
)
