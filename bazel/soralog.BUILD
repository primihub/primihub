# Description:
#   soralog library

package(default_visibility = ["//visibility:public"])


cc_library(
    name = "soralog",
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/*.cpp",
            
        ]
    ),
    hdrs = glob([
        "include/soralog/*.hpp",
        "include/soralog/**/*.hpp",
        
        ]
    ),
    copts = ['--std=c++17'],
    includes = [
        "include/",
    ],
    include_prefix = "include",
    deps = [
       "@fmt",  
       "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)