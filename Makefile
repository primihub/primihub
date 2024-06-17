BUILD_FLAG ?=

TARGET := //:node \
          //:cli \
          //:task_main

ifneq ($(disable_py_task), y)
  TARGET += //src/primihub/pybind_warpper:linkcontext \
      //src/primihub/pybind_warpper:opt_paillier_c2py \
      //src/primihub/task/pybind_wrapper:ph_secure_lib
  BUILD_FLAG += --define enable_py_task=true
endif

ifeq ($(mysql), y)
  BUILD_FLAG += --define enable_mysql_driver=true
endif

ifeq ($(protos), y)
  TARGET += //src/primihub/protos:worker_py_pb2_grpc \
      //src/primihub/protos:service_py_pb2_grpc
endif

JOBS?=
ifneq ($(jobs), )
	JOBS = $(jobs)
	BUILD_FLAG += --jobs=$(JOBS)
endif

ifeq ($(tee), y)
	BUILD_FLAG += --cxxopt=-DSGX
	BUILD_FLAG += --define enable_sgx=true
endif

ifeq ($(debug), y)
	BUILD_FLAG += --config=linux_asan
endif

release:
	bazel build --config=PLATFORM_HARDWARE $(BUILD_FLAG) ${TARGET}
	rm -f primihub-cli
	ln -s -f bazel-bin/cli primihub-cli
	rm -f primihub-node
	ln -s -f bazel-bin/node primihub-node

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
