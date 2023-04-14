load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

def primihub_extra_deps():
    if "openssl" not in native.existing_rules():       
        http_archive(
            name = "openssl",
            url = "https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1o.tar.gz",
            #sha256 = "f56dd7d81ce8d3e395f83285bd700a1098ed5a4cb0a81ce9522e41e6db7e0389",
            strip_prefix = "openssl-OpenSSL_1_1_1o",
            build_file = "@primihub//bazel:openssl.BUILD",
        )


    # arrow deps start
    if "arrow" not in native.existing_rules():        
        new_git_repository(
            name = "arrow",
            build_file = "@primihub//bazel:BUILD.arrow",
            patch_cmds = [
                # TODO: Remove the fowllowing once arrow issue is resolved.
                """sed -i.bak 's/type_traits/std::max<int16_t>(sizeof(int16_t), type_traits/g' cpp/src/parquet/column_reader.cc""",
                """sed -i.bak 's/value_byte_size/value_byte_size)/g' cpp/src/parquet/column_reader.cc""",
            ],
            branch = "release-4.0.0",
            remote = "https://github.com/primihub/arrow.git",       
        )
    
    if "com_github_jbeder_yaml_cpp" not in native.existing_rules(): 
        #yaml-cpp, need by libp2p
        git_repository(
            name = "com_github_jbeder_yaml_cpp",
            remote = "https://github.com/jbeder/yaml-cpp.git",
            tag="yaml-cpp-0.7.0",
        )

    if "com_github_nelhage_rules_boost" not in native.existing_rules():     
        git_repository(
        name = "com_github_nelhage_rules_boost",
        # commit = "1e3a69bf2d5cd10c34b74f066054cd335d033d71",
        branch = "master",
        remote = "https://github.com/primihub/rules_boost.git",
        # shallow_since = "1591047380 -0700",
        )

    # load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
    # boost_deps()


    if "brotli" not in native.existing_rules():   
        http_archive(
            name = "brotli",
            build_file = "@primihub//bazel:brotli.BUILD",
            sha256 = "4c61bfb0faca87219ea587326c467b95acb25555b53d1a421ffa3c8a9296ee2c",
            strip_prefix = "brotli-1.0.7",
            urls = [
                "https://storage.googleapis.com/mirror.tensorflow.org/github.com/google/brotli/archive/v1.0.7.tar.gz",
                "https://github.com/google/brotli/archive/refs/tags/v1.0.7.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/brotli-1.0.7.tar.gz"
            ],
        )

    if "bzip2" not in native.existing_rules():   
        http_archive(
            name = "bzip2",
            build_file = "@primihub//bazel:bzip2.BUILD",
            sha256 = "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269",
            strip_prefix = "bzip2-1.0.8",
            urls = [
                "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
                "https://fossies.org/linux/misc/bzip2-1.0.8.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/bzip2-1.0.8.tar.gz"
            ],
        )

    if "double-conversion" not in native.existing_rules():   
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
    
    if "lz4" not in native.existing_rules():   
        http_archive(
            name = "lz4",
            build_file = "@primihub//bazel:lz4.BUILD",
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

    if "rapidjson" not in native.existing_rules():   
        http_archive(
            name = "rapidjson",
            build_file = "@primihub//bazel:rapidjson.BUILD",
            sha256 = "30bd2c428216e50400d493b38ca33a25efb1dd65f79dfc614ab0c957a3ac2c28",
            strip_prefix = "rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e",
            urls = [
                "https://github.com/miloyip/rapidjson/archive/418331e99f859f00bdc8306f69eba67e8693c55e.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/rapidjson-418331e99f859f00bdc8306f69eba67e8693c55e.tar.gz"
            ],
        )

    if "snappy" not in native.existing_rules():   
        # Note: snappy is placed earlier as tensorflow's snappy does not include snappy-c
        http_archive(
            name = "snappy",
            build_file = "@primihub//bazel:snappy.BUILD",
            sha256 = "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f",
            strip_prefix = "snappy-1.1.8",
            urls = [
                "https://github.com/google/snappy/archive/1.1.8.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/snappy-1.1.8.tar.gz"
            ],
        )

    if "thrift" not in native.existing_rules():   
        http_archive(
            name = "thrift",
            build_file = "@primihub//bazel:thrift.BUILD",
            sha256 = "5da60088e60984f4f0801deeea628d193c33cec621e78c8a43a5d8c4055f7ad9",
            strip_prefix = "thrift-0.13.0",
            urls = [
                "https://github.com/apache/thrift/archive/v0.13.0.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/thrift-0.13.0.tar.gz"
            ],
        )

    if "xsimd" not in native.existing_rules():   
        http_archive(
            name = "xsimd",
            build_file = "@primihub//bazel:xsimd.BUILD",
            sha256 = "45337317c7f238fe0d64bb5d5418d264a427efc53400ddf8e6a964b6bcb31ce9",
            strip_prefix = "xsimd-7.5.0",
            urls = [
                "https://github.com/xtensor-stack/xsimd/archive/refs/tags/7.5.0.tar.gz",
            ],
        )

    if "zlib" not in native.existing_rules():   
        http_archive(
            name = "zlib",
            build_file = "@primihub//bazel:zlib.BUILD",
            sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
            strip_prefix = "zlib-1.2.11",
            urls = [
                "https://zlib.net/zlib-1.2.11.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/zlib-1.2.11.tar.gz"
            ],
        )

    if "zstd" not in native.existing_rules():   
        http_archive(
            name = "zstd",
            build_file = "@primihub//bazel:zstd.BUILD",
            sha256 = "a364f5162c7d1a455cc915e8e3cf5f4bd8b75d09bc0f53965b0c9ca1383c52c8",
            strip_prefix = "zstd-1.4.4",
            urls = [
                "https://github.com/facebook/zstd/archive/v1.4.4.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/zstd-1.4.4.tar.gz"
            ],
        )
    
    if "xz" not in native.existing_rules():   
        http_archive(
            name = "xz",
            build_file = "@primihub//bazel:xz.BUILD",
            sha256 = "0d2b89629f13dd1a0602810529327195eff5f62a0142ccd65b903bc16a4ac78a",
            strip_prefix = "xz-5.2.5",
            urls = [
                "https://github.com/xz-mirror/xz/archive/v5.2.5.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/xz-5.2.5.tar.gz"
            ],
        )
    
    if "io_opentelemetry_cpp" not in native.existing_rules():   
        http_archive(
            name = "io_opentelemetry_cpp",
            # sha256 = "<sha256>",
            strip_prefix = "opentelemetry-cpp-1.0.1",
            urls = [
                "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v1.0.1.tar.gz",
                "https://primihub.oss-cn-beijing.aliyuncs.com/opentelemetry-cpp-1.0.1.tar.gz"
            ],
        )
    # # Load OpenTelemetry dependencies after load.
    # load("@io_opentelemetry_cpp//bazel:repository.bzl", "opentelemetry_cpp_deps")

    # opentelemetry_cpp_deps()
    
    # arrow deps end here