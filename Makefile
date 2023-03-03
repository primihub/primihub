
TARGET := //:node \
         //:cli \
         //src/primihub/pybind_warpper:linkcontext \
         //src/primihub/pybind_warpper:opt_paillier_c2py \
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
