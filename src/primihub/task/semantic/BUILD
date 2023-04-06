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

config_setting(
   name = "microsoft-apsi",
   values = {"define": "microsoft-apsi=true"},
)

#factory for create task
cc_library(
    name = "task_factory",
    hdrs = ["factory.h"],
    deps = [
        ":fl_task",
        ":mpc_task",
        ":pir_task",
        ":psi_task",
        ":tee_task",
        ":private_server_base",
    ],
)
cc_library(
    name = "task_context",
    hdrs = ["task_context.h"],
    srcs = ["task_context.cc"],
    deps = [
        "//src/primihub/util/network:communication_lib",
        "//src/primihub/node:server_config",
        "//src/primihub/util:threadsafe_queue",
    ],
)
cc_library(
    name = "task_interface",
    hdrs = ["task.h"],
    srcs = ["task.cc"],
    deps = [
        ":task_context",
        "//src/primihub/protos:worker_proto",
        "//src/primihub/service:dataset_service",
    ],
)

#fl task
cc_library(
    name = "fl_task",
    hdrs = ["fl_task.h"],
    srcs = ["fl_task.cc"],
    deps = [
        ":task_interface",
        "//src/primihub/util:util_lib",
        "@com_github_base64_cpp//:base64_lib",
        "@boost//:process",
    ],
)

#mpc task
cc_library(
    name = "mpc_task",
    hdrs = ["mpc_task.h"],
    srcs = ["mpc_task.cc"],

    copts = select({
        ":x86_64": ["-maes"],
        ":aarch64": [],
        "//conditions:default": [],
    }),

    deps = [
        ":task_interface",
        "//src/primihub/util/network:network_lib",
        "//src/primihub/algorithm:algorithm_lib",

    ] + select({
        ":x86_64": [
            "//src/primihub/algorithm:cryptflow2_algorithm_lib",
        ],
        ":aarch64": [],
        "//conditions:default": [],
    }),

)

# psi task
cc_library(
    name = "psi_task",
    deps = [
        ":ecdh_psi_task",
        ":kkrt_psi_task",
    ],
)
cc_library(
    name = "psi_task_util",
    hdrs = ["psi_task.h"],
    srcs = ["psi_task.cc"],
    deps = [
        ":task_interface",
        "//src/primihub/common:common_defination",
        "//src/primihub/data_store:data_store_lib",
        "@arrow",
    ],
)

OPENMINED_PSI = "@org_openmined_psi//private_set_intersection/cpp"
cc_library(
    name = "ecdh_psi_task",
    hdrs = ["psi_ecdh_task.h"],
    srcs = ["psi_ecdh_task.cc"],
    deps = [
        ":psi_task_util",
        "//src/primihub/util:endian_util",
        "%s:psi_client" % OPENMINED_PSI,
        "%s:psi_server" % OPENMINED_PSI,
    ],
)

cc_library(
    name = "kkrt_psi_task",
    hdrs = ["psi_kkrt_task.h"],
    srcs = ["psi_kkrt_task.cc"],
    deps = [
        ":psi_task_util",
        "//src/primihub/util:endian_util",
        "//src/primihub/protos:worker_proto",
    ] + select({
        ":aarch64": [],
        ":x86_64":  ["@osu_libpsi//:libpsi"],
        ":darwin_x86_64": [],
        "//conditions:default": [],
    })
)

cc_library(
    name = "tee_task",
    hdrs = ["tee_task.h"],
    srcs = ["tee_task.cc"],
    deps = [
        ":task_interface",
        "@pybind11",
    ],
)

# pir task
cc_library(
    name = "pir_task",
    deps = select({
        "microsoft-apsi" : [":keyword_pir_task"],
        "//conditions:default": [":id_pir_task"],
    }),
)

cc_library(
    name = "keyword_pir_task",
    hdrs = [
        "keyword_pir_client_task.h",
        "keyword_pir_server_task.h",
    ],
    srcs = [
        "keyword_pir_client_task.cc",
        "keyword_pir_server_task.cc",
    ],
    copts = [
        "-w",
        "-D_ASPI",
    ],
    defines = ["USE_MICROSOFT_APSI"],
    deps = [
        ":task_interface",
        "//src/primihub/protos:common_proto",
        "@mircrosoft_apsi//:APSI",
    ]
)
cc_library(
    name = "private_server_base",
    hdrs = ["private_server_base.h"],
    srcs = ["private_server_base.cc"],
    deps = [
        ":task_interface",
        "//src/primihub/protos:worker_proto",
        "//src/primihub/common:common_defination",
        "//src/primihub/data_store:data_store_lib",
        "//src/primihub/service:dataset_service",
    ],
)

cc_library(
    name = "id_pir_task",
    hdrs = [
        "pir_client_task.h",
        "pir_server_task.h",
    ],
    srcs = [
        "pir_client_task.cc",
        "pir_server_task.cc",
    ],
    deps = [
        ":private_server_base",
        ":task_interface",
        "@org_openmined_pir//pir/cpp:pir",
    ],
)

# task semantic parser
cc_library(
    name = "task_semantic_parser",
    hdrs = ["parser.h"],
    srcs = ["parser.cc"],
    deps = [
        ":task_factory",
        "//src/primihub/task/language:language_parser_factory",
        "//src/primihub/task/semantic/scheduler:scheduler_factory",
        "//src/primihub/task:task_common",
        "//src/primihub/service:dataset_service",
    ]
)