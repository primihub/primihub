
# Description:
#   fmt library

package(default_visibility = ["//visibility:public"])


cc_library(
    name = "fmt",
    srcs = [
        #"src/fmt.cc", # No C++ module support
        "src/format.cc",
        "src/posix.cc",
    ],
    hdrs = glob([
        "include/fmt/*.h"
    ]),
    includes = [
        "include",
        "src",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)