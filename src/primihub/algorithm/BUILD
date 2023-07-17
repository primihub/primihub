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

cc_library(
  name = "aby3_ml",
  srcs = [
    "aby3ML.cc",
  ],
  hdrs = [
    "aby3ML.h",
  ],
  deps = [
    "@com_github_ladnir_aby3//aby3:aby3_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
  ],
)

cc_library(
  name = "generate_linear_model",
  srcs = [
    "linear_model_gen.cc",
  ],
  hdrs = [
    "linear_model_gen.h",
  ],
  deps = [
    "@eigen//:eigen",
    "@com_github_ladnir_aby3//aby3:aby3_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
  ]
)

cc_library(
  name = "regression",
  hdrs = ["regression.h"],
  srcs = ["regression.cc"],
  deps = [
    "//src/primihub/common:common_defination",
    "@com_github_glog_glog//:glog",
    "@eigen//:eigen",
    "@com_github_ladnir_aby3//aby3:aby3_lib",
    "@ladnir_cryptoTools//:libcryptoTools",
  ],
)
cc_library(
  name = "plain_ml",
  srcs = ["plainML.cc",],
  hdrs = ["plainML.h",],
  deps = [
    "//src/primihub/common:common_defination",
    "@com_github_ladnir_aby3//aby3:aby3_lib",
  ],
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
  deps = [
    "//src/primihub/common:party_config",
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
    "//src/primihub/util/network:communication_lib",
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

#cc_library(
#    name = "algorithm_lib",
#    srcs = glob([
#        "aby3ML.cc",
#        "linear_model_gen.cc",
#        "logistic.cc",
#        "logistic_plain.cc",
#        "plainML.cc",
#        "falcon_lenet.cc",
#        "arithmetic.cc",
#        "missing_val_processing.cc",
#        "mpc_statistics.cc",
#    ]),
#    hdrs = glob([
#        "base.h",
#        "aby3ML.h",
#        "linear_model_gen.h",
#        "logistic.h",
#        "logistic_plain.h",
#        "plainML.h",
#        "regression.h",
#        "falcon_lenet.h",
#        "arithmetic.h",
#        "missing_val_processing.h",
#        "mpc_statistics.h",
#    ]),
#    linkstatic = False,
#    deps = [
#        # ":eigen",
#        "//src/primihub/common:common_lib",
#        "//src/primihub/protocol:protocol_aby3_lib",
#         "//src/primihub/protocol:protocol_falcon_lib",
#        "//src/primihub/data_store:data_store_lib",
#        "//src/primihub/util:model_util_lib",
#        "//src/primihub/util:util_lib",
#        "//src/primihub/service:dataset_service",
#        "//src/primihub/executor:mpc_express_executor",
#        "//src/primihub/data_store:data_store_util",
#        "//src/primihub/util/crypto:crypto_lib",
#        #TODO condition select
#        "//src/primihub/util/network:communication_lib",
#        "//src/primihub/util/network:mpc_commpkg"
#    ],
#)

cc_library(
    name = "cryptflow2_algorithm_lib",
    srcs = glob([
        "cryptflow2_maxpool.cc",
    ]),
    hdrs = glob([
        "base.h",
        "cryptflow2_maxpool.h",
    ]),
    copts = select({
        ":x86_64": ["-maes", "-mrdseed", "-mavx2"],
        ":aarch64": [],
        "//conditions:default": [],
    }),
    deps = [
        "//src/primihub/common:common_lib",
        "//src/primihub/data_store:data_store_lib",
        "//src/primihub/service:dataset_service",
        "//src/primihub/protocol:protocol_cryptflow2_ot_lib",
        "//src/primihub/util/network:network_lib",
        "//src/primihub/util:instruction_check_util",
        "//src/primihub/util/network:communication_lib",
    ],
)
