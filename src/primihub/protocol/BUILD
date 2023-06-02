package(default_visibility = ["//visibility:public"])

PROTOCOL_BASE_PATH="src/primihub/protocol"

cc_library(
    name = "share_lib",
    deps = [
        "//src/primihub/protocol/aby3:share_lib_impl",
    ],
)

cc_library(
  name = "protocol_base_lib",
  srcs = glob([
      "task.cc",
      "sh_task.cc",
      "scheduler.cc",
      "runtime.cc",
  ]),
  hdrs = glob([
      "task.h",
      "sh_task.h",
      "scheduler.h",
      "runtime.h",
  ]),
  linkstatic = False,
  deps = [
      "@boost//:variant",
      "//src/primihub/common:common_lib",
      "//src/primihub/util:util_lib",
      "//src/primihub/util/network:network_lib",
      "@lib_function2//:function2",
      #TODO(cuibo)
      "//src/primihub/util/network:mpc_commpkg",
  ],
)

cc_library(
    name = "protocol_aby3_lib",
    deps = [
        "//%s/aby3:aby3_lib_impl" % PROTOCOL_BASE_PATH,
    ],

)

cc_library(
    name = "protocol_cryptflow2_ot_lib",
    deps = [
        "//%s/cryptflow2:cryptflow2_ot_lib_impl" % PROTOCOL_BASE_PATH,
    ],
)

cc_library(
    name = "protocol_falcon_lib",
    deps = [
        "//%s/falcon-public:falcon_lib_impl" % PROTOCOL_BASE_PATH,
    ],
)



