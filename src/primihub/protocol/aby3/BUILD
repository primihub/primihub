package(default_visibility = ["//visibility:public"])
cc_library(
  name = "aby3_lib_impl",
  srcs = glob([
      "evaluator/evaluator.cc",
      "evaluator/binary_evaluator.cc",
      "transpose.cc",
      "evaluator/piecewise.cc",
      "runtime.cc",
      "encryptor.cc",
  ]),
  hdrs = glob([
      "evaluator/evaluator.h",
      "evaluator/binary_evaluator.h",
      "evaluator/piecewise.h",
      "evaluator/converter.h",
      "transpose.h",
      "runtime.h",
      "encryptor.h",
      "sh3_gen.h",
  ]),
  linkstatic = False,
  deps = [
      "//src/primihub/protocol:protocol_base_lib",
      "//src/primihub/common:common_lib",
      "//src/primihub/util:util_lib",
      "//src/primihub/primitive:primitive_lib",
      "//src/primihub/util/crypto:crypto_lib",
      "@lib_function2//:function2",
      "@com_github_glog_glog//:glog",
  ],
)

cc_library(
    name = "share_lib_impl",
    srcs = [
        "share.cc",
    ],
    hdrs = [
        "share.h",
        "rand_ring_buffer.h",
    ],
    linkstatic = False,
    deps = [
        "//src/primihub/common:common_lib",
        "//src/primihub/util/crypto:prng_lib",
        "//src/primihub/util/network:network_lib",
        "@lib_function2//:function2",
    ],
)