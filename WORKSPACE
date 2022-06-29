load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")


_ALL_CONTENT = """\
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
"""


http_archive(
    name = "rules_foreign_cc",
    sha256 = "6041f1374ff32ba711564374ad8e007aef77f71561a7ce784123b9b4b88614fc",
    strip_prefix = "rules_foreign_cc-0.8.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.8.0.tar.gz",
)


load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
    name = "openssl",
    url = "https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1o.tar.gz",
    #sha256 = "f56dd7d81ce8d3e395f83285bd700a1098ed5a4cb0a81ce9522e41e6db7e0389",
    strip_prefix = "openssl-OpenSSL_1_1_1o",
    build_file = "//bazel:openssl.BUILD",
)

git_repository(
    name = "com_github_nelhage_rules_boost",
    # commit = "1e3a69bf2d5cd10c34b74f066054cd335d033d71",
    branch = "master",
    remote = "https://github.com/primihub/rules_boost.git",
    # shallow_since = "1591047380 -0700",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

new_git_repository(
    name = "toolkit_relic",
    build_file = "//bazel:BUILD.relic",
    remote = "https://github.com/relic-toolkit/relic.git",
    commit = "3f616ad64c3e63039277b8c90915607b6a2c504c",
    shallow_since = "1581106153 -0800",
)


http_archive(
  name="eigen",
  build_file_content = _ALL_CONTENT,
  strip_prefix = "eigen-3.4",
  urls=["https://gitlab.com/libeigen/eigen/-/archive/3.4/eigen-3.4.tar.bz2"],
  sha256 = "a6f7aaa7b19c289dfeb33281e1954f19bf2ba1c6cae2c182354d820f535abef8",
)

new_git_repository(
    name = "lib_function2",
    build_file = "//bazel:BUILD.function2",
    remote = "https://github.com/Naios/function2.git",
    commit = "b8cf935d096a87a645534e5c1015ee80960fe4de",
    shallow_since = "1616573746 +0100",
)


new_git_repository(
    name = "arrow",
    build_file = "//bazel:BUILD.arrow",
    patch_cmds = [
        # TODO: Remove the fowllowing once arrow issue is resolved.
        """sed -i.bak 's/type_traits/std::max<int16_t>(sizeof(int16_t), type_traits/g' cpp/src/parquet/column_reader.cc""",
        """sed -i.bak 's/value_byte_size/value_byte_size)/g' cpp/src/parquet/column_reader.cc""",
    ],
    branch = "release-4.0.0",
    remote = "https://github.com/primihub/arrow.git",       
)


# grpc with openssl
git_repository(
    name = "com_github_grpc_grpc",
    remote = "https://github.com/primihub/grpc.git",
    commit = "8838117b07a4faac3a6baaf645a490987b2a12b1",
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

# Includes boringssl, and other dependencies.
grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

# Loads transitive dependencies of GRPC.
grpc_extra_deps()

http_archive(
    name = "com_github_glog_glog",
    # sha256 = "cbba86b5a63063999e0fc86de620a3ad22d6fd2aa5948bff4995dcd851074a0b",
    strip_prefix = "glog-c8f8135a5720aee7de8328b42e4c43f8aa2e60aa",
    urls = ["https://github.com/google/glog/archive/c8f8135a5720aee7de8328b42e4c43f8aa2e60aa.zip"],
)

http_archive(
    name = "com_github_google_flatbuffers",
    url = "https://github.com/google/flatbuffers/archive/refs/tags/v2.0.0.tar.gz",
    strip_prefix = "flatbuffers-2.0.0",
    sha256 = "9ddb9031798f4f8754d00fca2f1a68ecf9d0f83dfac7239af1311e4fd9a565c4",
)

# gflags Needed for glog
http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = [
        "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
    ],
)

# Abseil C++ libraries
git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    commit = "0f3bb466b868b523cf1dc9b2aaaed65c77b28862",
    shallow_since = "1603283562 -0400",
)

