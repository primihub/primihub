# Description:
#   sqlite library


package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sqlite-cpp",
    srcs = glob(
        [
            # "src/**/*.cpp",
            # "src/**/*.hpp",

        ],

    ),
    hdrs = glob([
            "hdr/sqlite_modern_cpp/**/*.h",
            "hdr/sqlite_modern_cpp/*.h",
        
        ]
    ),
    copts = ['--std=c++17'],
    includes = [
        "hdr/sqlite_modern_cpp/",
    ],
    include_prefix = "hdr/",
    deps = [ 
    ],
)