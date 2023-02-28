#TARGET := //:node \
#         //:cli \
#         //:linkcontext \
#         //:opt_paillier_c2py \
#         //:linkcontext \
#         //:py_main

TARGET := //:node \
         //:cli \
         //src/primihub/pybind_warpper:linkcontext \
         //:py_main


linux_x86_64:
	bazel build --config=linux_x86_64 ${TARGET}

linux_aarch64:
	bazel build --config=linux_aarch64 ${TARGET}

macos_arm64:
	bazel build --config=macos --define cpu=arm64 --define microsoft-apsi=true ${TARGET}

.PHONY: clean

clean:
	bazel clean