# googletest
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/release-1.10.0.zip"],
    sha256 = "94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91",
    strip_prefix = "googletest-release-1.10.0",
)

http_archive(
    name = "bazel_common",
    url = "https://github.com/google/bazel-common/archive/refs/heads/master.zip",
    strip_prefix = "bazel-common-master",
    sha256 = "b7a8e1a4ad843df69c9714377f023276cd15c3b706a46b6e5a1dc7e101fec419",
)

http_archive(
    name = "bazel_skylib",
    strip_prefix = None,
    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.2/bazel-skylib-1.0.2.tar.gz",
    sha256 = "97e70364e9249702246c0e9444bccdc4b847bed1eb03c5a3ece4f83dfe6abc44",
)

load("@bazel_skylib//lib:versions.bzl", "versions")

versions.check(minimum_bazel_version = "5.0.0")

# json
http_archive(
    name = "nlohmann_json",
    strip_prefix = "json-3.9.1",
    urls = ["https://github.com/nlohmann/json/archive/v3.9.1.tar.gz"],
    sha256 = "4cf0df69731494668bdd6460ed8cb269b68de9c19ad8c27abc24cd72605b2d5b",
    build_file = "//bazel:BUILD.nlohmann_json",
)



# pybind11 , bazel ref:https://github.com/pybind/pybind11_bazel
# _PYBIND11_COMMIT = "72cbbf1fbc830e487e3012862b7b720001b70672"
http_archive(
  name = "pybind11_bazel",
  strip_prefix = "pybind11_bazel-72cbbf1fbc830e487e3012862b7b720001b70672",
  urls = [
        "https://github.com/pybind/pybind11_bazel/archive/72cbbf1fbc830e487e3012862b7b720001b70672.zip",
        "https://primihub.oss-cn-beijing.aliyuncs.com/pybind11_bazel-72cbbf1fbc830e487e3012862b7b720001b70672.zip"
        ],
)

# We still require the pybind library.
http_archive(
  name = "pybind11",
  build_file = "@pybind11_bazel//:pybind11.BUILD",
  strip_prefix = "pybind11-2.9.2",
  urls = ["https://github.com/pybind/pybind11/archive/refs/tags/v2.9.2.tar.gz"],
)
load("@pybind11_bazel//:python_configure.bzl", "python_configure")
python_configure(name = "local_config_python")


# ======== arrow dependencies  start ========

http_archive(
    name = "brotli",
    build_file = "//bazel:brotli.BUILD",
    sha256 = "4c61bfb0faca87219ea587326c467b95acb25555b53d1a421ffa3c8a9296ee2c",
    strip_prefix = "brotli-1.0.7",
    urls = [
        "https://storage.googleapis.com/mirror.tensorflow.org/github.com/google/brotli/archive/v1.0.7.tar.gz",
        "https://github.com/google/brotli/archive/refs/tags/v1.0.7.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/brotli-1.0.7.tar.gz"
    ],
)


http_archive(
    name = "bzip2",
    build_file = "//bazel:bzip2.BUILD",
    sha256 = "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269",
    strip_prefix = "bzip2-1.0.8",
    urls = [
        "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
        "https://fossies.org/linux/misc/bzip2-1.0.8.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/bzip2-1.0.8.tar.gz"
    ],
)


http_archive(
    name = "double-conversion",
    sha256 = "a63ecb93182134ba4293fd5f22d6e08ca417caafa244afaa751cbfddf6415b13",
    strip_prefix = "double-conversion-3.1.5",
    urls = [
        "https://storage.googleapis.com/mirror.tensorflow.org/github.com/google/double-conversion/archive/v3.1.5.tar.gz",
        "https://github.com/google/double-conversion/archive/v3.1.5.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/double-conversion-3.1.5.tar.gz",
        
    ],
)

