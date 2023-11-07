package(default_visibility = ["//visibility:public",],)
cc_library(
  name = "seatunnel_driver",
  hdrs = ["seatunnel_driver.h"],
  srcs = ["seatunnel_driver.cc"],
  deps = [
    "//src/primihub/data_store:base_driver",
    "//src/primihub/util:util_lib",
    "//src/primihub/util:thread_local_data",
    "//src/primihub/util/network:communication_lib",
    "@arrow",
    "@nlohmann_json",
    "@pybind11",
  ],
)