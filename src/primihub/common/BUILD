
package(default_visibility = ["//visibility:public",],)
C_OPT = [
  "-O0",
  "-g",
  "-Wall",
  "-ggdb",
  "-rdynamic",
  "-Wno-reserved-user-defined-literal",
]

LINK_OPTS = [

]

CURRENT_PATH="src/primihub/common"

cc_library(
  name = "common_lib",
  copts = C_OPT + [
    "--std=c++17",
  ],
  linkopts = LINK_OPTS,
  linkstatic = False,
  deps = [
    ":common_defination",
    ":config_lib",
    ":span",
    #":type",
    "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    "@com_github_glog_glog//:glog",
  ],
)

cc_library(
  name = "common_defination",
  hdrs = [
    "common.h",
    "value_check_util.h"
  ],
)

cc_library(
  name = "type",
  hdrs = ["type.h"],
  srcs = ["type.cc"],
  deps = [
    "@eigen//:eigen",
    "//src/primihub/common:common_defination",
    "@com_github_ladnir_aby3//aby3:aby3_lib",
  ],
)
cc_library(
  name = "party_config",
  srcs = ["party_config.cc"],
  hdrs = ["party_config.h"],
  deps = [
    ":common_defination",
    "//src/primihub/protos:common_proto",
  ],
)
cc_library(
  name = "span",
  hdrs = ["span.h"],
  deps = [
    "@ladnir_cryptoTools//:libcryptoTools",
  ]
)

cc_import(
  name = "config_lib",
  deps = [
    "//%s/config:config_lib" % CURRENT_PATH,
  ],
)

cc_import(
  name = "eventbus_lib",
  deps = [
    "//%s/eventbus:eventbus_lib" % CURRENT_PATH,
  ],
)