http_archive(
    name = "lz4",
    build_file = "//bazel:lz4.BUILD",
    patch_cmds = [
        """sed -i.bak 's/__attribute__ ((__visibility__ ("default")))//g' lib/lz4frame.h """,
    ],
    sha256 = "658ba6191fa44c92280d4aa2c271b0f4fbc0e34d249578dd05e50e76d0e5efcc",
    strip_prefix = "lz4-1.9.2",
    urls = [
        # "https://storage.googleapis.com/mirror.tensorflow.org/github.com/lz4/lz4/archive/v1.9.2.tar.gz",
        "https://github.com/lz4/lz4/archive/v1.9.2.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/lz4-1.9.2.tar.gz"
    ],
)

http_archive(
    name = "rapidjson",
    build_file = "//bazel:rapidjson.BUILD",
    sha256 = "30bd2c428216e50400d493b38ca33a25efb1dd65f79dfc614ab0c957a3ac2c28",
    strip_prefix = "rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e",
    urls = [
        "https://github.com/miloyip/rapidjson/archive/418331e99f859f00bdc8306f69eba67e8693c55e.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e.tar.gz"
    ],
)

# Note: snappy is placed earlier as tensorflow's snappy does not include snappy-c
http_archive(
    name = "snappy",
    build_file = "//bazel:snappy.BUILD",
    sha256 = "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f",
    strip_prefix = "snappy-1.1.8",
    urls = [
        "https://github.com/google/snappy/archive/1.1.8.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/snappy-1.1.8.tar.gz"
    ],
)

http_archive(
    name = "thrift",
    build_file = "//bazel:thrift.BUILD",
    sha256 = "5da60088e60984f4f0801deeea628d193c33cec621e78c8a43a5d8c4055f7ad9",
    strip_prefix = "thrift-0.13.0",
    urls = [
        "https://github.com/apache/thrift/archive/v0.13.0.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/thrift-0.13.0.tar.gz"
    ],
)

http_archive(
    name = "xsimd",
    build_file = "//bazel:xsimd.BUILD",
    sha256 = "45337317c7f238fe0d64bb5d5418d264a427efc53400ddf8e6a964b6bcb31ce9",
    strip_prefix = "xsimd-7.5.0",
    urls = [
        "https://github.com/xtensor-stack/xsimd/archive/refs/tags/7.5.0.tar.gz",
    ],
)

http_archive(
    name = "zlib",
    build_file = "//bazel:zlib.BUILD",
    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
    strip_prefix = "zlib-1.2.11",
    urls = [
        "https://zlib.net/zlib-1.2.11.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/zlib-1.2.11.tar.gz"
    ],
)

http_archive(
    name = "zstd",
    build_file = "//bazel:zstd.BUILD",
    sha256 = "a364f5162c7d1a455cc915e8e3cf5f4bd8b75d09bc0f53965b0c9ca1383c52c8",
    strip_prefix = "zstd-1.4.4",
    urls = [
        "https://github.com/facebook/zstd/archive/v1.4.4.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/zstd-1.4.4.tar.gz"
    ],
)

http_archive(
    name = "xz",
    build_file = "//bazel:xz.BUILD",
    sha256 = "0d2b89629f13dd1a0602810529327195eff5f62a0142ccd65b903bc16a4ac78a",
    strip_prefix = "xz-5.2.5",
    urls = [
        "https://github.com/xz-mirror/xz/archive/v5.2.5.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/xz-5.2.5.tar.gz"
    ],
)

http_archive(
    name = "io_opentelemetry_cpp",
    # sha256 = "<sha256>",
    strip_prefix = "opentelemetry-cpp-1.0.1",
    urls = [
        "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v1.0.1.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/opentelemetry-cpp-1.0.1.tar.gz"
    ],
)

# Load OpenTelemetry dependencies after load.
load("@io_opentelemetry_cpp//bazel:repository.bzl", "opentelemetry_cpp_deps")

opentelemetry_cpp_deps()


http_archive(
    name = "com_google_protobuf",
    # sha256 = "<sha256>",
    # strip_prefix = "opentelemetry-cpp-1.0.1",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.20.0.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/protobuf-3.20.0.tar.gz"
    ],
)


