BUILD_FLAG ?= 

TARGET := //:node \
          //:cli \
          //src/primihub/cli:reg_cli \
          //src/primihub/pybind_warpper:linkcontext \
          //src/primihub/pybind_warpper:opt_paillier_c2py \
          //:py_main

ifeq ($(mysql), y)
    BUILD_FLAG += --define enable_mysql_driver=true
endif

ifeq ($(protos), y)
    TARGET += //src/primihub/protos:worker_py_pb2_grpc //src/primihub/protos:service_py_pb2_grpc
endif

release:
	bazel build --config=PLATFORM_HARDWARE $(BUILD_FLAG) ${TARGET}
	ln -s bazel-bin/cli primihub-cli
	ln -s bazel-bin/node primihub-node

#linux_x86_64:
#	bazel build --config=linux_x86_64 ${TARGET}
#
#linux_aarch64:
#	bazel build --config=linux_aarch64 ${TARGET}
#
#macos_arm64:
#	bazel build --config=darwin_arm64 ${TARGET}
#
#macos_x86_64:
#	bazel build --config=darwin_x86_64 ${TARGET}

.PHONY: clean

clean:
	bazel clean
