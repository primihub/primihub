config_setting(
  name = "darwin_arm64",
  values = {
    "cpu": "darwin_arm64"
  }
)


config_setting(
  name = "darwin_x86_64",
  values = {
    "cpu": "darwin_x86_64"
  }
)

cc_library(
    name = "crypto",
    hdrs = glob(["include/openssl/*.h"]) + ["include/openssl/opensslconf.h"],
    srcs = ["libcrypto.a"],
    includes = ["include"],
    linkopts = select({
        ":darwin_arm64": [],
        "//conditions:default": ["-lpthread", "-ldl"],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "openssl",
    deps = [":crypto"],
    hdrs = glob(["include/openssl/*.h"]) + ["include/openssl/opensslconf.h"],
    srcs = ["libssl.a"],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

genrule(
       name = "openssl-build",
       srcs = glob(
           ["**/*"],
           exclude = ["bazel-*"],
       ),
       outs = [
           "libcrypto.a",
           "libssl.a",
           "include/openssl/opensslconf.h",
       ],
       cmd = """
           CONFIG_LOG=$$(mktemp)
           MAKE_LOG=$$(mktemp)
           OPENSSL_ROOT=$$(dirname $(location config))
           pushd $$OPENSSL_ROOT > /dev/null
               if ! ./config > $$CONFIG_LOG; then
                   cat $$CONFIG_LOG
               fi
               if ! make -j 4 > $$MAKE_LOG; then
                   cat $$MAKE_LOG
               fi
           popd > /dev/null
           cp $$OPENSSL_ROOT/libcrypto.a $(location libcrypto.a)
           cp $$OPENSSL_ROOT/libssl.a $(location libssl.a)
           cp $$OPENSSL_ROOT/include/openssl/opensslconf.h $(location include/openssl/opensslconf.h)
       """,
   )