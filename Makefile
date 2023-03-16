TARGET := //:node \
         //:cli \
         //:linkcontext \
         //:opt_paillier_c2py \
         //:linkcontext \
         //:py_main

linux_x86_64:
	bazel build --config=linux_x86_64 ${TARGET}

linux_aarch64:
	bazel build --config=linux_aarch64 ${TARGET}

macos_arm64:
	bazel build --config=darwin_arm64 ${TARGET}

macos_x86_64:
	bazel build --config=darwin_x86_64 ${TARGET}

.PHONY: clean

clean:
	bazel clean
