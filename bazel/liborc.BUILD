package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

# Note: orc-proto-wrapper.cc includes orc_proto.pb.cc
# and prefix with Adaptor.hh. The Adaptor.hh
# was supposed to capture platform discrepancies.
# However, since orc_proto.pb.cc can be compiled
# with cc_proto_library successfully, there is no need
# for orc-proto-wrapper.cc
cc_library(
    name = "liborc",
    srcs = glob(
        [
            "c++/src/*.cc",
            "c++/src/*.hh",
            "c++/src/io/*.cc",
            "c++/src/io/*.hh",
            "c++/src/wrap/*.cc",
            "c++/src/wrap/*.hh",
            "c++/src/wrap/*.h",
            "c++/src/sargs/SearchArgument.cc",
            "c++/src/sargs/SearchArgument.hh",
            "c++/include/orc/*.hh",
        ],
        exclude = [
            "c++/src/wrap/orc-proto-wrapper.cc",
            "c++/src/OrcHdfsFile.cc",
        ],
    ) + select({
        "@bazel_tools//src/conditions:windows": [],
        "//conditions:default": [
            "c++/src/OrcHdfsFile.cc",
        ],
    }),
    hdrs = [
        "c++/include/orc/orc-config.hh",
        "c++/include/orc/sargs/SearchArgument.hh",
        "c++/src/wrap",
    ],
    copts = [],
    defines = [],
    includes = [
        "c++/include",
        "c++/src",
        "c++/src/io",
        "c++/src/wrap",
        "proto",
    ],
    linkopts = [],
    visibility = ["//visibility:public"],
    deps = [
        ":libhdfspp",
        ":orc_cc_proto",
        "@lz4",
        "@snappy",
        "@zlib",
        "@zstd",
    ],
)

cc_library(
    name = "libhdfspp",
    srcs = glob(
        [
            "c++/libs/libhdfspp/include/hdfspp/*.h",
        ],
        exclude = [
        ],
    ),
    hdrs = [
    ],
    copts = [],
    defines = [],
    includes = [
        "c++/libs/libhdfspp/include",
    ],
    deps = [],
)

proto_library(
    name = "orc_proto",
    srcs = ["proto/orc_proto.proto"],
)

cc_proto_library(
    name = "orc_cc_proto",
    deps = [":orc_proto"],
)

genrule(
    name = "orc-config_hh",
    srcs = ["c++/include/orc/orc-config.hh.in"],
    outs = ["c++/include/orc/orc-config.hh"],
    cmd = ("sed " +
           "-e 's/@ORC_VERSION@/1.6.7/g' " +
           "-e 's/cmakedefine/define/g' " +
           "$< >$@"),
)

genrule(
    name = "Adaptor_hh",
    srcs = ["c++/src/Adaptor.hh.in"],
    outs = ["c++/src/Adaptor.hh"],
    cmd = select({
        "@bazel_tools//src/conditions:windows": (
            "sed " +
            "-e 's/cmakedefine HAS_PREAD/undef HAS_PREAD/g' " +
            "-e 's/cmakedefine NEEDS_REDUNDANT_MOVE/undef NEEDS_REDUNDANT_MOVE/g' " +
            "-e 's/cmakedefine NEEDS_Z_PREFIX/undef NEEDS_Z_PREFIX/g' " +
            "-e 's/cmakedefine/define/g' " +
            "$< >$@"
        ),
        "@bazel_tools//src/conditions:darwin": (
            "sed " +
            "-e 's/cmakedefine NEEDS_REDUNDANT_MOVE/undef NEEDS_REDUNDANT_MOVE/g' " +
            "-e 's/cmakedefine NEEDS_Z_PREFIX/undef NEEDS_Z_PREFIX/g' " +
            "-e 's/cmakedefine/define/g' " +
            "$< >$@"
        ),
        "//conditions:default": (
            "sed " +
            "-e 's/cmakedefine INT64_IS_LL/undef INT64_IS_LL/g' " +
            "-e 's/cmakedefine HAS_POST_2038/undef HAS_POST_2038/g' " +
            "-e 's/cmakedefine NEEDS_REDUNDANT_MOVE/undef NEEDS_REDUNDANT_MOVE/g' " +
            "-e 's/cmakedefine NEEDS_Z_PREFIX/undef NEEDS_Z_PREFIX/g' " +
            "-e 's/cmakedefine/define/g' " +
            "$< >$@"
        ),
    }),
)