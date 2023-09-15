package(default_visibility = ["//visibility:public"])

# tools and path used by trusted code
filegroup(
    name = "sgx_edger8r",
    srcs = ["bin/x64/sgx_edger8r"],
)
filegroup(
    name = "sgx_sign",
    srcs = ["bin/x64/sgx_sign"],
)

filegroup(
    name = "sgx_headers",
    srcs = glob(["include/**/*"]),
)

# The libsgx_tcrypto.a may be used outside enclave.
filegroup(
    name = "sgx_tcrypto",
    srcs = ["lib64/libsgx_tcrypto.a"],
)

filegroup(
    name = "sgx_trusted_static_libs",
    srcs = [
        ":sgx_tcrypto",
        "lib64/libsgx_trts.a",
        "lib64/libsgx_tstdc.a",
        "lib64/libsgx_tcxx.a",
        "lib64/libsgx_tprotected_fs.a",
        "lib64/libsgx_tkey_exchange.a",
        "lib64/libsgx_tservice.a",
    ],
)

filegroup(
    name = "sgx_openssl_headers",
    srcs = glob(["sgxssl/include/**/*"]),
)

# sgx_openssl is only be used inside encalves.
filegroup(
    name = "sgx_openssl_trusted_libs",
    srcs = ["sgxssl/lib64/libsgx_tsgxssl.a", "sgxssl/lib64/libsgx_tsgxssl_crypto.a"],
)

cc_import(
    name = "sgx_openssl_untrusted_libs",
    hdrs = [],
    static_library = "sgxssl/lib64/libsgx_usgxssl.a",
)

filegroup(
    name = "sgx_qvl_headers",
    srcs = glob(["sgxqvl/include/*"]),
)

# sgx dcap verification files
cc_library(
    name = "sgxqvl_lib",
    hdrs = [":sgx_qvl_headers"],
    strip_include_prefix = "sgxqvl/include",
    srcs = glob(["sgxqvl/lib64/*.so"]),
)
cc_library(
    name = "sgxql_lib",
    hdrs = [":sgx_qvl_headers"],
    strip_include_prefix = "sgxqvl/include",
    srcs = [],
)

# library used by untrusted code
cc_library(
    name = "sgx_lib",
    hdrs = [":sgx_headers"],
    strip_include_prefix = "include",
    linkstatic = 1,
    # Do Not link shared libraries in order to run code in NO-SGX environment.
    srcs = [":sgx_headers", "lib64/libsgx_capable.a", ":sgx_tcrypto"] + glob(["lib64/libsgx_u*.a"]),
)
