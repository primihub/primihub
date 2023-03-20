package(
    default_visibility = [
        "//visibility:public",
    ],
)

#load("@rules_foreign_cc//tools/build_defs:boost_build.bzl", "boost_build")

#boost_build(
#    name = "boost_fiber",
#    lib_source = "@boost//:all",
#    static_libraries = ["libboost_fiber.a"],
#    user_options = ["--with-fiber"],
#    visibility = ["//visibility:public"],
#    deps = [":boost_context"],
#)

boost_build(
    name = "boost_context",
    lib_source = "@boost//:all",
    static_libraries = ["libboost_context.a"],
    user_options = ["--with-context"],
    visibility = ["//visibility:public"],
)

genrule(
    name = "cryptoTools_config_h",
    outs = [
        "cryptoTools/Common/config.h",
    ],
    cmd = """
        set -x
        tmpdir="cryptoTools.tmp"
        mkdir -p "$${tmpdir}/cryptoTools/Common"
        echo "#pragma once \r\n \
#define ENABLE_RELIC ON \r\n \
#define ENABLE_CIRCUITS ON \r\n \
#define ENABLE_FULL_GSL ON \r\n \
#define ENABLE_CPP_14 ON \r\n \
#define ENABLE_BOOST ON \r\n \
#define ENABLE_SSE ON \r\n \
#define ENABLE_CPP_14 ON \r\n \
#define ENABLE_FULL_GSL ON \r\n \
#define ENABLE_NET_LOG ON \r\n \
#define ENABLE_NASM ON \r\n \
#if (defined(_MSC_VER) || defined(__SSE2__)) && defined(ENABLE_SSE) \r\n \
#define ENABLE_SSE_BLAKE2 ON \r\n \
#define OC_ENABLE_SSE2 ON \r\n \
#endif \r\n \
#if (defined(_MSC_VER) || defined(__PCLMUL__)) && defined(ENABLE_SSE) \r\n \
#define OC_ENABLE_PCLMUL \r\n \
#endif \r\n \
#if (defined(_MSC_VER) || defined(__AES__)) && defined(ENABLE_SSE) \r\n \
#define OC_ENABLE_AESNI ON \r\n \
#else \r\n \
#define OC_ENABLE_PORTABLE_AES ON \r\n \
#endif \r\n \
">"$${tmpdir}"/cryptoTools/Common/config.h
        ls -ltrh "$${tmpdir}"
        mv "$${tmpdir}"/cryptoTools/Common/config.h $(location cryptoTools/Common/config.h)
        rm -r -f -- "$${tmpdir}"
    """,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cryptoTools",
    srcs = glob(
                ["cryptoTools/Circuit/*.cpp",
                "cryptoTools/Common/*.cpp",
                "cryptoTools/Crypto/*.cpp",
                "cryptoTools/Crypto/blake2/c/*.c",
                "cryptoTools/Crypto/blake2/sse/*.c",
                "cryptoTools/Network/*.cpp"],
        ),
    hdrs = [":cryptoTools_config_h"] + glob(
                ["cryptoTools/Circuit/*.h",
                "cryptoTools/Common/*.h",
                "cryptoTools/Crypto/*.h",
                "cryptoTools/Crypto/blake2/c/*.h",
                "cryptoTools/Crypto/blake2/sse/*.h",
                "cryptoTools/Network/*.h"],
        ),
    textual_hdrs = [
                    "cryptoTools/gsl/span",
                    "cryptoTools/gsl/gsl_assert",
                    "cryptoTools/gsl/gsl_util",
                    "cryptoTools/gsl/gsl_byte",
                    "cryptoTools/gsl/gsl_algorithm",
                    "cryptoTools/gsl/gsl",
                    "cryptoTools/gsl/multi_span",
                    "cryptoTools/gsl/string_span",
                    "cryptoTools/gsl/gls-lite.hpp",
                ],
    includes = [".", ":cryptoTools_config_h"],
    copts = ["-I@tookit_relic//:relic/include -std=c++14 -O0 -g -ggdb -rdynamic -maes -msse2 -msse3 -msse4.1 -mpclmul -DENABLE_CIRCUITS=ON -DENABLE_RELIC=ON -DENABLE_BOOST=ON -DENABLE_SSE=ON -DRAND=HASHD -DBoost_USE_MULTITHREADED=ON"],
    linkopts = ["-pthread"],
    linkstatic = True,
    deps = [
                #":boost_fiber",
                "@toolkit_relic//:relic",
        ],
)

cc_library(
    name = "tests_cryptoTools",
    srcs = glob(
                ["tests_cryptoTools/**/*.cpp"],
        ),
    hdrs = glob(
                ["tests_cryptoTools/**/*.h"],
        ),
    copts = ["-std=c++14 -O0 -g -ggdb -rdynamic -maes -msse2 -msse3 -msse4.1 -mpclmul -DENABLE_CIRCUITS=ON -DENABLE_RELIC=ON -DENABLE_BOOST=ON -DBoost_USE_MULTITHREADED=ON"],
    linkopts = ["-pthread"],
    linkstatic = True,
    deps = [
                ":cryptoTools",
                "@toolkit_relic//:relic",
        ],
)

cc_library(
    name = "lib_frontend_cryptoTools",
    srcs = glob([
                    "frontend_cryptoTools/**/*.cpp"

                 ],),
    hdrs = glob([
                    "frontend_cryptoTools/**/*.h"
                ],),
    copts = ["-std=c++14 -O0 -g -ggdb -rdynamic -maes -msse2 -msse3 -msse4.1 -mpclmul -DENABLE_CIRCUITS=ON -DENABLE_RELIC=ON -DENABLE_BOOST=ON -DENABLE_SSE=ON -DBoost_USE_MULTITHREADED=ON -DENABLE_FULL_GSL=ON -DENABLE_CPP_14=ON"],
    linkopts = ["-pthread -lstdc++"],
    linkstatic = True,
    deps = [
                ":cryptoTools",
                ":tests_cryptoTools",
                "@toolkit_relic//:relic",
        ],
)

cc_binary(
    name = "frontend_cryptoTools",
    srcs = glob(
                ["frontend_cryptoTools/**/*.cpp"], ["frontend_cryptoTools/**/*.h"],
        ),
    includes = [".", "Tutorials/Network.h"],
    copts = ["-std=c++14 -O0 -g -ggdb -rdynamic -maes -msse2 -msse3 -msse4.1 -mpclmul -DENABLE_CIRCUITS=ON -DENABLE_RELIC=ON -DENABLE_BOOST=ON -DENABLE_SSE=ON"],
    linkopts = ["-pthread -lstdc++"],
    linkstatic = False,
    deps = [
                ":lib_frontend_cryptoTools",
                ":cryptoTools",
                ":tests_cryptoTools",
                "@toolkit_relic//:relic",
        ],
)