git_repository(
    name = "cares-bazel",
    branch = "master",
    remote = "https://github.com/hobo0cn/cares-bazel.git",
    patch_cmds = [
        "git submodule update --init --recursive",
    ],
)

# fmt bazle, ref: https://fossies.org/linux/fmt/support/bazel/README.md
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

new_git_repository(
    name = "fmt",
    build_file = "//bazel:fmt.BUILD",
    remote = "https://github.com/fmtlib/fmt.git",
    tag = "6.1.2",
)


new_git_repository(
    name = "libp2p",
    build_file = "//bazel:libp2p.BUILD",
    remote = "https://github.com/primihub/cpp-libp2p.git",
    branch="master",
)

# soralog , need by libp2p
# TODO need change to glog
new_git_repository(
    name = "com_openmpc_soralog",
    build_file = "//bazel:soralog.BUILD",
    remote = "https://github.com/primihub/soralog.git",
    branch="master",
)

# sqlite, need by libp2p
http_archive(
    name = "com_github_soramitsu_sqlite",
    build_file = "//bazel:sqlite.BUILD",
    strip_prefix = "libp2p-sqlite-modern-cpp-3.2",
    urls = [
        "https://github.com/soramitsu/libp2p-sqlite-modern-cpp/archive/refs/tags/v3.2.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/libp2p-sqlite-modern-cpp-3.2.tar.gz"],
)

#yaml-cpp, need by libp2p
git_repository(
    name = "com_github_jbeder_yaml_cpp",
    remote = "https://github.com/jbeder/yaml-cpp.git",
    tag="yaml-cpp-0.7.0",
)
# hat_trie , need by libp2p
new_git_repository(
    name = "com_github_masterjedy_hat_hrie",
    build_file = "//bazel:hat_trie.BUILD",
    remote = "https://github.com/masterjedy/hat-trie.git",
    branch="master",
)

# boost di, used by libp2p
http_archive(
    name = "com_github_masterjedy_di",
    build_file = "//bazel:di.BUILD",
    strip_prefix = "di-1.1.0",
    urls = [
        "https://github.com/boost-ext/di/archive/refs/tags/v1.1.0.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/di-1.1.0.tar.gz"
    ],
)

# seal 3.3.2, used by crypTFlow2
http_archive(
  name = "com_microsoft_seal_3.3.2",
  sha256 = "7e29c36c81f2061b0680002fbb869cb9756ca7896b768a1f5d97d5dd08fc43a2",
  build_file = "//bazel:BUILD.seal",
  strip_prefix = "SEAL-3.3.2/native/src/",
  urls = ["https://github.com/microsoft/SEAL/archive/refs/tags/v3.3.2.zip"],
)

http_archive(
    name = "com_github_gmp",
    build_file = "//bazel:gmp.BUILD",
    #sha256 = "87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912",
    strip_prefix = "gmp-6.2.1",
    urls = ["https://gmplib.org/download/gmp/gmp-6.2.1.tar.xz"],
)

#PSI
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
   name = "org_openmined_psi",
   remote = "https://github.com/primihub/PSI.git",
   branch = "master",
   init_submodules = True,
)

load("@org_openmined_psi//private_set_intersection:preload.bzl", "psi_preload")

psi_preload()

load("@org_openmined_psi//private_set_intersection:deps.bzl", "psi_deps")

psi_deps()


load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")


git_repository(
   name = "org_openmined_pir",
   remote = "https://github.com/primihub/PIR.git",
   branch = "master",
   init_submodules = True,
)

load("@org_openmined_pir//pir:preload.bzl", "pir_preload")

pir_preload()

load("@org_openmined_pir//pir:deps.bzl", "pir_deps")

pir_deps()


# leveldb as local kv store
load("//3rdparty/bazel-rules-leveldb/bazel:repos.bzl", leveldb_repos="repos")
leveldb_repos()

load("//3rdparty/bazel-rules-leveldb/bazel:deps.bzl", leveldb_deps="deps")
leveldb_deps()