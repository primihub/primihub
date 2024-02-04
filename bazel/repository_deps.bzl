load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def primihub_deps_cn():
  # Abseil C++ libraries
  if "com_google_absl" not in native.existing_rules():
    http_archive(
      name = "com_google_absl",
      sha256 = "1764491a199eb9325b177126547f03d244f86b4ff28f16f206c7b3e7e4f777ec",
      strip_prefix = "abseil-cpp-278e0a071885a22dcd2fd1b5576cc44757299343",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/google_absl_278e0a071885a22dcd2fd1b5576cc44757299343.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/abseil/abseil-cpp/archive/278e0a071885a22dcd2fd1b5576cc44757299343.tar.gz",
        "https://github.com/abseil/abseil-cpp/archive/278e0a071885a22dcd2fd1b5576cc44757299343.tar.gz",
      ],
    )

  if "build_bazel_rules_apple" not in native.existing_rules():
    http_archive(
      name = "build_bazel_rules_apple",
      sha256 = "0052d452af7742c8f3a4e0929763388a66403de363775db7e90adecb2ba4944b",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_apple.0.31.3.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/bazelbuild/rules_apple/releases/download/0.31.3/rules_apple.0.31.3.tar.gz",
        "https://github.com/bazelbuild/rules_apple/releases/download/0.31.3/rules_apple.0.31.3.tar.gz",
      ],
    )
  if "build_bazel_rules_swift" not in native.existing_rules():
    http_archive(
      name = "build_bazel_rules_swift",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_swift.0.21.0.tar.gz",
        # "https://github.com/bazelbuild/rules_swift/releases/download/0.21.0/rules_swift.0.21.0.tar.gz",
      ],
      sha256 = "8407fa0fd04a7ce1d6bb95e90b216404466f809eda459c23cb57b5fa1ef9d639",
    )

  if "io_bazel_rules_rust" not in native.existing_rules():
    http_archive(
      name = "io_bazel_rules_rust",
      sha256 = "75b29ba47ff4ef81f48574d1109bb6612788212524afe99e21467c71c980baa5",
      strip_prefix = "rules_rust-8cfa049d478ad33e407d572e260e912bdb31796a",
      urls = [
        # Master branch as of 25/07/2020
        # "https://github.com/bazelbuild/rules_rust/archive/8cfa049d478ad33e407d572e260e912bdb31796a.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/rules_rust-8cfa049d478ad33e407d572e260e912bdb31796a.tar.gz",
      ],
    )
  if "io_opentelemetry_cpp" not in native.existing_rules():
    http_archive(
      name = "io_opentelemetry_cpp",
      sha256 = "0cddc5a582b52d9234bd261f1fd218d4cd136ed8c79e7af99034d1dc7da8c33b",
      strip_prefix = "opentelemetry-cpp-1.0.1",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/opentelemetry-cpp-1.0.1.tar.gz"
      ],
    )
  if "build_bazel_apple_support" not in native.existing_rules():
    http_archive(
      name = "build_bazel_apple_support",
      sha256 = "76df040ade90836ff5543888d64616e7ba6c3a7b33b916aa3a4b68f342d1b447",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/apple_support.0.11.0.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/bazelbuild/apple_support/releases/download/0.11.0/apple_support.0.11.0.tar.gz",
        "https://github.com/bazelbuild/apple_support/releases/download/0.11.0/apple_support.0.11.0.tar.gz",
      ],
    )
  if "build_bazel_apple_support" not in native.existing_rules():
    http_archive(
      name = "openssl",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/openssl-OpenSSL_1_1_1o.tar.gz",
      ],
      #sha256 = "f56dd7d81ce8d3e395f83285bd700a1098ed5a4cb0a81ce9522e41e6db7e0389",
      strip_prefix = "openssl-OpenSSL_1_1_1o",
      build_file = "@com_github_bazel_rules_3rdparty//:openssl.BUILD",
    )

  if "com_github_glog_glog" not in native.existing_rules():
    http_archive(
      name = "com_github_glog_glog",
      # sha256 = "cbba86b5a63063999e0fc86de620a3ad22d6fd2aa5948bff4995dcd851074a0b",
      strip_prefix = "glog-0.6.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/glog-0.6.0.zip"
      ],
    )
  if "upb" not in native.existing_rules():
    http_archive(
      name = "upb",
      #sha256 = "6a5f67874af66b239b709c572ac1a5a00fdb1b29beaf13c3e6f79b1ba10dc7c4",
      strip_prefix = "upb-0.0.2-dp",
      urls = [
          "https://primihub.oss-cn-beijing.aliyuncs.com/tools/v0.0.2-dp.tar.gz",
          "https://github.com/primihub/upb/archive/refs/tags/v0.0.2-dp.tar.gz",
      ],
    )
  if "snappy" not in native.existing_rules():
    # Note: snappy is placed earlier as tensorflow's snappy does not include snappy-c
    http_archive(
      name = "snappy",
      build_file = "@com_github_bazel_rules_3rdparty//:snappy.BUILD",
      sha256 = "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f",
      strip_prefix = "snappy-1.1.8",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/snappy-1.1.8.tar.gz"
      ],
    )
  if "com_github_google_flatbuffers" not in native.existing_rules():
    http_archive(
      name = "com_github_google_flatbuffers",
      url = "https://primihub.oss-cn-beijing.aliyuncs.com/tools/flatbuffers-2.0.0.tar.gz",
      strip_prefix = "flatbuffers-2.0.0",
      sha256 = "9ddb9031798f4f8754d00fca2f1a68ecf9d0f83dfac7239af1311e4fd9a565c4",
    )
  if "com_github_redis_hiredis" not in native.existing_rules():
    http_archive(
      name = "com_github_redis_hiredis",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.hiredis",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/hiredis-392de5d7f97353485df1237872cb682842e8d83f.tar.gz"
      ],
      sha256 = "2101650d39a8f13293f263e9da242d2c6dee0cda08d343b2939ffe3d95cf3b8b",
      strip_prefix = "hiredis-392de5d7f97353485df1237872cb682842e8d83f"
    )
  if "rules_proto" not in native.existing_rules():
    http_archive(
      name = "rules_proto",
      sha256 = "a4382f78723af788f0bc19fd4c8411f44ffe0a72723670a34692ffad56ada3ac",
      strip_prefix = "rules_proto-f7a30f6f80006b591fa7c437fe5a951eb10bcbcf",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_proto_f7a30f6f80006b591fa7c437fe5a951eb10bcbcf.zip",
        "https://github.com/bazelbuild/rules_proto/archive/f7a30f6f80006b591fa7c437fe5a951eb10bcbcf.zip"
      ],
    )
  if "bazel_skylib" not in native.existing_rules():
    http_archive(
      name = "bazel_skylib",
      # `main` as of 2021-10-27
      # Release request: https://github.com/bazelbuild/bazel-skylib/issues/336
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/bazel-skylib-6e30a77347071ab22ce346b6d20cf8912919f644.zip",
        "https://github.com/bazelbuild/bazel-skylib/archive/6e30a77347071ab22ce346b6d20cf8912919f644.zip",
      ],
      strip_prefix = "bazel-skylib-6e30a77347071ab22ce346b6d20cf8912919f644",
      sha256 = "247361e64b2a85b40cb45b9c071e42433467c6c87546270cbe2672eb9f317b5a",
    )

  if "double-conversion" not in native.existing_rules():
    http_archive(
      name = "double-conversion",
      sha256 = "a63ecb93182134ba4293fd5f22d6e08ca417caafa244afaa751cbfddf6415b13",
      strip_prefix = "double-conversion-3.1.5",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/double-conversion-3.1.5.tar.gz",
      ],
    )

  if "boost" not in native.existing_rules():
    http_archive(
      name = "boost",
      build_file = "@com_github_nelhage_rules_boost//:BUILD.boost",
      patches = ["@com_github_nelhage_rules_boost//:0001-json-array-erase-relocate.patch"],
      patch_cmds = ["rm -f doc/pdf/BUILD"],
      patch_cmds_win = ["Remove-Item -Force doc/pdf/BUILD"],
      sha256 = "273f1be93238a068aba4f9735a4a2b003019af067b9c183ed227780b8f36062c",
      strip_prefix = "boost_1_79_0",
      urls = [
          #"https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.gz",
          "https://primihub.oss-cn-beijing.aliyuncs.com/boost_1_79_0.tar.gz",
      ],
    )

  if "eigen" not in native.existing_rules():
    http_archive(
      name = "eigen",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.eigen",
      strip_prefix = "eigen-3.4",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/eigen-3.4.tar.bz2",
      ],
      sha256 = "a6f7aaa7b19c289dfeb33281e1954f19bf2ba1c6cae2c182354d820f535abef8",
    )
  if "rapidjson" not in native.existing_rules():
    http_archive(
      name = "rapidjson",
      build_file = "@com_github_bazel_rules_3rdparty//:rapidjson.BUILD",
      sha256 = "30bd2c428216e50400d493b38ca33a25efb1dd65f79dfc614ab0c957a3ac2c28",
      strip_prefix = "rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e.tar.gz"
      ],
    )
  if "com_github_google_leveldb" not in native.existing_rules():
    http_archive(
      name = "com_github_google_leveldb",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.leveldb",
      sha256 = "a6fa7eebd11de709c46bf1501600ed98bf95439d6967963606cc964931ce906f",
      strip_prefix = "leveldb-1.23",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/leveldb-1.23.zip"
      ],
    )
  if "com_googlesource_code_re2" not in native.existing_rules():
    http_archive(
      name = "com_googlesource_code_re2",
      sha256 = "319a58a58d8af295db97dfeecc4e250179c5966beaa2d842a82f0a013b6a239b",
      # Release 2021-09-01
      strip_prefix = "re2-8e08f47b11b413302749c0d8b17a1c94777495d5",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/re2-8e08f47b11b413302749c0d8b17a1c94777495d5.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/google/re2/archive/8e08f47b11b413302749c0d8b17a1c94777495d5.tar.gz",
        "https://github.com/google/re2/archive/8e08f47b11b413302749c0d8b17a1c94777495d5.tar.gz",
      ],
    )

  if "org_bzip_bzip2" not in native.existing_rules():
    http_archive(
      name = "org_bzip_bzip2",
      build_file = "@com_github_nelhage_rules_boost//:BUILD.bzip2",
      sha256 = "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269",
      strip_prefix = "bzip2-1.0.8",
      urls = [
          "https://primihub.oss-cn-beijing.aliyuncs.com/tools/bzip2-1.0.8.tar.gz",
          "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
      ],
    )
  if "com_github_gflags_gflags" not in native.existing_rules():
    http_archive(
      # gflags Needed for glog
      name = "com_github_gflags_gflags",
      sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
      strip_prefix = "gflags-2.2.2",
      urls = [
          "https://primihub.oss-cn-beijing.aliyuncs.com/tools/gflags-2.2.2.tar.gz",
      ],
    )
  if not native.existing_rule("rules_pkg"):
    http_archive(
      name = "rules_pkg",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_pkg-0.5.1.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.5.1/rules_pkg-0.5.1.tar.gz",
        "https://github.com/bazelbuild/rules_pkg/releases/download/0.5.1/rules_pkg-0.5.1.tar.gz",
      ],
      sha256 = "a89e203d3cf264e564fcb96b6e06dd70bc0557356eb48400ce4b5d97c2c3720d",
    )

  if "rules_cc" not in native.existing_rules():
    http_archive(
      name = "rules_cc",
      sha256 = "35f2fb4ea0b3e61ad64a369de284e4fbbdcdba71836a5555abb5e194cf119509",
      strip_prefix = "rules_cc-624b5d59dfb45672d4239422fa1e3de1822ee110",
      urls = [
        #2019-08-15
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_cc-624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
        "https://github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
      ],
    )
  if "curl" not in native.existing_rules():
    http_archive(
      name = "curl",
      build_file = "@com_github_bazel_rules_3rdparty//:curl.BUILD",
      sha256 = "ba98332752257b47b9dea6d8c0ad25ec1745c20424f1dd3ff2c99ab59e97cf91",
      strip_prefix = "curl-7.73.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/curl-7.73.0.tar.gz",
        # "https://curl.haxx.se/download/curl-7.73.0.tar.gz"
      ],
    )
  if "zlib" not in native.existing_rules():
    http_archive(
      name = "zlib",
      build_file = "@com_github_bazel_rules_3rdparty//:zlib.BUILD",
      sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
      strip_prefix = "zlib-1.2.11",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/zlib-1.2.11.tar.gz"
      ],
    )
  if "xsimd" not in native.existing_rules():
    http_archive(
      name = "xsimd",
      build_file = "@com_github_bazel_rules_3rdparty//:xsimd.BUILD",
      sha256 = "45337317c7f238fe0d64bb5d5418d264a427efc53400ddf8e6a964b6bcb31ce9",
      strip_prefix = "xsimd-7.5.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/xsimd-7.5.0.tar.gz",
      ],
    )
  if "rules_proto_grpc" not in native.existing_rules():
    http_archive(
      name = "rules_proto_grpc",
      urls = [
        #"https://github.com/rules-proto-grpc/rules_proto_grpc/archive/2.0.0.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/rules_proto_grpc-2.0.0.tar.gz"
      ],
      sha256 = "d771584bbff98698e7cb3cb31c132ee206a972569f4dc8b65acbdd934d156b33",
      strip_prefix = "rules_proto_grpc-2.0.0",
    )
  if "rules_proto_grpc" not in native.existing_rules():
    http_archive(
      name = "rules_foreign_cc",
      sha256 = "484fc0e14856b9f7434072bc2662488b3fe84d7798a5b7c92a1feb1a0fa8d088",
      strip_prefix = "rules_foreign_cc-0.8.0",
      url = "https://primihub.oss-cn-beijing.aliyuncs.com/tools/rules_foreign_cc_cn-0.8.0.tar.gz",
    )

  if "bazel_gazelle" not in native.existing_rules():
    http_archive(
      name = "bazel_gazelle",
      sha256 = "d987004a72697334a095bbaa18d615804a28280201a50ed6c234c40ccc41e493",
      strip_prefix = "bazel-gazelle-0.19.1",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/bazel-gazelle-0.19.1.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/bazelbuild/bazel-gazelle/archive/v0.19.1.tar.gz",
        "https://github.com/bazelbuild/bazel-gazelle/archive/v0.19.1.tar.gz",
      ],
    )
  if "bazel_gazelle" not in native.existing_rules():
    http_archive(
      name = "google_sparsehash",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.sparsehash",
      strip_prefix = "sparsehash-master",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/sparsehash-master.zip"
      ],
    )
  if "io_bazel_rules_go" not in native.existing_rules():
    http_archive(
      name = "io_bazel_rules_go",
      sha256 = "dbf5a9ef855684f84cac2e7ae7886c5a001d4f66ae23f6904da0faaaef0d61fc",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_go-v0.24.11.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.24.11/rules_go-v0.24.11.tar.gz",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.24.11/rules_go-v0.24.11.tar.gz",
      ],
    )

  if "brotli" not in native.existing_rules():
    http_archive(
      name = "brotli",
      build_file = "@com_github_bazel_rules_3rdparty//:brotli.BUILD",
      sha256 = "4c61bfb0faca87219ea587326c467b95acb25555b53d1a421ffa3c8a9296ee2c",
      strip_prefix = "brotli-1.0.7",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/brotli-1.0.7.tar.gz"
      ],
    )
  if "private_join_and_compute" not in native.existing_rules():
    #TODO revert to the upstream repository when the https://github.com/google/private-join-and-compute/pull/21 is merged
    http_archive(
      name = "private_join_and_compute",
      # sha256 = "219f7cff49841901f8d88a7f84c9c8a61e69b5eb308a8535835743093eb4b595",
      strip_prefix = "private-join-and-compute-master",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/private-join-and-compute-master.zip",
        "https://gitlab.primihub.com/openmpc/private-join-and-compute/-/archive/master/private-join-and-compute-master.zip",
      ]
    )

  if "nlohmann_json" not in native.existing_rules():
    http_archive(
      name = "nlohmann_json",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.nlohmann_json",
      strip_prefix = "json-3.9.1",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/json-3.9.1.tar.gz"
      ],
      sha256 = "4cf0df69731494668bdd6460ed8cb269b68de9c19ad8c27abc24cd72605b2d5b",
    )

  if "platforms" not in native.existing_rules():
    http_archive(
      name = "platforms",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/platforms-0.0.6.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
        "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
      ],
      sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
    )

  if "com_github_facebook_zstd" not in native.existing_rules():
    http_archive(
      name = "com_github_facebook_zstd",
      build_file = "@com_github_nelhage_rules_boost//:BUILD.zstd",
      sha256 = "e28b2f2ed5710ea0d3a1ecac3f6a947a016b972b9dd30242369010e5f53d7002",
      strip_prefix = "zstd-1.5.1",
      urls = [
        #"https://github.com/facebook/zstd/releases/download/v1.5.1/zstd-1.5.1.tar.gz",
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/zstd-1.5.1.tar.gz",
      ],
    )

  #support sqlite
  if "com_github_sqlite_wrapper" not in native.existing_rules():
    http_archive(
      name = "com_github_sqlite_wrapper",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.sqlite",
      sha256 = "57f91ed44ef205fe97b8c6586002fe6031cd02771d1c5d8415d9c515ad1532d1",
      strip_prefix = "SQLiteCpp-3.2.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/SQLiteCpp-3.2.0.tar.gz",
      ]
    )
  if "rules_python" not in native.existing_rules():
    http_archive(
      name = "rules_python",
      strip_prefix = "rules_python-4b84ad270387a7c439ebdccfd530e2339601ef27",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/rules_python-4b84ad270387a7c439ebdccfd530e2339601ef27.tar.gz",
      ],
      sha256 = "e5470e92a18aa51830db99a4d9c492cc613761d5bdb7131c04bd92b9834380f6",
    )

  if "com_github_grpc_grpc" not in native.existing_rules():
    http_archive(
      name="com_github_grpc_grpc",
      strip_prefix = "grpc-1.42.x.openssl.0",
      urls=[
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/grpc-1.42.x.openssl.v1.tar.gz",
      ],
      sha256 = "5911a187c715d37c059ba8655c9a00afb9a113dbeb7adb4a958136f6f5dac76f",
    )

  if "envoy_api" not in native.existing_rules():
    ENVOY_API_COMMIT_ID = "20b1b5fcee88a20a08b71051a961181839ec7268"
    http_archive(
      name = "envoy_api",
      sha256 = "e89d4dddbadf797dd2700ce45ee8abc82557a934a15fcad82673e7d13213b868",
      strip_prefix = "data-plane-api-%s" % ENVOY_API_COMMIT_ID,
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/data-plane-api-%s.tar.gz" % ENVOY_API_COMMIT_ID,
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/envoyproxy/data-plane-api/archive/%s.tar.gz" % ENVOY_API_COMMIT_ID,
        "https://github.com/envoyproxy/data-plane-api/archive/%s.tar.gz" % ENVOY_API_COMMIT_ID,
      ],
    )

  if "com_google_googleapis" not in native.existing_rules():
    http_archive(
      name = "com_google_googleapis",
      sha256 = "5bb6b0253ccf64b53d6c7249625a7e3f6c3bc6402abd52d3778bfa48258703a0",
      strip_prefix = "googleapis-2f9af297c84c55c8b871ba4495e01ade42476c92",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/googleapis-2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/googleapis/googleapis/archive/2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz",
        "https://github.com/googleapis/googleapis/archive/2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz",
      ],
    )
  if "com_github_cares_cares" not in native.existing_rules():
    http_archive(
      name = "com_github_cares_cares",
      build_file = "@com_github_grpc_grpc//third_party:cares/cares.BUILD",
      sha256 = "e8c2751ddc70fed9dc6f999acd92e232d5846f009ee1674f8aee81f19b2b915a",
      strip_prefix = "c-ares-e982924acee7f7313b4baa4ee5ec000c5e373c30",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/c-ares-e982924acee7f7313b4baa4ee5ec000c5e373c30.tar.gz",
        "https://storage.googleapis.com/grpc-bazel-mirror/github.com/c-ares/c-ares/archive/e982924acee7f7313b4baa4ee5ec000c5e373c30.tar.gz",
        "https://github.com/c-ares/c-ares/archive/e982924acee7f7313b4baa4ee5ec000c5e373c30.tar.gz",
      ],
    )

  if "thrift" not in native.existing_rules():
    http_archive(
      name = "thrift",
      build_file = "@com_github_bazel_rules_3rdparty//:thrift.BUILD",
      sha256 = "5da60088e60984f4f0801deeea628d193c33cec621e78c8a43a5d8c4055f7ad9",
      strip_prefix = "thrift-0.13.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/thrift-0.13.0.tar.gz"
      ],
    )
  if "com_github_microsoft_gsl" not in native.existing_rules():
    http_archive(
      name = "com_github_microsoft_gsl",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.gsl",
      sha256 = "f0e32cb10654fea91ad56bde89170d78cfbf4363ee0b01d8f097de2ba49f6ce9",
      strip_prefix = "GSL-4.0.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/GSL-4.0.0.tar.gz",
      ],
    )

  if "lz4" not in native.existing_rules():
    http_archive(
      name = "lz4",
      build_file = "@com_github_bazel_rules_3rdparty//:lz4.BUILD",
      patch_cmds = [
        """sed -i.bak 's/__attribute__ ((__visibility__ ("default")))//g' lib/lz4frame.h """,
      ],
      sha256 = "658ba6191fa44c92280d4aa2c271b0f4fbc0e34d249578dd05e50e76d0e5efcc",
      strip_prefix = "lz4-1.9.2",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/lz4-1.9.2.tar.gz"
      ],
    )

  #generate uuid
  if "com_github_stduuid" not in native.existing_rules():
    http_archive(
      name = "com_github_stduuid",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.stduuid",
      sha256 = "f554f6a9fe4d852fa217de6ab977afbf3f49e4a1dcb263afd61a94253c4c7a48",
      strip_prefix = "stduuid-1.2.2",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/stduuid-1.2.2.tar.gz",
      ]
    )

  if "pybind11" not in native.existing_rules():
    http_archive(
      name = "pybind11",
      build_file = "@pybind11_bazel//:pybind11.BUILD",
      strip_prefix = "pybind11-2.9.2",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/pybind11-2.9.2.tar.gz"
      ],
      sha256 = "6bd528c4dbe2276635dc787b6b1f2e5316cf6b49ee3e150264e455a0d68d19c1",
    )

  if "rules_java" not in native.existing_rules():
    http_archive(
      name = "rules_java",
      sha256 = "f5a3e477e579231fca27bf202bb0e8fbe4fc6339d63b38ccb87c2760b533d1c3",
      strip_prefix = "rules_java-981f06c3d2bd10225e85209904090eb7b5fb26bd",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/repository_deps/rules_java-981f06c3d2bd10225e85209904090eb7b5fb26bd.tar.gz",
        "https://github.com/bazelbuild/rules_java/archive/981f06c3d2bd10225e85209904090eb7b5fb26bd.tar.gz"
      ],
    )

  if "poco" not in native.existing_rules():
    http_archive(
      name = "poco",
      build_file = "@com_github_bazel_rules_3rdparty//:poco.BUILD",
      sha256 = "71ef96c35fced367d6da74da294510ad2c912563f12cd716ab02b6ed10a733ef",
      strip_prefix = "poco-poco-1.12.4-release",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/poco-poco-1.12.4-release.tar.gz",
      ],
    )

  if "arrow" not in native.existing_rules():
    http_archive(
      name = "arrow",
      build_file = "@com_github_bazel_rules_3rdparty//:BUILD.arrow",
      patch_cmds = [
        # TODO: Remove the fowllowing once arrow issue is resolved.
        """sed -i.bak 's/type_traits/std::max<int16_t>(sizeof(int16_t), type_traits/g' cpp/src/parquet/column_reader.cc""",
        """sed -i.bak 's/value_byte_size/value_byte_size)/g' cpp/src/parquet/column_reader.cc""",
      ],
      strip_prefix = "arrow-4.0.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/tools/arrow_v4.0.0.tar.gz",
      ],
      sha256 = "749e3f5972e9de4319eb5ff6fcf40679702a2b3e9114536aba13e35ebe6e161b",
    )

  if "com_google_protobuf" not in native.existing_rules():
    http_archive(
      name = "com_google_protobuf",
      sha256 = "b07772d38ab07e55eca4d50f4b53da2d998bb221575c60a4f81100242d4b4889",
      strip_prefix = "protobuf-3.20.0",
      urls = [
        "https://primihub.oss-cn-beijing.aliyuncs.com/protobuf-3.20.0.tar.gz"
      ],
    )